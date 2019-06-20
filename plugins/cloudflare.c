/* Plugin for Cloudflare
 *
 * Copyright (C) 2019 SimonP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, visit the Free Software Foundation
 * website at http://www.gnu.org/licenses/gpl-2.0.html or write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "plugin.h"

#define JSMN_HEADER
#include "jsmn.h"

/* cloudxns.net specific update request format */
static const char *CLOUDFLARE_ZONE_ID	= "GET %s/zones?name=%s HTTP/1.0\r\n"	\
	"Host: %s\r\n"				\
	"User-Agent: %s\r\n"		\
	"Accept: */*\r\n"			\
	"X-Auth-Email: %s\r\n"		\
	"X-Auth-Key: %s\r\n"		\
	"Content-Type: application/json\r\n\r\n";
	
static const char *CLOUDFLARE_DOMAIN_ID	= "GET %s/zones/%s/dns_records?type=A&name=%s HTTP/1.0\r\n"	\
	"Host: %s\r\n"				\
	"User-Agent: %s\r\n"		\
	"Accept: */*\r\n"			\
	"X-Auth-Email: %s\r\n"		\
	"X-Auth-Key: %s\r\n"		\
	"Content-Type: application/json\r\n\r\n";
	
static const char *CLOUDFLARE_UPDATE_REQUEST	= "PUT %s/zones/%s/dns_records/%s HTTP/1.0\r\n"	\
	"Host: %s\r\n"				\
	"User-Agent: %s\r\n"		\
	"Accept: */*\r\n"			\
	"X-Auth-Email: %s\r\n"		\
	"X-Auth-Key: %s\r\n"		\
	"Content-Type: application/json\r\n" \
	"Content-Length: %zd\r\n\r\n" \
	"%s";
	
static const char *CLOUDFLARE_UPDATE_JSON_FORMAT = "{\"type\":\"A\",\"name\":\"%s\",\"content\":\"%s\"}";

static const char *KEY_SUCCESS = "success";

static int request  (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *hostname);
static int response (http_trans_t *trans, ddns_info_t *info, ddns_alias_t *hostname);

static ddns_system_t plugin = {
	.name         = "default@cloudflare.com",

	.request      = (req_fn_t)request,
	.response     = (rsp_fn_t)response,

	.checkip_name = DYNDNS_MY_IP_SERVER,
	.checkip_url  = DYNDNS_MY_CHECKIP_URL,
	.checkip_ssl  = DYNDNS_MY_IP_SSL,

	.server_name  = "api.cloudflare.com",
	.server_url   = "/client/v4"
};

static int check_response_code(int status)
{
	switch (status)
	{
		case 200:
		case 304:
			return RC_OK;
		case 400:
			logit(LOG_ERR, "Cloudflare says our request was invalid.");
			return RC_DDNS_RSP_NOTOK;
		case 401:
			logit(LOG_ERR, "Bad username.");
			return RC_DDNS_RSP_NOTOK;
		case 403:
			logit(LOG_ERR, "We're not allowed to perform the DNS update.");
			return RC_DDNS_INVALID_OPTION;
		case 429:
			logit(LOG_WARNING, "We got rate limited.");
			return RC_DDNS_RSP_RETRY_LATER;
		case 405:
			logit(LOG_ERR, "Bad HTTP method; the interface changed?");
			return RC_DDNS_RSP_NOTOK;
		case 415:
			logit(LOG_ERR, "Cloudflare didn't like our JSON; the inferface changed?");
			return RC_DDNS_RSP_NOTOK;
		default:
			logit(LOG_ERR, "Received status %i, don't know what that means.", status);
			return RC_DDNS_RSP_NOTOK;
	}
}

/* TODO: these could be useful in other files; move them to their own file. */

