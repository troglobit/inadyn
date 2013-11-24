/* Interface for HTTP functions
 *
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
 * Copyright (C) 2010-2013  Joachim Nilsson <troglobit@gmail.com>
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

#ifndef INADYN_HTTP_CLIENT_H_
#define INADYN_HTTP_CLIENT_H_

#include "os.h"
#include "errorcode.h"
#include "tcp.h"

#define HTTP_DEFAULT_TIMEOUT	10000	/* msec */
#define	HTTP_DEFAULT_PORT	80

typedef struct {
	tcp_sock_t super;
	int        initialized;
} http_client_t;

typedef struct {
	char *p_req;
	int   req_len;

	char *p_rsp;
	int   max_rsp_len;
	int   rsp_len;

	char *p_rsp_body;

	int   status;
	char  status_desc[256];
} http_trans_t;

int http_client_construct          (http_client_t *client);
int http_client_destruct           (http_client_t *client, int num);

int http_client_initialize         (http_client_t *client, char *msg);
int http_client_shutdown           (http_client_t *client);

int http_client_transaction        (http_client_t *client, http_trans_t *trans);

int http_client_set_port           (http_client_t *client, int  porg);
int http_client_get_port           (http_client_t *client, int *port);

int http_client_set_remote_name    (http_client_t *client, const char  *name);
int http_client_get_remote_name    (http_client_t *client, const char **name);

int http_client_set_remote_timeout (http_client_t *client, int  timeout);
int http_client_get_remote_timeout (http_client_t *client, int *timeout);

int http_client_set_bind_iface     (http_client_t *client, char  *ifname);
int http_client_get_bind_iface     (http_client_t *client, char **ifname);

#endif /* INADYN_HTTP_CLIENT_H_ */

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
