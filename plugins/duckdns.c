/* Plugin for DuckDNS
 *
 * Copyright (C) 2010-2014  Joachim Nilsson <troglobit@gmail.com>
 * Copyright (C) 2014       Andy Padavan <andy.padavan@gmail.com>
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

/*
 * For API documentation we currently only have this
 *     https://www.duckdns.org/install.jsp#linux-cron
 */
#define DUCKDNS_UPDATE_IP_HTTP_REQUEST					\
	"GET %s?"							\
	"domains=%s&"							\
	"token=%s&"							\
	"ip=%s "   							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

static int request (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *alias);
static int response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias);

static ddns_system_t plugin = {
	.name         = "default@duckdns.org",

	.request      = (req_fn_t)request,
	.response     = (rsp_fn_t)response,

	.checkip_name = "ipv4.wtfismyip.com",
	.checkip_url  = "/text",
	.checkip_ssl  = DDNS_CHECKIP_SSL_SUPPORTED,

	.server_name  = "www.duckdns.org",
	.server_url   = "/update"
};

static int request(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *alias)
{
	char name[SERVER_NAME_LEN], *p;

	/* Handle some.name.duckdns.org properly */
	p = strstr(alias->name, "duckdns.org");
	if (p) {
		size_t len = p - alias->name;

		if (len > sizeof(name))
			len = sizeof(name);

		strlcpy(name, alias->name, len);
	} else {
		snprintf(name, sizeof(name), "%s", alias->name);
	}

	return snprintf(ctx->request_buf, ctx->request_buflen,
			DUCKDNS_UPDATE_IP_HTTP_REQUEST,
			info->server_url,
			name,
			info->creds.username,
			alias->address,
			info->server_name.name);
}

static int response(http_trans_t *trans, ddns_info_t *UNUSED(info), ddns_alias_t *UNUSED(alias))
{
	char *resp = trans->p_rsp_body;

	DO(http_status_valid(trans->status));

	if (strstr(resp, "OK") || strstr(resp, "good"))
		return RC_OK;

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
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
