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

#ifndef _TCP_H_INCLUDED
#define _TCP_H_INCLUDED

#include "os.h"
#include "errorcode.h"
#include "ip.h"


/* SOME DEFAULT CONFIGURATIONS */
#define TCP_DEFAULT_TIMEOUT	20000 /*ms*/


typedef struct
{
	ip_sock_t super;
	int initialized;
} tcp_sock_t;

/*
	 basic resource allocations for the tcp object
*/
int tcp_construct(tcp_sock_t *p_self);

/*
	Resource free.
*/
int tcp_destruct(tcp_sock_t *p_self);

/*
	Sets up the object.

	- ...
*/
int tcp_initialize(tcp_sock_t *p_self, char *msg);

/*
	Disconnect and some other clean up.
*/
int tcp_shutdown(tcp_sock_t *p_self);


/* send data*/
int tcp_send(tcp_sock_t *p_self, const char *p_buf, int len);

/* receive data*/
int tcp_recv(tcp_sock_t *p_self, char *p_buf, int max_recv_len, int *p_recv_len);

/* Accessors */

int tcp_set_port(tcp_sock_t *p_self, int p);
int tcp_set_remote_name(tcp_sock_t *p_self, const char* p);
int tcp_set_remote_timeout(tcp_sock_t *p_self, int t);
int tcp_set_bind_iface(tcp_sock_t *p_self, char *ifname);

int tcp_get_port(tcp_sock_t *p_self, int *p_port);
int tcp_get_remote_name(tcp_sock_t *p_self, const char* *p);
int tcp_get_remote_timeout(tcp_sock_t *p_self, int *p);
int tcp_get_bind_iface(tcp_sock_t *p_self, char **ifname);


#endif /*_TCP_H_INCLUDED*/