static int parse_json(const char *json, jsmntok_t *out_tokens[])
{
	int num_tokens;
	
	jsmn_parser parser;
	jsmn_init(&parser);
	num_tokens = jsmn_parse(&parser, json, strlen(json), NULL, 0);
	
	if (num_tokens < 0) {
		logit(LOG_ERR, "Failed to parse JSON.");
		return -1;
	}
	
	*out_tokens = malloc(num_tokens * sizeof(jsmntok_t));
	
	if (!(*out_tokens)) {
		logit(LOG_ERR, "Couldn't allocate memory to parse JSON.");
		return -1;
	}
	
	jsmn_init(&parser);
	jsmn_parse(&parser, json, strlen(json), *out_tokens, num_tokens);
	
	return num_tokens;
}

static int jsoneq(const char *json, const jsmntok_t *tok, const char *s)
{
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}

	return -1;
}

static int json_bool(const char *json, const jsmntok_t *token, int *out_value)
{
	if (token->type == JSMN_PRIMITIVE) {
		*out_value = json[token->start] == 't';
		return 0;
	}
	
	return -1;
}

static int check_success(const char *json, const jsmntok_t tokens[], const int num_tokens)
{
	for (int i = 1; i < num_tokens; i++) {
		if (jsoneq(json, tokens + i, KEY_SUCCESS) == 0) {
			int true;
			
			if (i < num_tokens - 1 && json_bool(json, tokens + i + 1, &true) == 0) {
				return true ? 0 : -1;
			}
				
			return -1;
		}
	}
}

static int check_success_only(const char *json)
{
	int num_tokens;
	jsmntok_t *tokens;
	
	num_tokens = parse_json(json, &tokens);	
	
	return check_success(json, tokens, num_tokens);
}

static int get_result_value(const char *json, const char *key, jsmntok_t *out_result)
{
	int num_tokens;
	jsmntok_t *tokens;
	
	num_tokens = parse_json(json, &tokens);
	
	if (num_tokens < 0)
		return -1;
	
	if (tokens[0].type != JSMN_OBJECT) {
		logit(LOG_ERR, "JSON response contained no objects.");
		free(tokens);
		return -1;
	}
	
	if (check_success(json, tokens, num_tokens) == -1) {
		logit(LOG_ERR, "Request was unsuccessful.");
		return -1;
	}
	
	for (int i = 1; i < num_tokens; i++) {
		if (jsoneq(json, tokens + i, key) == 0) {
			if (i < num_tokens - 1) {
				*out_result = tokens[i+1];
				return 0;
			}
		}
	}
	
	logit(LOG_ERR, "Could not find key '%s'.", key);
	return -1;
}

static int json_copy_value(char *dest, size_t dest_size, const char *json, const jsmntok_t *token)
{
	if (token->type != JSMN_STRING)
		return -1;
	
	int length = token->end - token->start;
	
	if (length > dest_size - 1)
		return -2;
	
	strncpy(dest, json + token->start, length);
	dest[length] = '\0';
	
	return 0;
}

