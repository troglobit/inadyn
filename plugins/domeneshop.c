/* Plugin for domene.shop
 *
 * Copyright (C) 2024 Kenan Amundsen Elkoca <kenan@elkoca.com>
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

/* https://api.domeneshop.no/docs/#tag/ddns/paths/~1dyndns~1update/get */
#define DOMENESHOP_UPDATE_IP_REQUEST						\
	"GET %s?"							\
	"hostname=%s&"							\
	"myip=%s "							\
	"HTTP/1.1\r\n"							\
	"Host: %s\r\n"							\
	"Authorization: Basic %s\r\n"							\
	"User-Agent: %s\r\n\r\n"

static int request  (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *alias);
static int response (http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias);
static int check_response_code (int status);

static ddns_system_t domeneshop = {
	.name         = "default@domene.shop",

	.request      = (req_fn_t)request,
	.response     = (rsp_fn_t)response,

	.checkip_name = DYNDNS_MY_IP_SERVER,
	.checkip_url  = DYNDNS_MY_CHECKIP_URL,
	.checkip_ssl  = DYNDNS_MY_IP_SSL,

	.server_name  = "api.domeneshop.no",
	.server_url   = "/v0/dyndns/update"
};

static int request(ddns_t *ctx, ddns_info_t *info, ddns_alias_t *alias)
{
	return snprintf(ctx->request_buf, ctx->request_buflen,
			info->system->server_req,
			info->server_url,
			alias->name,
			alias->address,
			info->server_name.name,
			info->creds.encoded_password,
			info->user_agent);
}

static int check_response_code(int status)
{
    if (status == 204)
        return RC_OK;
        
    return http_status_valid(status);
}

static int response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias)
{
	int rc;
	char *body = trans->rsp_body;

	(void)info;
	(void)alias;

	rc = check_response_code(trans->status);

	if (rc == RC_OK && !strstr(body, ""))
		rc = RC_DDNS_RSP_NOTOK;

	return rc;
}

PLUGIN_INIT(plugin_init)
{
	plugin_register(&domeneshop, DOMENESHOP_UPDATE_IP_REQUEST);
}

PLUGIN_EXIT(plugin_exit)
{
	plugin_unregister(&domeneshop);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
