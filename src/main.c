/* Inadyn is a small and simple dynamic DNS (DDNS) client
 *
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
 * Copyright (C) 2010-2015  Joachim Nilsson <troglobit@gmail.com>
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

#include <getopt.h>
#include <stdlib.h>
#include <pwd.h>		/* getpwnam() */
#include <grp.h>		/* getgrnam() */

#include "debug.h"
#include "ddns.h"
#include "error.h"

int    debug = 0;
char  *config = NULL;
uid_t  uid = 0;
gid_t  gid = 0;
cfg_t *cfg;

static int alloc_context(ddns_t **pctx)
{
	int rc = 0;
	ddns_t *ctx;
	int http_to_dyndns_constructed = 0;
	int http_to_ip_constructed = 0;

	if (!pctx)
		return RC_INVALID_POINTER;

	*pctx = (ddns_t *)malloc(sizeof(ddns_t));
	if (!*pctx)
		return RC_OUT_OF_MEMORY;

	do {
		int i;

		ctx = *pctx;
		memset(ctx, 0, sizeof(ddns_t));

		/* Alloc space for http_to_ip_server data */
		ctx->work_buflen = DYNDNS_HTTP_RESPONSE_BUFFER_SIZE;
		ctx->work_buf = (char *)malloc(ctx->work_buflen);
		if (!ctx->work_buf) {
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		/* Alloc space for request data */
		ctx->request_buflen = DYNDNS_HTTP_REQUEST_BUFFER_SIZE;
		ctx->request_buf = (char *)malloc(ctx->request_buflen);
		if (!ctx->request_buf) {
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		i = 0;
		while (i < DYNDNS_MAX_SERVER_NUMBER) {
			if (http_construct(&ctx->http_to_ip_server[i++])) {
				rc = RC_OUT_OF_MEMORY;
				break;
			}
		}
		http_to_ip_constructed = 1;

		i = 0;
		while (i < DYNDNS_MAX_SERVER_NUMBER) {
			if (http_construct(&ctx->http_to_dyndns[i++])) {
				rc = RC_OUT_OF_MEMORY;
				break;
			}
		}
		http_to_dyndns_constructed = 1;

		ctx->cmd = NO_CMD;
		ctx->startup_delay_sec = DYNDNS_DEFAULT_STARTUP_SLEEP;
		ctx->normal_update_period_sec = DYNDNS_DEFAULT_SLEEP;
		ctx->sleep_sec = DYNDNS_DEFAULT_SLEEP;
		ctx->total_iterations = DYNDNS_DEFAULT_ITERATIONS;
		ctx->cmd_check_period = DYNDNS_DEFAULT_CMD_CHECK_PERIOD;
		ctx->force_addr_update = 0;

		i = 0;
		while (i < DYNDNS_MAX_SERVER_NUMBER)
			ctx->info[i++].creds.encoded_password = NULL;

		ctx->initialized = 0;
	}
	while (0);

	if (rc) {

		if (ctx->work_buf)
			free(ctx->work_buf);

		if (ctx->request_buf)
			free(ctx->request_buf);

		if (http_to_dyndns_constructed)
			http_destruct(ctx->http_to_dyndns, DYNDNS_MAX_SERVER_NUMBER);

		if (http_to_ip_constructed)
			http_destruct(ctx->http_to_ip_server, DYNDNS_MAX_SERVER_NUMBER);

		free(ctx);
		*pctx = NULL;
	}

	return 0;
}

static void free_context(ddns_t *ctx)
{
	int i;

	if (!ctx)
		return;

	http_destruct(ctx->http_to_ip_server, DYNDNS_MAX_SERVER_NUMBER);
	http_destruct(ctx->http_to_dyndns, DYNDNS_MAX_SERVER_NUMBER);

	if (ctx->work_buf) {
		free(ctx->work_buf);
		ctx->work_buf = NULL;
	}

	if (ctx->request_buf) {
		free(ctx->request_buf);
		ctx->request_buf = NULL;
	}

	for (i = 0; i < DYNDNS_MAX_SERVER_NUMBER; i++) {
		ddns_info_t *info = &ctx->info[i];

		if (info->creds.encoded_password) {
			free(info->creds.encoded_password);
			info->creds.encoded_password = NULL;
		}
	}

	if (ctx->external_command) {
		free(ctx->external_command);
		ctx->external_command = NULL;
	}

	if (ctx->bind_interface) {
		free(ctx->bind_interface);
		ctx->bind_interface = NULL;
	}

	if (ctx->check_interface) {
		free(ctx->check_interface);
		ctx->check_interface = NULL;
	}

	free(ctx);
}

/* XXX: Should be called from forked child ... */
static int drop_privs(void)
{
	if (uid) {
		if (uid != getuid()) {
			if (setuid(uid))
				return 1;
		}

		if (gid != getgid()) {
			if (setgid(gid))
				return 2;
		}

	}

	return 0;
}

static void parse_privs(char *user)
{
	char *group = strstr(user, ":");
	struct passwd *pw;

	if (group)
		*group++ = 0;

	pw = getpwnam(user);
	if (pw) {
		uid = pw->pw_uid;
		gid = pw->pw_gid;
	}

	if (group) {
		struct group *gr = getgrnam(group);

		if (gr)
			gid = gr->gr_gid;
	}
}

static int usage(char *progname)
{
	fprintf (stderr, "Inadyn | Small and simple DDNS client\n"
		 "------------------------------------------------------------------------------\n"
		 "Usage: %s [OPTIONS]\n"
		 " -f, --config=FILE               Use FILE for config, default %s\n"
		 " -d, --debug=LEVEL               Enable developer debug messages\n"
		 " -p, --drop-privs=USER[:GROUP]   Drop privileges after start to USER:GROUP\n"
		 " -n, --foreground                Run in foreground, useful when run from finit\n"
		 " -h, --help                      This help text\n"
		 " -L, --logfile=FILE              Log to FILE instead of syslog\n"
		 " -1, --once                      Run once, then exit regardless of status\n"
		 " -t, --startup-delay=SEC         Delay initial network connections SEC seconds\n"
		 " -l, --syslog                    Log to syslog, default unless --foreground\n"
		 " -V, --verbose                   Verbose logging\n"
		 " -v, --version                   Display program version\n"
		 "------------------------------------------------------------------------------\n"
		 "Report bugs to %s\n\n", progname, DEFAULT_CONFIG_FILE, PACKAGE_BUGREPORT);

	return 1;
}


int main(int argc, char *argv[])
{
	int c, rc = 0, restart;
	struct option opt[] = {
		{"config",        1, 0, 'f'},
		{"debug",         1, 0, 'd'},
		{"drop-privs",    1, 0, 'p'},
		{"foreground",    0, 0, 'n'},
		{"help",          0, 0, 'h'},
		{"logfile",       1, 0, 'L'},
		{"once",          0, 0, '1'},
		{"startup-delay", 1, 0, 't'},
		{"syslog",        0, 0, 'l'},
		{"verbose",       0, 0, 'V'},
		{"version",       0, 0, 'v'},
		{0,               0, 0, 0  }
	};
	ddns_t *ctx = NULL;

	while ((c = getopt_long(argc, argv, "f:d:p:nh?L:1t:lVv", opt, NULL)) != EOF) {
		switch (c) {
		case 'f':	/* --config=FILE */
			config = strdup(optarg);
			break;

		case 'd':	/* --debug=LEVEL */
			debug = atoi(optarg);
			break;

		case 'p':	/* --drop-privs=USER[:GROUP] */
			parse_privs(optarg);
			break;

		case 'n':	/* --foreground */
			foreground = 1;
			break;

		case 'L':	/* --logfile=FILE */
			logfile = strdup(optarg);
			break;

		case '1':	/* --once */
			once = 1;
			break;

		case 't':	/* --startup-delay=SEC */
			startup_delay = atoi(optarg);
			break;

		case 'l':	/* --syslog */
			syslog = 1;
			break;

		case 'V':	/* --verbose */
			verbose = 1;
			break;

		case 'v':
			puts(VERSION);
			return 0;

		case 'h':	/* --help */
		case ':':	/* Missing parameter for option. */
		case '?':	/* Unknown option. */
		default:
			return usage(argv[0]);
		}
	}

	if (os_install_signal_handler(ctx))
		return RC_OS_INSTALL_SIGHANDLER_FAILED;

	if (drop_privs()) {
		logit(LOG_WARNING, "Failed dropping privileges: %s", strerror(errno));
		return RC_OS_CHANGE_PERSONA_FAILURE;
	}

	if (!config)
		config = strdup(DEFAULT_CONFIG_FILE);

#ifdef ENABLE_SSL
	SSL_library_init();
	SSL_load_error_strings();
#endif

	do {
		restart = 0;

		cfg = parse_conf(config);
		if (!cfg)
			return RC_FILE_IO_MISSING_FILE;

		rc = alloc_context(&ctx);
		if (rc == RC_OK) {
			rc = ddns_main_loop(ctx, argc, argv);
			if (rc == RC_RESTART)
				restart = 1;

			free_context(ctx);
		}

		cfg_free(cfg);
	} while (restart);

	os_close_dbg_output();
	free(config);

#ifdef ENABLE_SSL
	ERR_free_strings();
#endif

	return rc;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
