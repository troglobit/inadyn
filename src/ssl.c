/* Interface for optional HTTPS functions
 *
 * Copyright (C) 2014  Joachim Nilsson <troglobit@gmail.com>
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

#include "debug.h"
#include "http.h"

int ssl_init(http_t *client, char *msg)
{
#ifndef ENABLE_SSL
	(void)client;
	(void)msg;
	return 0;
#else
	char *subject, *issuer;
#ifdef CONFIG_GNUTLS
	const
#endif
		X509 *cert;

	logit(LOG_INFO, "%s, initiating HTTPS ...", msg);

	client->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	if (!client->ssl_ctx)
		return RC_HTTPS_OUT_OF_MEMORY;

	client->ssl = SSL_new(client->ssl_ctx);
	if (!client->ssl)
		return RC_HTTPS_OUT_OF_MEMORY;

	SSL_set_fd(client->ssl, client->tcp.ip.socket);
	if (-1 == SSL_connect(client->ssl))
		return RC_HTTPS_FAILED_CONNECT;

	logit(LOG_INFO, "SSL connection using %s", SSL_get_cipher(client->ssl));

	/* Get server's certificate (note: beware of dynamic allocation) - opt */
	cert = SSL_get_peer_certificate(client->ssl);
	if (!cert)
		return RC_HTTPS_FAILED_GETTING_CERT;

	subject = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
	issuer  = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
	if (subject || issuer) {
		logit(LOG_INFO, "Server certificate -- subject: %s; issuer: %s", subject ?: "<NONE>", issuer ?: "<NONE>");

		if (subject)
			OPENSSL_free(subject);

		if (issuer)
			OPENSSL_free(issuer);
	}

	/* We could do all sorts of certificate verification stuff here before
	   deallocating the certificate. */
	X509_free(cert);

	return 0;
#endif
}

int ssl_exit(http_t *client)
{
#ifndef ENABLE_SSL
	(void)client;
	return 0;
#else
	/* SSL/TLS close_notify */
	SSL_shutdown(client->ssl);

	/* Clean up. */
	SSL_free(client->ssl);
	SSL_CTX_free(client->ssl_ctx);

	return 0;
#endif
}

int ssl_send(http_t *client, const char *buf, int len)
{
#ifndef ENABLE_SSL
	(void)client;
	(void)buf;
	(void)len;
	return RC_HTTPS_NO_SSL_SUPPORT;
#else
	int err = SSL_write(client->ssl, buf, len);

	if (err <= 0)
		/* XXX: TODO add SSL_get_error() to figure out why */
		return RC_HTTPS_SEND_ERROR;

	logit(LOG_DEBUG, "Successfully sent DDNS update using HTTPS!");
	return 0;
#endif
}

int ssl_recv(http_t *client, char *buf, int buf_len, int *recv_len)
{
#ifndef ENABLE_SSL
	(void)client;
	(void)buf;
	(void)buf_len;
	(void)recv_len;
	return RC_HTTPS_NO_SSL_SUPPORT;
#else
	int len, err;

	/* Read HTTP header */
	len = err = SSL_read(client->ssl, buf, buf_len);
	if (err <= 0)
		/* XXX: TODO add SSL_get_error() to figure out why */
		return RC_HTTPS_RECV_ERROR;

	/* Read HTTP body */
	*recv_len = SSL_read(client->ssl, &buf[len], buf_len - len);
	if (*recv_len <= 0)
		*recv_len = 0;
	*recv_len += len;
	logit(LOG_DEBUG, "Successfully received DDNS update response (%d bytes) using HTTPS!", *recv_len);

	return 0;
#endif
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
