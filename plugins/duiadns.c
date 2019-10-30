/* Plugin for Duiadns
 *
 * Copyright (C) 2015  Duiadns, Co. www.duiadns.net
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

#include "md5.h"
#include "plugin.h"

#define DUIADNS_UPDATE_IP_HTTP_REQUEST                   \
	"GET %s?"							\
	"host=%s&"							\
	"password=%s&"							\
	"ip4=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"User-Agent: %s\r\n"

#define MD5_DIGEST_BYTES  16

static int request  (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *alias);
static int response (http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias);

static ddns_system_t plugin = {
	.name         = "default@duiadns.net",

	.request      = (req_fn_t)request,
	.response     = (rsp_fn_t)response,

	.checkip_name = "ipv4.duiadns.net",
	.checkip_url  = "/",

	.server_name  = "ipv4.duiadns.net",
	.server_url   = "/dynamic.duia"
};

static int request(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *alias)
{
	int           i;
	char          digeststr[MD5_DIGEST_BYTES * 2 + 1] = {0};
	unsigned char digestbuf[MD5_DIGEST_BYTES] = {0};

	md5((unsigned char *)info->creds.password, strlen(info->creds.password), digestbuf);
	for (i = 0; i < MD5_DIGEST_BYTES; i++)
		sprintf(&digeststr[i * 2], "%02x", digestbuf[i]);

	return snprintf(ctx->request_buf, ctx->request_buflen,
			DUIADNS_UPDATE_IP_HTTP_REQUEST,
			info->server_url,
			alias->name,
			digeststr,
			alias->address_ipv4,
			info->server_name.name,
			info->user_agent);
}

static int response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias)
{
	char *resp = trans->rsp_body;

	DO(http_status_valid(trans->status));

	if (strstr(resp, "Hostname "))
		return RC_OK;

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

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */

