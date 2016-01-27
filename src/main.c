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
#include <confuse.h>

#include "debug.h"
#include "ddns.h"
#include "error.h"

int    once = 0;
int    loglevel = LOG_NOTICE;
int    foreground = 0;
int    ignore_errors = 0;
int    startup_delay = DDNS_DEFAULT_STARTUP_SLEEP;
int    use_syslog = 0;
char  *config = NULL;
char  *cache_dir = NULL;
char  *script_exec = NULL;
uid_t  uid = 0;
gid_t  gid = 0;
cfg_t *cfg;

extern cfg_t *conf_parse_file(char *file, ddns_t *ctx);
extern void conf_info_cleanup(void);


static int alloc_context(ddns_t **pctx)
{
	int rc = 0;
	ddns_t *ctx;

	if (!pctx)
		return RC_INVALID_POINTER;

	*pctx = (ddns_t *)malloc(sizeof(ddns_t));
	if (!*pctx)
		return RC_OUT_OF_MEMORY;

	do {
		ctx = *pctx;
		memset(ctx, 0, sizeof(ddns_t));

		/* Alloc space for http_to_ip_server data */
		ctx->work_buflen = DDNS_HTTP_RESPONSE_BUFFER_SIZE;
		ctx->work_buf = (char *)malloc(ctx->work_buflen);
		if (!ctx->work_buf) {
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		/* Alloc space for request data */
		ctx->request_buflen = DDNS_HTTP_REQUEST_BUFFER_SIZE;
		ctx->request_buf = (char *)malloc(ctx->request_buflen);
		if (!ctx->request_buf) {
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		ctx->cmd = NO_CMD;
		ctx->normal_update_period_sec = DDNS_DEFAULT_PERIOD;
		ctx->update_period = DDNS_DEFAULT_PERIOD;
		ctx->total_iterations = DDNS_DEFAULT_ITERATIONS;
		ctx->cmd_check_period = DDNS_DEFAULT_CMD_CHECK_PERIOD;
		ctx->force_addr_update = 0;

		ctx->initialized = 0;
	}
	while (0);

	if (rc) {

		if (ctx->work_buf)
			free(ctx->work_buf);

		if (ctx->request_buf)
			free(ctx->request_buf);

		free(ctx);
		*pctx = NULL;
	}

	return 0;
}

static void free_context(ddns_t *ctx)
{
	if (!ctx)
		return;

	if (ctx->work_buf) {
		free(ctx->work_buf);
		ctx->work_buf = NULL;
	}

	if (ctx->request_buf) {
		free(ctx->request_buf);
		ctx->request_buf = NULL;
	}

	conf_info_cleanup();

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
		if (gid != getgid()) {
			if (setgid(gid))
				return 2;
		}

		if (uid != getuid()) {
			if (setuid(uid))
				return 1;
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

static int usage(int code)
{
	fprintf (stderr, "\nUsage: %s [1chnsv] [-e CMD] [-f FILE] [-l LVL] [-p USR:GRP] [-t SEC]\n\n"
		 " -1, --once                     Run once, then exit regardless of status\n"
		 " -c, --continue-on-error        Ignore errors from DDNS provider (DO NOT USE)\n"
		 " -e, --exec=/path/to/cmd        Script to run on IP update\n"
		 " -f, --config=FILE              Use FILE for config, default %s\n"
		 " -h, --help                     Show summary of command line options and exit\n"
		 " -l, --loglevel=LEVEL           Set log level: none, err, info, notice*, debug\n"
		 " -n, --foreground               Run in foreground, useful when run from finit\n"
		 " -p, --drop-privs=USER[:GROUP]  Drop privileges after start to USER:GROUP\n"
		 " -s, --syslog                   Log to syslog, default unless --foreground\n"
		 " -t, --startup-delay=SEC        Initial startup delay, default none\n"
		 " -v, --version                  Show program version and exit\n\n"
		 "Bug report address: %s\n\n", __progname, DEFAULT_CONFIG_FILE, PACKAGE_BUGREPORT);

	return code;
}


int main(int argc, char *argv[])
{
	int c, rc = 0, restart;
	struct option opt[] = {
		{ "once",              0, 0, '1' },
		{ "continue-on-error", 0, 0, 'c' },
		{ "exec",              1, 0, 'e' },
		{ "config",            1, 0, 'f' },
		{ "loglevel",          1, 0, 'l' },
		{ "help",              0, 0, 'h' },
		{ "foreground",        0, 0, 'n' },
		{ "drop-privs",        1, 0, 'p' },
		{ "syslog",            0, 0, 's' },
		{ "startup-delay",     1, 0, 't' },
		{ "version",           0, 0, 'v' },
		{ NULL,                0, 0, 0   }
	};
	ddns_t *ctx = NULL;

	while ((c = getopt_long(argc, argv, "1ce:f:h?np:st:v", opt, NULL)) != EOF) {
		switch (c) {
		case '1':	/* --once */
			once = 1;
			break;

		case 'c':	/* --continue-on-error */
			ignore_errors = 1;
			break;

		case 'e':	/* --exec=CMD */
			script_exec = strdup(optarg);
			break;

		case 'f':	/* --config=FILE */
			config = strdup(optarg);
			break;

		case 'l':	/* --loglevel=LEVEL */
			loglevel = loglvl(optarg);
			if (-1 == loglevel)
				return usage(1);
			break;

		case 'n':	/* --foreground */
			foreground = 1;
			break;

		case 'p':	/* --drop-privs=USER[:GROUP] */
			parse_privs(optarg);
			break;

		case 's':	/* --syslog */
			use_syslog = 1;
			break;

		case 't':	/* --startup-delay=SEC */
			startup_delay = atoi(optarg);
			break;

		case 'v':
			puts(VERSION);
			return 0;

		case 'h':	/* --help */
		case ':':	/* Missing parameter for option. */
		case '?':	/* Unknown option. */
		default:
			return usage(0);
		}
	}

	/* if silent required, fork() to background and close console window */
	if (use_syslog || !foreground) {
		DO(close_console_window());
		openlog(NULL, LOG_PID, LOG_USER);
		setlogmask(LOG_UPTO(loglevel));
	}

	if (drop_privs()) {
		logit(LOG_WARNING, "Failed dropping privileges: %s", strerror(errno));
		return RC_OS_CHANGE_PERSONA_FAILURE;
	}

	if (!config)
		config = strdup(DEFAULT_CONFIG_FILE);

	/* "Hello!" Let user know we've started up OK */
	logit(LOG_INFO, "%s", VERSION_STRING);

#ifdef ENABLE_SSL
	SSL_library_init();
	SSL_load_error_strings();
#endif

	do {
		restart = 0;

		rc = alloc_context(&ctx);
		if (rc != RC_OK)
			break;

		if (os_install_signal_handler(ctx))
			return RC_OS_INSTALL_SIGHANDLER_FAILED;

		cfg = conf_parse_file(config, ctx);
		if (!cfg) {
			free_context(ctx);
			return RC_FILE_IO_MISSING_FILE;
		}

		rc = ddns_main_loop(ctx);
		if (rc == RC_RESTART)
			restart = 1;

		free_context(ctx);
		cfg_free(cfg);
	} while (restart);

	if (use_syslog)
		closelog();
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
