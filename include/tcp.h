/* Interface for TCP functions
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

#ifndef INADYN_TCP_H_
#define INADYN_TCP_H_

#include "os.h"
#include "errorcode.h"
#include "ip.h"

#define TCP_DEFAULT_TIMEOUT	20000	/* msec */

typedef struct {
	ip_sock_t super;
	int initialized;
} tcp_sock_t;

int tcp_construct(tcp_sock_t *p_self);
int tcp_destruct(tcp_sock_t *p_self);

int tcp_initialize(tcp_sock_t *p_self, char *msg);
int tcp_shutdown(tcp_sock_t *p_self);

int tcp_send(tcp_sock_t *p_self, const char *p_buf, int len);
int tcp_recv(tcp_sock_t *p_self, char *p_buf, int max_recv_len, int *p_recv_len);

int tcp_set_port(tcp_sock_t *p_self, int p);
int tcp_set_remote_name(tcp_sock_t *p_self, const char *p);
int tcp_set_remote_timeout(tcp_sock_t *p_self, int t);
int tcp_set_bind_iface(tcp_sock_t *p_self, char *ifname);

int tcp_get_port(tcp_sock_t *p_self, int *p_port);
int tcp_get_remote_name(tcp_sock_t *p_self, const char * *p);
int tcp_get_remote_timeout(tcp_sock_t *p_self, int *p);
int tcp_get_bind_iface(tcp_sock_t *p_self, char **ifname);

#endif /* INADYN_TCP_H_ */

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
