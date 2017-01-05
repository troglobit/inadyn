/* Interface for IP functions
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
 * Boston, MA 02110-1301, USA.
 */

#include <arpa/nameser.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "debug.h"
#include "ip.h"

int ip_construct(ip_sock_t *ip)
{
	ASSERT(ip);

	memset(ip, 0, sizeof(ip_sock_t));

	ip->initialized = 0;
	ip->socket      = -1; /* Initialize to 'error', not a possible socket id. */
	ip->timeout     = IP_DEFAULT_TIMEOUT;

	return 0;
}

/* Resource free. */
int ip_destruct(ip_sock_t *ip)
{
	ASSERT(ip);

	if (ip->initialized == 1)
		ip_exit(ip);

	return 0;
}

/* Check for socket error */
static int soerror(int sd)
{
	int code = 0;
	socklen_t len = sizeof(code);

	if (getsockopt(sd, SOL_SOCKET, SO_ERROR, &code, &len))
		return 1;

	errno = code;

	return code;
}

/* In the wonderful world of network programming the manual states that
 * EINPROGRESS is only a possible error on non-blocking sockets.  Real world
 * experience, however, suggests otherwise.  Simply poll() for completion and
 * then continue. --Joachim */
static int check_error(int sd, int msec)
{
	struct pollfd pfd = { sd, POLLOUT, 0 };

	if (EINPROGRESS == errno) {
		logit(LOG_INFO, "Waiting (%d sec) for three-way handshake to complete ...", msec / 1000);
		if (poll (&pfd, 1, msec) > 0 && !soerror(sd)) {
			logit(LOG_INFO, "Connected.");
			return 0;
		}
	}

	return 1;
}

static void set_timeouts(int sd, int timeout)
{
	struct timeval sv;

	memset(&sv, 0, sizeof(sv));
	sv.tv_sec  =  timeout / 1000;
	sv.tv_usec = (timeout % 1000) * 1000;
	if (-1 == setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &sv, sizeof(sv)))
		logit(LOG_INFO, "Failed setting receive timeout socket option: %s", strerror(errno));
	if (-1 == setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &sv, sizeof(sv)))
		logit(LOG_INFO, "Failed setting send timeout socket option: %s", strerror(errno));
}

/* Sets up the object. */
int ip_init(ip_sock_t *ip, char *msg)
{
	int rc = 0;

	ASSERT(ip);

	if (ip->initialized == 1)
		return 0;

	do {
		int s, sd, tries = 0;
		char port[10];
		char host[NI_MAXHOST];
		struct addrinfo hints, *servinfo, *ai;
		struct sockaddr *sa;
		socklen_t len;

		/* remote address */
		if (!ip->remote_host)
			break;

		/* Clear DNS cache before calling getaddrinfo(). */
		res_init();

		/* Obtain address(es) matching host/port */
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;	/* Use AF_UNSPEC to allow IPv4 or IPv6 */
		hints.ai_socktype = SOCK_DGRAM;	/* Datagram socket */
		snprintf(port, sizeof(port), "%d", ip->port);

		s = getaddrinfo(ip->remote_host, port, &hints, &servinfo);
		if (s != 0 || !servinfo) {
			logit(LOG_WARNING, "Failed resolving hostname %s: %s", ip->remote_host, gai_strerror(s));
			rc = RC_IP_INVALID_REMOTE_ADDR;
			break;
		}
		ai = servinfo;

		while (1) {
			sd = socket(AF_INET, SOCK_STREAM, 0);
			if (sd == -1) {
				logit(LOG_ERR, "Error creating client socket: %s", strerror(errno));
				rc = RC_IP_SOCKET_CREATE_ERROR;
				break;
			}

			set_timeouts(sd, ip->timeout);
			ip->socket = sd;
			ip->initialized = 1;

			/* Now we try connecting to the server, on connect fail, try next DNS record */
			sa  = ai->ai_addr;
			len = ai->ai_addrlen;

			if (getnameinfo(sa, len, host, sizeof(host), NULL, 0, NI_NUMERICHOST))
				goto next;

			logit(LOG_INFO, "%s, %sconnecting to %s(%s:%d)", msg, tries ? "re" : "", ip->remote_host, host, ip->port);
			if (connect(sd, sa, len)) {
				tries++;

				if (!check_error(sd, ip->timeout))
					break; /* OK */
			next:
				ai = ai->ai_next;
				if (ai) {
					logit(LOG_INFO, "Failed connecting to that server: %s",
					      errno != EINPROGRESS ? strerror(errno) : "retrying ...");

					close(sd);
					continue;
				}

				logit(LOG_WARNING, "Failed connecting to %s: %s", ip->remote_host, strerror(errno));
				rc = RC_IP_CONNECT_FAILED;
			}

			break;
		}

		freeaddrinfo(servinfo);
	}
	while (0);

	if (rc) {
		ip_exit(ip);
		return rc;
	}

	return 0;
}

