/* Interface for TCP functions
 *
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
 * Copyright (C) 2010-2014  Joachim Nilsson <troglobit@gmail.com>
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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "tcp.h"

int tcp_construct(tcp_sock_t *tcp)
{
	ASSERT(tcp);

	DO(ip_construct(&tcp->ip));

	/* Reset its part of the struct (skip IP part) */
	memset(((char *)tcp + sizeof(tcp->ip)), 0, sizeof(*tcp) - sizeof(tcp->ip));
	tcp->initialized = 0;

	return 0;
}

int tcp_destruct(tcp_sock_t *tcp)
{
	ASSERT(tcp);

	if (tcp->initialized == 1)
		tcp_exit(tcp);

	return ip_destruct(&tcp->ip);
}

static int local_set_params(tcp_sock_t *tcp)
{
	int timeout;

	/* Set default TCP specififc params */
	tcp_get_remote_timeout(tcp, &timeout);
	if (timeout == 0)
		tcp_set_remote_timeout(tcp, TCP_DEFAULT_TIMEOUT);

	return 0;
}

/* On error tcp_exit() is called by upper layers. */
int tcp_init(tcp_sock_t *tcp, char *msg)
{
	int rc = 0;

	ASSERT(tcp);

	do {
		TRY(local_set_params(tcp));
		TRY(ip_init(&tcp->ip, msg));
		tcp->initialized  = 1;
	} while (0);

	if (rc) {
		tcp_exit(tcp);
		return rc;
	}

	return 0;
}

int tcp_exit(tcp_sock_t *tcp)
{
	ASSERT(tcp);

	if (!tcp->initialized)
		return 0;

	tcp->initialized = 0;

	return ip_exit(&tcp->ip);
}

int tcp_send(tcp_sock_t *tcp, const char *buf, int len)
{
	ASSERT(tcp);

	if (!tcp->initialized)
		return RC_TCP_OBJECT_NOT_INITIALIZED;

	return ip_send(&tcp->ip, buf, len);
}

int tcp_recv(tcp_sock_t *tcp, char *buf, int buf_len, int *recv_len)
{
	ASSERT(tcp);

	if (!tcp->initialized)
		return RC_TCP_OBJECT_NOT_INITIALIZED;

	return ip_recv(&tcp->ip, buf, buf_len, recv_len);
}

int tcp_set_port(tcp_sock_t *tcp, int port)
{
	ASSERT(tcp);

	return ip_set_port(&tcp->ip, port);
}

int tcp_get_port(tcp_sock_t *tcp, int *port)
{
	ASSERT(tcp);

	return ip_get_port(&tcp->ip, port);
}

int tcp_set_remote_name(tcp_sock_t *tcp, const char *p)
{
	ASSERT(tcp);

	return ip_set_remote_name(&tcp->ip, p);
}

int tcp_get_remote_name(tcp_sock_t *tcp, const char **p)
{
	ASSERT(tcp);

	return ip_get_remote_name(&tcp->ip, p);
}

int tcp_set_remote_timeout(tcp_sock_t *tcp, int p)
{
	ASSERT(tcp);

	return ip_set_remote_timeout(&tcp->ip, p);
}

int tcp_get_remote_timeout(tcp_sock_t *tcp, int *p)
{
	ASSERT(tcp);

	return ip_get_remote_timeout(&tcp->ip, p);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