static int request(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *hostname)
{
	int           rc = RC_ERROR;
	char          username[64], zone_name[64];
	char          zone_id[256], hostname_id[256];
	char          json_data[256];
		
	do {
		{
			const char *separator = strchr(info->creds.username, '/');
			
			/* cloudflare complains about request headers if the username is not an email, so let's try to make sure it is */
			const char *at = strchr(info->creds.username, '@');

			if (!separator || !at) {
				logit(LOG_ERR, "Username not in correct format.", hostname->name);
				rc = RC_DDNS_INVALID_OPTION;
				break;
			}
			
			size_t username_len = separator - info->creds.username;
			
			if (username_len > sizeof(username) - 1) {
				logit(LOG_ERR, "Username too long.");
				break;
			}

			strncpy(username, info->creds.username, username_len);
			username[username_len] = '\0';
			strncpy(zone_name, separator + 1, sizeof(zone_name) - 1);
		}

		logit(LOG_DEBUG, "User: %s Zone: %s", username, zone_name);
		
		{
			http_t        client;
			http_trans_t  trans;
			
			TRY(http_construct(&client));

			http_set_port(&client, info->server_name.port);
			http_set_remote_name(&client, info->server_name.name);

			client.ssl_enabled = info->ssl_enabled;
			TRY(http_init(&client, "Zone id query"));

			trans.req_len     = snprintf(ctx->request_buf, ctx->request_buflen,
										CLOUDFLARE_ZONE_ID,
										plugin.server_url,
										zone_name,
										info->server_name.name,
										info->user_agent,
										username,
										info->creds.password);
										
			trans.req         = ctx->request_buf;
			trans.rsp         = ctx->work_buf;
			trans.max_rsp_len = ctx->work_buflen - 1; /* Save place for a \0 at the end */
			
			TRY(http_transaction(&client, &trans));
			TRY(http_exit(&client));

			http_destruct(&client, 1);
			
			TRY(check_response_code(trans.status));
			{
				const char *response = trans.rsp_body;
				jsmntok_t id;
				if (get_result_value(response, "id", &id) < 0) {
					logit(LOG_ERR, "Zone '%s' not found.", zone_name);
					rc = RC_DDNS_INVALID_OPTION;
					break;
				}
				
				if (json_copy_value(zone_id, sizeof(zone_id), response, &id) < 0) {
					logit(LOG_ERR, "Zone id did not fit into buffer.");
					rc = RC_ERROR;
					break;
				}
			}
		}
		
		logit(LOG_DEBUG, "Cloudflare Zone: '%s' Id: %s", zone_name, zone_id);
		
		{
			http_t        client;
			http_trans_t  trans;
			
			TRY(http_construct(&client));

			http_set_port(&client, info->server_name.port);
			http_set_remote_name(&client, info->server_name.name);

			client.ssl_enabled = info->ssl_enabled;
			TRY(http_init(&client, "Hostname id query"));

			trans.req_len     = snprintf(ctx->request_buf, ctx->request_buflen,
										CLOUDFLARE_DOMAIN_ID, plugin.server_url,
										zone_id,
										hostname->name,
										info->server_name.name,
										info->user_agent,
										username,
										info->creds.password);
										
			trans.req         = ctx->request_buf;
			trans.rsp         = ctx->work_buf;
			trans.max_rsp_len = ctx->work_buflen - 1; /* Save place for a \0 at the end */

			TRY(http_transaction(&client, &trans));
			TRY(http_exit(&client));

			http_destruct(&client, 1);
			
			TRY(check_response_code(trans.status));
			{
				const char *response = trans.rsp_body;
				jsmntok_t id;
				if (get_result_value(response, "id", &id) < 0) {
					logit(LOG_ERR, "Hostname '%s' not found.", hostname->name);
					rc = RC_DDNS_INVALID_OPTION;
					break;
				}
				
				if (json_copy_value(hostname_id, sizeof(hostname_id), response, &id) < 0) {
					logit(LOG_ERR, "Hostname id did not fit into buffer.");
					rc = RC_ERROR;
					break;
				}
			}
		}
		
		{
			int contentLength = snprintf(json_data, sizeof(json_data), CLOUDFLARE_UPDATE_JSON_FORMAT, hostname->name, hostname->address);
			
			return snprintf(ctx->request_buf, ctx->request_buflen,
					   CLOUDFLARE_UPDATE_REQUEST,
					   plugin.server_url, zone_id, hostname_id,
					   info->server_name.name,
					   info->user_agent,
					   username,
					   info->creds.password,
					   contentLength, json_data);
		}
	}
	while (0);
	
	return -1;
}

static int response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *hostname)
{
	int rc = check_response_code(trans->status);

	if (rc == RC_OK && check_success_only(trans->rsp_body) < 0)
		rc = RC_DDNS_RSP_NOTOK;

	return rc;
}

PLUGIN_INIT(plugin_init)
{
	plugin_register(&plugin);
}

PLUGIN_EXIT(plugin_exit)
{
	plugin_unregister(&plugin);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