/* Disconnect and some other clean up. */
int ip_exit(ip_sock_t *ip)
{
	ASSERT(ip);

	if (!ip->initialized)
		return 0;

	if (ip->socket > -1) {
		close(ip->socket);
		ip->socket = -1;
	}

	ip->initialized = 0;

	return 0;
}

int ip_send(ip_sock_t *ip, const char *buf, int len)
{
	ASSERT(ip);

	if (!ip->initialized)
		return RC_IP_OBJECT_NOT_INITIALIZED;

	if (send(ip->socket, buf, len, 0) == -1) {
		logit(LOG_WARNING, "Network error while sending query/update: %s", strerror(errno));
		return RC_IP_SEND_ERROR;
	}

	return 0;
}

/*
  Receive data into user's buffer.
  return
  if the max len has been received
  if a timeout occures
  In p_recv_len the total number of bytes are returned.
  Note:
  if the recv_len is bigger than 0, no error is returned.
*/
int ip_recv(ip_sock_t *ip, char *buf, int len, int *recv_len)
{
	int rc = 0;
	int remaining_bytes = len;
	int total_bytes = 0;

	ASSERT(ip);
	ASSERT(buf);
	ASSERT(recv_len);

	if (!ip->initialized)
		return RC_IP_OBJECT_NOT_INITIALIZED;

	while (remaining_bytes > 0) {
		int bytes;
		int chunk_size = remaining_bytes > IP_DEFAULT_READ_CHUNK_SIZE
			? IP_DEFAULT_READ_CHUNK_SIZE
			: remaining_bytes;

		bytes = recv(ip->socket, buf + total_bytes, chunk_size, 0);
		if (bytes < 0) {
			logit(LOG_WARNING, "Network error while waiting for reply: %s", strerror(errno));
			rc = RC_IP_RECV_ERROR;
			break;
		}

		if (bytes == 0) {
			if (total_bytes == 0)
				rc = RC_IP_RECV_ERROR;
			break;
		}

		total_bytes    += bytes;
		remaining_bytes = len - total_bytes;
	}

	*recv_len = total_bytes;

	return rc;
}

/* Accessors */
int ip_set_port(ip_sock_t *ip, int port)
{
	ASSERT(ip);

	if (port < 0 || port > IP_SOCKET_MAX_PORT)
		return RC_IP_BAD_PARAMETER;

	ip->port = port;

	return 0;
}

int ip_get_port(ip_sock_t *ip, int *port)
{
	ASSERT(ip);
	ASSERT(port);
	*port = ip->port;

	return 0;
}

int ip_set_remote_name(ip_sock_t *ip, const char *name)
{
	ASSERT(ip);
	ip->remote_host = name;

	return 0;
}

int ip_get_remote_name(ip_sock_t *ip, const char **name)
{
	ASSERT(ip);
	ASSERT(name);
	*name = ip->remote_host;

	return 0;
}

int ip_set_remote_timeout(ip_sock_t *ip, int timeout)
{
	ASSERT(ip);
	ip->timeout = timeout;

	return 0;
}

int ip_get_remote_timeout(ip_sock_t *ip, int *timeout)
{
	ASSERT(ip);
	ASSERT(timeout);
	*timeout = ip->timeout;

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
