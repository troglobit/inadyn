/* Plugin for https://hn.org
 *
 * Copyright (C) 2016  Joachim Nilsson <troglobit@gmail.com>
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

#include <ctype.h>
#include "plugin.h"

/*
 * Example:
 *             Update tyrion.hn.org to IPv4 address 10.2.3.4,
 *             using password "^F*D&S^s~09d".
 *
 * POST /update.php HTTP/1.1
 * Host: tyrionzqj32v3.hn.org
 * Content-Type: application/x-www-form-urlencoded
 * Content-Length: 68
 * User-Agent: PostProgram/1.1
 * 
 * hostname=tyrion.hn.org&password=%5EF%2AD%26S%5Es%7E09d&ipv4=10.2.3.4
 */
#define HN_UPDATE_IP_HTTP_REQUEST					\
	"POST %s HTTP/1.1\r\n"						\
	"Host: %s\r\n"							\
	"Content-Type: application/x-www-form-urlencoded\r\n"		\
	"Content-Length: %d\r\n"					\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"		\
	"%s"

static int request  (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *alias);
static int response (http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias);

static ddns_system_t plugin = {
	.name         = "default@hn.org",

	.request      = (req_fn_t)request,
	.response     = (rsp_fn_t)response,

//	.checkip_name = "myipv3.hn.org",
//	.checkip_url  = "/",
	.checkip_name = DYNDNS_MY_IP_SERVER,
	.checkip_url  = DYNDNS_MY_CHECKIP_URL,

	.server_name  = "troglobitzqj32v3.hn.org",
//	.server_name  = "hn.org",
	.server_url   = "/update.php"
};

static char *urlencode(char *str, char *buf, size_t len)
{
	size_t i, j;

	if (!buf || !len) {
		errno = EINVAL;
		return NULL;
	}
		
	memset(buf, 0, len);

	for (i = 0, j = 0; i < strlen(str) && j < len; i++) {
		char c = str[i];

		if (isalnum(c) || c == '-' || c == '.' || c == '_') {
			buf[j++] = c;
			continue;
		}

		if (j + 4 < len) {
			snprintf(&buf[j], 4, "%%%02X", c);
			j += 3;
		}
	}

	return buf;
}

/*
 * Users get to choose from five pools of hostnames
 *
 * .hn.org      (id 32)
 * .home.hn.org (id 33)
 * .base.hn.org (id 34)
 * .a.hn.org    (id 35)
 * .b.hn.org    (id 36)
 */
int pool2id(char *pool)
{
	int i;
	struct {
		char *pool;
		int   id;
	} map[] = {
		{ ".hn.org",      32 },
		{ ".home.hn.org", 33 },
		{ ".base.hn.org", 34 },
		{ ".a.hn.org",    35 },
		{ ".b.hn.org",    36 },
		{ NULL, 0 },
	};

	for (i = 0; map[i].pool; i++) {
		if (!strcmp(pool, map[i].pool))
			return map[i].id;
	}

	return -1;
}

int map_hostpool(char *alias, char *host, size_t len)
{
	int id;
	char *ptr;

	strlcpy(host, alias, len);
	ptr = strchr(host, '.');
	if (!ptr)
		return -1;

	id = pool2id(ptr);
	if (id <= 0)
		return -1;

	snprintf(&ptr[0], len - (ptr - host), "zqj%dv3.hn.org", id);

	return 0;
}

/* The alias must be the full tyrion.hn.org since we parse the alias to figure out
 * which of hn.org, home.hn.org, base.hn.org, etc. you use.  That in turn leads to
 * the pool mapping defined */
static int request(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *alias)
{
	int len;
	char host[256];
	char buf[512];

	len = snprintf(buf, sizeof(buf), "hostname=%s&password=%s&ipv4=%s",
		       alias->name,
		       urlencode(info->creds.password, ctx->request_buf, ctx->request_buflen),
		       alias->address);

	if (map_hostpool(alias->name, host, sizeof(host)))
		return -1;

	return snprintf(ctx->request_buf, ctx->request_buflen,
			HN_UPDATE_IP_HTTP_REQUEST,
			info->server_url, host, len, buf);
}

static int response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias)
{
	char *body = trans->p_rsp_body;

	DO(http_status_valid(trans->status));

	if (strstr(body, "UPDATE_RESPONSE_CODE=101"))
		return 0;

	/* Too frequent updates, must wait at least 300 sec || 
	 * Too many incoming requests from client IPv4 address. */
	if (strstr(body, "UPDATE_RESPONSE_CODE=201") || strstr(body, "UPDATE_RESPONSE_CODE=206"))
		return RC_DYNDNS_RSP_RETRY_LATER;

	/* Server error */
	if (strstr(body, "UPDATE_RESPONSE_CODE=202"))
		return RC_DYNDNS_RSP_RETRY_LATER;

	/* Password incorrect or hostname nonexistent or hostname inactive, or
	 * Account in static dns mode*/
	if (strstr(body, "UPDATE_RESPONSE_CODE=205") || strstr(body, "UPDATE_RESPONSE_CODE=208"))
		return RC_DYNDNS_INVALID_OPTION;

	/* Missing user-agent */
	if (strstr(body, "UPDATE_RESPONSE_CODE=207"))
		return RC_DYNDNS_INVALID_OR_MISSING_PARAMETERS;

	/* Submitted fields rejected (too much data, illegal characters, etc.) */
	if (strstr(body, "UPDATE_RESPONSE_CODE=209"))
		return RC_DYNDNS_INVALID_IP_ADDR_IN_HTTP_RESPONSE;

	return RC_DYNDNS_RSP_NOTOK;
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
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
