/*
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

#include <string.h>
#include "http_client.h"
#include "errorcode.h"

#define super_construct(p) tcp_construct(p)
#define super_destruct(p)  tcp_destruct(p)
#define super_init(p,msg)  tcp_initialize(p, msg)
#define super_shutdown(p)  tcp_shutdown(p)


int http_client_construct(http_client_t *client)
{
	ASSERT(client);

	DO(super_construct(&client->super));

	memset((char *)client + sizeof(client->super), 0, sizeof(*client) - sizeof(client->super));
	client->initialized = 0;

	return 0;
}

/* Resource free. */
int http_client_destruct(http_client_t *client, int num)
{
	int i = 0, rv = 0;

	while (i < num)
		rv = super_destruct(&client[i++].super);

	return rv;
}

/* Set default TCP specififc params */
static int local_set_params(http_client_t *client)
{
	int timeout = 0;
	int port;

	http_client_get_remote_timeout(client, &timeout);
	if (timeout == 0)
		http_client_set_remote_timeout(client, HTTP_DEFAULT_TIMEOUT);

	http_client_get_port(client, &port);
	if (port == 0)
		http_client_set_port(client, HTTP_DEFAULT_PORT);

	return 0;
}

/* Sets up the object. */
int http_client_init(http_client_t *client, char *msg)
{
	int rc;

	do {
		TRY(local_set_params(client));
		TRY(super_init(&client->super, msg));
	}
	while (0);

	if (rc) {
		http_client_shutdown(client);
		return rc;
	}

	client->initialized = 1;

	return 0;
}

/* Disconnect and some other clean up. */
int http_client_shutdown(http_client_t *client)
{
	ASSERT(client);

	if (!client->initialized)
		return 0;

	client->initialized = 0;

	return super_shutdown(&client->super);
}

static void http_response_parse(http_trans_t *trans)
{
	char *body;
	char *rsp = trans->p_rsp_body = trans->p_rsp;
	int status = trans->status = 0;
	const char sep[] = "\r\n\r\n";

	memset(trans->status_desc, 0, sizeof(trans->status_desc));

	if (rsp != NULL && (body = strstr(rsp, sep)) != NULL) {
		body += strlen(sep);
		trans->p_rsp_body = body;
	}

	if (sscanf(trans->p_rsp, "HTTP/1.%*c %d %255[^\r\n]", &status, trans->status_desc) == 2)
		trans->status = status;
}

/* Send req and get response */
int http_client_transaction(http_client_t *client, http_trans_t *trans)
{
	int rc;

	ASSERT(client);
	ASSERT(trans);

	if (!client->initialized)
		return RC_HTTP_OBJECT_NOT_INITIALIZED;

	do {
		TRY(tcp_send(&client->super, trans->p_req, trans->req_len));
		TRY(tcp_recv(&client->super, trans->p_rsp, trans->max_rsp_len, &trans->rsp_len));
	}
	while (0);

	trans->p_rsp[trans->rsp_len] = 0;
	http_response_parse(trans);

	return rc;
}


int http_client_set_port(http_client_t *client, int port)
{
	ASSERT(client);
	return tcp_set_port(&client->super, port);
}

int http_client_get_port(http_client_t *client, int *port)
{
	ASSERT(client);
	return tcp_get_port(&client->super, port);
}


int http_client_set_remote_name(http_client_t *client, const char *name)
{
	ASSERT(client);
	return tcp_set_remote_name(&client->super, name);
}

int http_client_get_remote_name(http_client_t *client, const char **name)
{
	ASSERT(client);
	return tcp_get_remote_name(&client->super, name);
}

int http_client_set_remote_timeout(http_client_t *client, int timeout)
{
	ASSERT(client);
	return tcp_set_remote_timeout(&client->super, timeout);
}

int http_client_get_remote_timeout(http_client_t *client, int *timeout)
{
	ASSERT(client);
	return tcp_get_remote_timeout(&client->super, timeout);
}

int http_client_set_bind_iface(http_client_t *client, char *ifname)
{
	ASSERT(client);
	return tcp_set_bind_iface(&client->super, ifname);
}

int http_client_get_bind_iface(http_client_t *client, char **ifname)
{
	ASSERT(client);
	ASSERT(ifname);
	return tcp_get_bind_iface(&client->super, ifname);
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
