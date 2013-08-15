/*
Copyright (C) 2003-2004 Narcis Ilisei

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* interface for tcp functions */

#ifndef _IP_H_INCLUDED
#define _IP_H_INCLUDED

#include "os.h"
#include "errorcode.h"

/* SOME DEFAULT CONFIGURATIONS */
#define IP_DEFAULT_TIMEOUT		20000 /* ms */
#define IP_SOCKET_MAX_PORT		65535
#define IP_DEFAULT_READ_CHUNK_SIZE	100

/* typedefs */
enum
{
	TYPE_TCP = 0,
	TYPE_UDP
};

typedef struct
{
	int initialized;

	char *ifname;
	int bound;             /* When bound to an interface */

	int type;
	int socket;
	struct sockaddr_in local_addr;
	struct sockaddr remote_addr;
	socklen_t remote_len;
	const char *p_remote_host_name;

	unsigned short port;
	int timeout;
} ip_sock_t;


/*public functions*/

/*
	 basic resource allocations for the  object
*/
int ip_construct(ip_sock_t *p_self);

/*
	Resource free .
*/
int ip_destruct(ip_sock_t *p_self);

/*
	Sets up the object.

	- ...
*/
int ip_initialize(ip_sock_t *p_self);

/*
	Disconnect and some other clean up.
*/
int ip_shutdown(ip_sock_t *p_self);

/* send data*/
int ip_send(ip_sock_t *p_self, const char *p_buf, int len);

/* receive data*/
int ip_recv(ip_sock_t *p_self, char *p_buf, int max_recv_len, int *p_recv_len);


/*Accessors */
int ip_set_port(ip_sock_t *p_self, int p);
int ip_set_remote_name(ip_sock_t *p_self, const char *p);
int ip_set_remote_timeout(ip_sock_t *p_self, int t);
int ip_set_bind_iface(ip_sock_t *p_self, char *ifname);

int ip_get_port(ip_sock_t *p_self, int *p_port);
int ip_get_remote_name(ip_sock_t *p_self, const char **p);
int ip_get_remote_timeout(ip_sock_t *p_self, int *p);
int ip_get_bind_iface(ip_sock_t *p_self, char **ifname);

#endif /*_IP_H_INCLUDED*/

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "ellemtel"
 *  c-basic-offset: 8
 * End:
 */
