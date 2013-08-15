/* Small cmd line program useful for maintaining an IP address in a Dynamic DNS system.
 *
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdlib.h>

#include "debug_if.h"
#include "dyndns.h"
#include "errorcode.h"

/**
   basic resource allocations for the dyn_dns object
*/
static int init_context(ddns_t **pctx)
{
	int rc = 0;
	ddns_t *ctx;
	int http_to_dyndns_constructed = 0;
	int http_to_ip_constructed = 0;
	int i;

	if (!pctx)
		return RC_INVALID_POINTER;

	*pctx = (ddns_t *)malloc(sizeof(ddns_t));
	if (!*pctx)
		return RC_OUT_OF_MEMORY;

	do
	{
		ctx = *pctx;
		memset(ctx, 0, sizeof(ddns_t));

		/* Alloc space for http_to_ip_server data */
		ctx->work_buflen = DYNDNS_HTTP_RESPONSE_BUFFER_SIZE;
		ctx->work_buf    = (char *)malloc(ctx->work_buflen);
		if (!ctx->work_buf)
		{
			rc = RC_OUT_OF_MEMORY;
			break;
   		}

		/* Alloc space for request data */
		ctx->request_buflen = DYNDNS_HTTP_REQUEST_BUFFER_SIZE;
		ctx->request_buf    = (char*)malloc(ctx->request_buflen);
		if (!ctx->request_buf)
		{
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		i = 0;
		while (i < DYNDNS_MAX_SERVER_NUMBER)
		{
			if (http_client_construct(&ctx->http_to_ip_server[i++]))
			{
				rc = RC_OUT_OF_MEMORY;
				break;
			}
		}
		http_to_ip_constructed = 1;

		i = 0;
		while (i < DYNDNS_MAX_SERVER_NUMBER)
		{
			if (http_client_construct(&ctx->http_to_dyndns[i++]))
			{
				rc = RC_OUT_OF_MEMORY;
				break;
			}
		}
		http_to_dyndns_constructed = 1;

		ctx->cmd = NO_CMD;
		ctx->normal_update_period_sec = DYNDNS_DEFAULT_SLEEP;
		ctx->sleep_sec = DYNDNS_DEFAULT_SLEEP;
		ctx->total_iterations = DYNDNS_DEFAULT_ITERATIONS;

		i = 0;
		while (i < DYNDNS_MAX_SERVER_NUMBER)
			ctx->info[i++].creds.encoded_password = NULL;

		ctx->initialized = 0;
	}
	while (0);

	if (rc)
	{

		if (ctx->work_buf)
			free(ctx->work_buf);

		if (ctx->request_buf)
			free(ctx->work_buf);

		if (http_to_dyndns_constructed)
			http_client_destruct(ctx->http_to_dyndns, DYNDNS_MAX_SERVER_NUMBER);

		if (http_to_ip_constructed)
			http_client_destruct(ctx->http_to_ip_server, DYNDNS_MAX_SERVER_NUMBER);

		free(ctx);
		*pctx = NULL;
	}

	return 0;
}


/**
   Resource free.
*/
static int free_context(ddns_t *ctx)
{
	int i;

	if (!ctx)
		return 0;

	if (ctx->initialized == 1)
		dyn_dns_shutdown(ctx);

	http_client_destruct(ctx->http_to_ip_server, DYNDNS_MAX_SERVER_NUMBER);
	http_client_destruct(ctx->http_to_dyndns, DYNDNS_MAX_SERVER_NUMBER);

	free(ctx->work_buf);
	ctx->work_buf = NULL;

	free(ctx->request_buf);
	ctx->request_buf = NULL;

	for (i = 0; i < DYNDNS_MAX_SERVER_NUMBER; i++)
	{
		ddns_info_t *info = &ctx->info[i];

		free(info->creds.encoded_password);
		info->creds.encoded_password = NULL;
	}

	free(ctx->cfgfile);
	ctx->cfgfile = NULL;

	free(ctx->pidfile);
	ctx->pidfile = NULL;

	free(ctx->external_command);
	ctx->external_command = NULL;

	free(ctx->bind_interface);
	ctx->bind_interface = NULL;

	free(ctx->check_interface);
	ctx->check_interface = NULL;

	free(ctx->cache_file);
	ctx->cache_file = NULL;

	free(ctx);

	return 0;
}

int main(int argc, char* argv[])
{
	int restart = 0;
	int os_handler_installed = 0;
	int rc = 0;
	ddns_t *ctx = NULL;

	do
	{
		rc = init_context(&ctx);
		if (rc != 0)
			break;

		if (!os_handler_installed)
		{
			rc = os_install_signal_handler(ctx);
			if (rc != 0)
			{
				logit(LOG_WARNING, "Failed installing OS signal handler: %s", errorcode_get_name(rc));
				break;
			}
			os_handler_installed = 1;
		}

		rc = dyn_dns_main(ctx, argc, argv);
		if (rc == RC_RESTART)
		{
			restart = 1;

			/* do some cleanup if restart requested */
			rc = free_context(ctx);
			if (rc != 0)
				logit(LOG_WARNING, "Failed cleaning up before restart: %s, ignoring...", errorcode_get_name(rc));
		}
		else
		{
			/* Error, or OK.  In either case exit outer loop. */
			restart = 0;
		}
	}
	while (restart);

	if (rc != 0)
		logit(LOG_WARNING, "Failed %sstarting daemon: %s", restart ? "re" : "", errorcode_get_name(rc));

	/* Cleanup */
	free_context(ctx);
	os_close_dbg_output();

	return (int)rc;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "ellemtel"
 *  c-basic-offset: 8
 * End:
 */
