/* OpenSSL interface for optional HTTPS functions
 *
 * Copyright (C) 2014-2017  Joachim Nilsson <troglobit@gmail.com>
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

#include "log.h"
#include "http.h"
#include "ssl.h"

int ssl_init(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
#endif
	
	return 0;
}

void ssl_exit(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	ERR_free_strings();
	EVP_cleanup();
#endif
}

static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	char    buf[256];
	X509   *cert;
	int     err, depth;

	cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));

	/*
	 * Catch a too long certificate chain. The depth limit set using
	 * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
	 * that whenever the "depth>verify_depth" condition is met, we
	 * have violated the limit and want to log this error condition.
	 * We must do it here, because the CHAIN_TOO_LONG error would not
	 * be found explicitly; only errors introduced by cutting off the
	 * additional certificates would be logged.
	 */
	if (depth > 100) {
		preverify_ok = 0;
		err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
		X509_STORE_CTX_set_error(ctx, err);
	}

	if (!preverify_ok)
		logit(LOG_ERR, "Certificate verification error:num=%d:%s:depth=%d:%s",
		      err, X509_verify_cert_error_string(err), depth, buf);

	/*
	 * At this point, err contains the last verification error. We can use
	 * it for something special
	 */
	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
		X509_NAME_oneline(X509_get_issuer_name(cert), buf, sizeof(buf));
		logit(LOG_ERR, "issuer= %s", buf);
	}

	if (secure_ssl)
		return preverify_ok;

	return 1;
}

static int ssl_set_ca_location(http_t *client)
{
	int ret;

	/* A user defined CA PEM bundle overrides any built-ins or fall-backs */
	if (ca_trust_file) {
		ret = SSL_CTX_load_verify_locations(client->ssl_ctx, ca_trust_file, NULL);
		goto done;
	}

	ret = SSL_CTX_set_default_verify_paths(client->ssl_ctx);
	if (ret < 1)
		ret = SSL_CTX_load_verify_locations(client->ssl_ctx, CAFILE1, NULL);
	if (ret < 1)
		ret = SSL_CTX_load_verify_locations(client->ssl_ctx, CAFILE2, NULL);
done:
	if (ret < 1)
		return 1;

	return 0;
}

static int ssl_error_cb(const char *str, size_t len, void *data)
{
	size_t sz;
	char buf[512];

	memset(buf, 0, sizeof(buf));
	sz = len < sizeof(buf) ? len : sizeof(buf) - 1;
	memcpy(buf, str, sz);

	logit(LOG_ERR, "OpenSSL error: %s", buf);

	return 0;
}

static void ssl_check_error(void)
{
	ERR_print_errors_cb(ssl_error_cb, NULL);
}

int ssl_open(http_t *client, char *msg)
{
	const char *sn;
	char buf[512];
	X509 *cert;
	int rc;

	if (!client->ssl_enabled)
		return tcp_init(&client->tcp, msg);

	tcp_set_port(&client->tcp, HTTPS_DEFAULT_PORT);
	DO(tcp_init(&client->tcp, msg));

	logit(LOG_INFO, "%s, initiating HTTPS ...", msg);
	client->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	if (!client->ssl_ctx)
		return RC_HTTPS_OUT_OF_MEMORY;

	/* POODLE, only allow TLSv1.x or later */
	SSL_CTX_set_options(client->ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);

	SSL_CTX_set_verify(client->ssl_ctx, SSL_VERIFY_PEER, verify_callback);
	SSL_CTX_set_verify_depth(client->ssl_ctx, 150);

	/* Try to figure out location of trusted CA certs on system */
	if (ssl_set_ca_location(client))
		return RC_HTTPS_NO_TRUSTED_CA_STORE;

	client->ssl = SSL_new(client->ssl_ctx);
	if (!client->ssl)
		return RC_HTTPS_OUT_OF_MEMORY;

	/* SSL SNI support: tell the servername we want to speak to */
	http_get_remote_name(client, &sn);
	if (!SSL_set_tlsext_host_name(client->ssl, sn))
		return RC_HTTPS_SNI_ERROR;

	SSL_set_fd(client->ssl, client->tcp.socket);
	rc = SSL_connect(client->ssl);
	if (rc < 0) {
		ssl_check_error();
		return RC_HTTPS_FAILED_CONNECT;
	}

	logit(LOG_INFO, "SSL connection using %s", SSL_get_cipher(client->ssl));

	cert = SSL_get_peer_certificate(client->ssl);
	if (!cert)
		return RC_HTTPS_FAILED_GETTING_CERT;

	if (SSL_get_verify_result(client->ssl) == X509_V_OK)
		logit(LOG_DEBUG, "Certificate OK");

	X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));
	logit(LOG_INFO, "SSL server cert subject: %s", buf);
	X509_NAME_oneline(X509_get_issuer_name(cert), buf, sizeof(buf));
	logit(LOG_INFO, "SSL server cert issuer: %s", buf);

	X509_free(cert);

	return 0;
}

int ssl_close(http_t *client)
{
	if (client->ssl_enabled) {
		if (client->ssl) {
			/* SSL/TLS close_notify */
			SSL_shutdown(client->ssl);

			/* Clean up. */
			SSL_free(client->ssl);
			client->ssl = NULL;
		}
		if (client->ssl_ctx) {
			SSL_CTX_free(client->ssl_ctx);
			client->ssl_ctx = NULL;
		}
	}

	return tcp_exit(&client->tcp);
}

int ssl_send(http_t *client, const char *buf, int len)
{
	int rc;

	if (!client->ssl_enabled)
		return tcp_send(&client->tcp, buf, len);

	do {
	rc = SSL_write(client->ssl, buf, len);
	} while (rc == SSL_ERROR_WANT_WRITE);	
	if (rc <= 0) {
		ssl_check_error();
		return RC_HTTPS_SEND_ERROR;
	}

	logit(LOG_DEBUG, "Successfully sent HTTPS request!");

	return 0;
}

int ssl_recv(http_t *client, char *buf, int buf_len, int *recv_len)
{
	int len, rc;

	*recv_len = 0;
	if (!client->ssl_enabled)
		return tcp_recv(&client->tcp, buf, buf_len, recv_len);

	/* Read HTTP header */
	do {
	len = rc = SSL_read(client->ssl, buf, buf_len);
	} while (rc == SSL_ERROR_WANT_READ);
	if (rc <= 0) {
		ssl_check_error();
		return RC_HTTPS_RECV_ERROR;
	}
	*recv_len += len;

	/* Read HTTP body */
	do {
	len = rc = SSL_read(client->ssl, &buf[len], buf_len - len);
	} while (rc == SSL_ERROR_WANT_READ);
	if (rc <= 0) {
		ssl_check_error();
		len = 0;
	}
	*recv_len += len;
	logit(LOG_DEBUG, "Successfully received HTTPS response (%d bytes)!", *recv_len);

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
