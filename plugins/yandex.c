/* Plugin for Yandex PDD (Yandex.Connect)
 *
 * Copyright (C) 2017 https://github.com/dmitrodem
 * Copyright (C) 2019 Timur Birsh <taem@linukz.org>
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

#define YANDEX_GET_REQUEST						\
	"GET %s "							\
	"HTTP/1.1\r\n"							\
	"Host: %s\r\n"							\
	"PddToken: %s\r\n"						\
	"User-Agent: %s\r\n\r\n"

#define YANDEX_POST_REQUEST						\
	"POST %s "							\
	"HTTP/1.1\r\n"							\
	"Host: %s\r\n"							\
	"PddToken: %s\r\n"						\
	"User-Agent: %s\r\n"						\
	"Content-Length: %i\r\n"					\
	"Content-Type: application/x-www-form-urlencoded\r\n\r\n"	\
	"%s"

static int request  (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *alias);
static int response (http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias);

static ddns_system_t plugin = {
	.name         = "default@pdd.yandex.ru",

	.request      = (req_fn_t)request,
	.response     = (rsp_fn_t)response,

	.checkip_name = DYNDNS_MY_IP_SERVER,
	.checkip_url  = DYNDNS_MY_CHECKIP_URL,
	.checkip_ssl  = DYNDNS_MY_IP_SSL,

	.server_name  = "pddimp.yandex.ru:443",
	.server_url   = "/dynamic/update.php"
};

// XXX: please increase if not enough
#define TOKENS_EXPECTED		4096

// FIXME: remove and use parse_json instead
static jsmntok_t *get_tokens(const char *response, int *n) {
	jsmn_parser *p;
	jsmntok_t   *t;
	int	     r;

	p = malloc(sizeof(*p));
	if (!p) {
		logit(LOG_ERR, "Not enough memory\n");
		return NULL;
	}

	t = malloc(sizeof(*t) * TOKENS_EXPECTED);
	if (!t) {
		logit(LOG_ERR, "Not enough memory\n");
		free(p);
		return NULL;
	}

	jsmn_init(p);
	r = jsmn_parse(p, response, strlen(response), t, TOKENS_EXPECTED);
	free(p);
	*n = r;
	if (r < 0){
		logit(LOG_ERR, "Failed to parse JSON");
		free(t);
		return NULL;
	}
	if ((r < 1) || (t[0].type != JSMN_OBJECT)) {
		logit(LOG_ERR, "JSON object expected");
		free(t);
		return NULL;
	}

	return t;
}

static int success(const char *response) {
	jsmntok_t *t;
	int	   n, i;
	int	   retval = 0;

	t = get_tokens(response, &n);
	if (!t)
		return 0;

	for (i = 1; i < n-1; i++) {
		jsmntok_t *tok = &t[i];
		if (jsoneq(response, tok, "success") == 0) {
			if (jsoneq(response, tok + 1, "ok") == 0) {
				retval = 1;
				goto out;
			}
		}
	}

out:
	free(t);
	return retval;
}

static int get_record_id(const char *response, const char *subdomain){
	jsmntok_t *t;
	int	   n, i;
	int	   record_id = 0;
	int	   id_keyword_found = 0;

	enum {
		ST_UNKNOWN,
		ST_FIND_RECORDS,
		ST_FIND_RECORD_ID
	} state = ST_UNKNOWN;

	enum {
		SUBDOMAIN_NONE,
		SUBDOMAIN_KEYWORD,
		SUBDOMAIN_VALUE
	} subdomain_field;

	enum {
		TYPE_NONE,
		TYPE_KEYWORD,
		TYPE_VALUE
	} type_field;

	t = get_tokens(response, &n);
	if (!t)
		return -1;

	for (i = 1; i < n; i++) {
		jsmntok_t *tok = &t[i];

		switch (tok->type) {
		case JSMN_STRING:
		case JSMN_PRIMITIVE:
			if (jsoneq(response, tok, "records") == 0) {
				state = ST_FIND_RECORDS;
				break;
			}

			if (state != ST_FIND_RECORD_ID)
				break;

			if (jsoneq(response, tok, "subdomain") == 0) {
				subdomain_field = SUBDOMAIN_KEYWORD;
				break;
			}

			if (jsoneq(response, tok, "type") == 0) {
				type_field = TYPE_KEYWORD;
				break;
			}

			if (jsoneq(response, tok, "record_id") == 0) {
				id_keyword_found = 1;
				break;
			}

			if ((subdomain_field == SUBDOMAIN_KEYWORD) &&
					(jsoneq(response, tok, subdomain)) == 0)
				subdomain_field = SUBDOMAIN_VALUE;

			if ((type_field == TYPE_KEYWORD) &&
					(jsoneq(response, tok, "A")) == 0)
				type_field = TYPE_VALUE;

			if (id_keyword_found) {
				char buf[129]; // additional byte for \0
				size_t n = tok->end - tok->start;

				id_keyword_found = 0;
				if (n > sizeof(buf) - 1)
					break;

				memset(buf, 0, sizeof(buf));
				strncpy(buf, response + tok->start, n);
				record_id = strtol(buf, NULL, 10);
			}

			if (subdomain_field == SUBDOMAIN_VALUE &&
					type_field == TYPE_VALUE &&
					record_id > 0) {
				free(t);
				return record_id;
			}

			break;
		case JSMN_ARRAY:
			if (state != ST_FIND_RECORDS)
				break;

			state = ST_FIND_RECORD_ID;
			break;
		case JSMN_OBJECT:
			if (state != ST_FIND_RECORD_ID)
				break;

			subdomain_field  = SUBDOMAIN_NONE;
			type_field	 = TYPE_NONE;
			id_keyword_found = 0;
			break;
		}
	}

	free(t);

	if (state != ST_FIND_RECORD_ID) {
		logit(LOG_ERR, "Got JSON document that cannot understand\n");
		return -1;
	}

	return 0;
}

static int request(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *alias) {
	http_t	     client;
	http_trans_t trans;
	char	     url[512];
	char	     *resp;
	int	     record_id;
	int	     len;
	int	     rc = 0;

	do {
		TRY(http_construct(&client));

		http_set_port(&client, info->server_name.port);
		http_set_remote_name(&client, info->server_name.name);

		client.ssl_enabled = info->ssl_enabled;
		TRY(http_init(&client, "Trying to get record_id"));

		snprintf(url, sizeof(url), "/api2/admin/dns/list?domain=%s",
			 info->creds.username);

		trans.req_len = snprintf(ctx->request_buf, ctx->request_buflen,
					 YANDEX_GET_REQUEST, url,
					 info->server_name.name,
					 info->creds.password,
					 info->user_agent);
		trans.req = ctx->request_buf;
		trans.rsp = ctx->work_buf;
		trans.max_rsp_len = ctx->work_buflen - 1;

		rc  = http_transaction(&client, &trans);
		rc |= http_exit(&client);

		http_destruct(&client, 1);

		if (rc)
			break;

		TRY(http_status_valid(trans.status));
		resp = trans.rsp_body;

		logit(LOG_DEBUG, "Yandex response: %s\n", resp);

		if (!success(resp))
			break;

		record_id = get_record_id(resp, alias->name);

		if (record_id < 0)
			break;

		if (record_id > 0) {
			logit(LOG_INFO, "Updating record, id = %i", record_id);
			len = snprintf(url, sizeof(url),
					"domain=%s&record_id=%i&subdomain=%s&content=%s",
					info->creds.username, record_id, alias->name,
					alias->address);
		} else {
			logit(LOG_INFO, "Creating record");
			len = snprintf(url, sizeof(url),
					"domain=%s&type=A&subdomain=%s&content=%s",
					info->creds.username, alias->name,
					alias->address);
		}

		return snprintf(ctx->request_buf, ctx->request_buflen,
				YANDEX_POST_REQUEST,
				record_id > 0 ? "/api2/admin/dns/edit"
					      : "/api2/admin/dns/add",
				info->server_name.name,
				info->creds.password,
				info->user_agent,
				len, url);
	} while (0);

	return -1;
}

static int response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias) {
	char *resp = trans->rsp_body;

	DO(http_status_valid(trans->status));

	if (success(resp))
		return 0;

	return RC_DDNS_RSP_NOTOK;
}

PLUGIN_INIT(plugin_init)
{
	plugin_register(&plugin);
}

PLUGIN_EXIT(plugin_exit)
{
	plugin_unregister(&plugin);
}
