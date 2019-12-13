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
#include "json.h"

#define CHECK(fn)       { rc = (fn); if (rc) goto cleanup; }

#define API_HOST "api.cloudflare.com"
#define API_URL "/client/v4"

static const char *CLOUDFLARE_ZONE_ID_REQUEST = "GET " API_URL "/zones?name=%s HTTP/1.0\r\n"	\
	"Host: " API_HOST "\r\n"	\
	"User-Agent: %s\r\n"		\
	"Accept: */*\r\n"			\
	"X-Auth-Email: %s\r\n"		\
	"X-Auth-Key: %s\r\n"		\
	"Content-Type: application/json\r\n\r\n";
	
static const char *CLOUDFLARE_HOSTNAME_ID_REQUEST	= "GET " API_URL "/zones/%s/dns_records?type=A&name=%s HTTP/1.0\r\n"	\
	"Host: " API_HOST "\r\n"	\
	"User-Agent: %s\r\n"		\
	"Accept: */*\r\n"			\
	"X-Auth-Email: %s\r\n"		\
	"X-Auth-Key: %s\r\n"		\
	"Content-Type: application/json\r\n\r\n";
	
static const char *CLOUDFLARE_HOSTNAME_UPDATE_REQUEST	= "PUT " API_URL "/zones/%s/dns_records/%s HTTP/1.0\r\n"	\
	"Host: " API_HOST "\r\n"	\
	"User-Agent: %s\r\n"		\
	"Accept: */*\r\n"			\
	"X-Auth-Email: %s\r\n"		\
	"X-Auth-Key: %s\r\n"		\
	"Content-Type: application/json\r\n" \
	"Content-Length: %zd\r\n\r\n" \
	"%s";
	
static const char *CLOUDFLARE_UPDATE_JSON_FORMAT = "{\"type\":\"A\",\"name\":\"%s\",\"content\":\"%s\"}";

static const char *KEY_SUCCESS = "success";

static int setup	(ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *hostname);
static int request  (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *hostname);
static int response (http_trans_t *trans, ddns_info_t *info, ddns_alias_t *hostname);

static ddns_system_t plugin = {
	.name         = "default@cloudflare.com",

	.setup        = (setup_fn_t)setup,
	.request      = (req_fn_t)request,
	.response     = (rsp_fn_t)response,

	.checkip_name = DYNDNS_MY_IP_SERVER,
	.checkip_url  = DYNDNS_MY_CHECKIP_URL,
	.checkip_ssl  = DYNDNS_MY_IP_SSL,

	.server_name  = "api.cloudflare.com",
	.server_url   = "/client/v4"
};

#define MAX_NAME 64
#define MAX_ID (32 + 1)

// filled by the setup func
static struct {
	char zone_id[MAX_ID];
	char hostname_id[MAX_ID];
} data;

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
			logit(LOG_ERR, "We're not allowed to perform the DNS update with the provided credentials.");
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
	
	return -1;
}

static int check_success_only(const char *json)
{
	int num_tokens;
	jsmntok_t *tokens;
	
	num_tokens = parse_json(json, &tokens);	
	
	if (num_tokens == -1)
		return -1;

	int result = check_success(json, tokens, num_tokens);

	free(tokens);
	return result;
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
		goto cleanup;
	}
	
	if (check_success(json, tokens, num_tokens) == -1) {
		logit(LOG_ERR, "Request was unsuccessful.");
		goto cleanup;
	}
	
	for (int i = 1; i < num_tokens; i++) {
		if (jsoneq(json, tokens + i, key) == 0) {
			if (i < num_tokens - 1) {
				*out_result = tokens[i+1];
				free(tokens);
				return 0;
			}
		}
	}
	
	logit(LOG_ERR, "Could not find key '%s'.", key);

cleanup:
	free(tokens);
	return -1;
}

static int json_copy_value(char *dest, size_t dest_size, const char *json, const jsmntok_t *token)
{
	if (token->type != JSMN_STRING)
		return -1;
	
	size_t length = token->end - token->start;
	
	if (length > dest_size - 1)
		return -2;
	
	strncpy(dest, json + token->start, length);
	dest[length] = '\0';
	
	return 0;
}

static int get_id(char *dest, size_t dest_size, const ddns_info_t *info, char *request, size_t request_len)
{
	int rc = RC_OK;

	const size_t RESP_BUFFER_SIZE = 4096;
	
	http_t        client;
	http_trans_t  trans;

	char *response_buf = malloc(RESP_BUFFER_SIZE * sizeof(char));

	if (!response_buf)
		return RC_OUT_OF_MEMORY;

	CHECK(http_construct(&client));

	http_set_port(&client, info->server_name.port);
	http_set_remote_name(&client, info->server_name.name);

	client.ssl_enabled = info->ssl_enabled;
	CHECK(http_init(&client, "Id query"));

	trans.req = request;
	trans.req_len = request_len;
	trans.rsp = response_buf;
	trans.max_rsp_len = RESP_BUFFER_SIZE - 1; /* Save place for a \0 at the end */

	logit(LOG_DEBUG, "Request:\n%s", request);
	CHECK(http_transaction(&client, &trans));

	http_exit(&client);
	http_destruct(&client, 1);

	logit(LOG_DEBUG, "Response:\n%s", trans.rsp);
	CHECK(check_response_code(trans.status));

	const char *response = trans.rsp_body;
	jsmntok_t id;

	if (get_result_value(response, "id", &id) < 0) {
		rc = RC_DDNS_INVALID_OPTION;
		goto cleanup;
	}

	if (json_copy_value(dest, dest_size, response, &id) < 0) {
		logit(LOG_ERR, "Id did not fit into buffer.");
		rc = RC_BUFFER_OVERFLOW;
	}

cleanup:
	free(response_buf);
	return rc;
}

static int check_username(const ddns_info_t *info)
{
	/* cloudflare complains about request headers if the username is not an email, so let's try to make sure it is */
	if (!strchr(info->creds.username, '@')) {
		logit(LOG_ERR, "Username not in correct format. (Should be an email address.)");
		return RC_DDNS_INVALID_OPTION;
	}

	return RC_OK;
}

static void get_zone(char *dest, const char *hostname)
{
	const char *end = hostname + strlen(hostname);
	const char *root;

	int count = 0;
	for (root = end; root != hostname; root--) {
		if (*root == '.')
			count++;

		if (count == 2) {
			root++;
			break;
		}
	}

	strncpy(dest, root, MAX_NAME);
}

static int setup(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *hostname)
{
	int rc = RC_OK;

	const size_t REQUEST_BUFFER_SIZE = 1028;

	char zone_name[MAX_NAME];

	DO(check_username(info));
	get_zone(zone_name, hostname->name);

	logit(LOG_DEBUG, "User: %s Zone: %s", info->creds.username, zone_name);

	char *request_buf = malloc(REQUEST_BUFFER_SIZE * sizeof(char));
	if (!request_buf)
		return RC_OUT_OF_MEMORY;

	{
		size_t request_len = snprintf(request_buf, REQUEST_BUFFER_SIZE,
			CLOUDFLARE_ZONE_ID_REQUEST,
			zone_name,
			info->user_agent,
			info->creds.username,
			info->creds.password);

		if (request_len > REQUEST_BUFFER_SIZE) {
			logit(LOG_ERR, "Request did not fit into buffer.", zone_name);
			rc = RC_BUFFER_OVERFLOW;
			goto cleanup;
		}

		rc = get_id(data.zone_id, MAX_ID, info, request_buf, request_len);

		if (rc != RC_OK) {
			logit(LOG_ERR, "Zone '%s' not found.", zone_name);
			rc = RC_DDNS_RSP_NOHOST;
			goto cleanup;
		}
	}
	
	logit(LOG_DEBUG, "Cloudflare Zone: '%s' Id: %s", zone_name, data.zone_id);

	{
		size_t request_len = snprintf(request_buf, REQUEST_BUFFER_SIZE,
			CLOUDFLARE_HOSTNAME_ID_REQUEST,
			data.zone_id,
			hostname->name,
			info->user_agent,
			info->creds.username,
			info->creds.password);

		if (request_len > REQUEST_BUFFER_SIZE) {
			logit(LOG_ERR, "Request did not fit into buffer.", zone_name);
			rc = RC_BUFFER_OVERFLOW;
			goto cleanup;
		}

		rc = get_id(data.hostname_id, MAX_ID, info, request_buf, request_len);

		if (rc != RC_OK) {
			logit(LOG_ERR, "Hostname '%s' not found.", hostname->name);
			rc = RC_DDNS_RSP_NOHOST;
			goto cleanup;
		}
	}

	logit(LOG_DEBUG, "Cloudflare Host: '%s' Id: %s", hostname->name, data.hostname_id);

cleanup:
	free(request_buf);
	return rc;
}

static int request(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *hostname)
{
	char          json_data[256];
	size_t content_len = snprintf(json_data, sizeof(json_data), CLOUDFLARE_UPDATE_JSON_FORMAT, hostname->name, hostname->address);

	return snprintf(ctx->request_buf, ctx->request_buflen,
		CLOUDFLARE_HOSTNAME_UPDATE_REQUEST,
		data.zone_id, data.hostname_id,
		info->user_agent,
		info->creds.username,
		info->creds.password,
		content_len, json_data);
}

static int response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *hostname)
{
	(void)info;
	(void)hostname;

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
