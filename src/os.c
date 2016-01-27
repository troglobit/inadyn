/*
 * Copyright (C) 2003-2004  Narcis Ilisei
 * Copyright (C) 2006       Steve Horbachuk
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

#include <libgen.h>		/* dirname() */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "os.h"
#include "debug.h"
#include "ddns.h"
#include "cache.h"

#define MAXSTRING 1024

/**
    The dbg destination.
    DBG_SYS_LOG for SysLog
    DBG_STD_LOG for standard console
*/
static int global_mod_dbg_dest = DBG_STD_LOG;

/**
    Returns the dbg destination.
    DBG_SYS_LOG for SysLog
    DBG_STD_LOG for standard console
*/
int get_dbg_dest(void)
{
	return global_mod_dbg_dest;
}

void set_dbg_dest(int dest)
{
	global_mod_dbg_dest = dest;
}

static char *current_time(void)
{
	time_t now;
	struct tm *timeptr;
	static const char wday_name[7][3] = {
		"Sun", "Mon", "Tue", "Wed",
		"Thu", "Fri", "Sat"
	};
	static const char mon_name[12][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char result[26];

	time(&now);
	timeptr = localtime(&now);

	sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %d:",
		wday_name[timeptr->tm_wday], mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, 1900 + timeptr->tm_year);

	return result;
}

void os_printf(int prio, char *fmt, ...)
{
	size_t len;
	va_list args;
	char message[MAXSTRING + 1] = "";

	message[MAXSTRING] = 0;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);

	len = strlen(message);
	if (message[len - 1] == '\n')
		message[len - 1] = 0;

	if (get_dbg_dest() == DBG_SYS_LOG) {
		syslog(prio, "%s", message);
		return;
	}

	printf("%s %s\n", current_time(), message);
	fflush(stdout);
}

/* storage for the parameter needed by the handler */
static void *param = NULL;

void os_sleep_ms(int ms)
{
	usleep(ms * 1000);
}

int os_get_socket_error(void)
{
	return errno;
}

int os_ip_support_startup(void)
{
	return 0;
}

int os_ip_support_cleanup(void)
{
	return 0;
}

int os_shell_execute(char *cmd, char *ip, char *hostname, char *iface)
{
	int rc = 0;
	int child;

	child = vfork();
	switch (child) {
	case 0:		/* child */
		setenv("INADYN_IP", ip, 1);
		setenv("INADYN_HOSTNAME", hostname, 1);
		if (iface)
			setenv("INADYN_IFACE", iface, 1);
		execl("/bin/sh", "sh", "-c", cmd, (char *)0);
		exit(1);
		break;

	case -1:
		rc = RC_OS_FORK_FAILURE;
		break;

	default:		/* parent */
		break;
	}

	return rc;
}

/**
 * unix_signal_handler - Signal handler
 * @signo: Signal number
 *
 * Handler for registered/known signals. Most others will terminate the
 * daemon.
 *
 * NOTE:
 * Since printf() is one of the possible back-ends of logit(), and
 * printf() is not one of the safe syscalls to be used, according to
 * POSIX signal(7). The calls are commented, since they are most likely
 * also only needed for debugging.
 */
static void unix_signal_handler(int signo)
{
	ddns_t *ctx = (ddns_t *)param;

	if (ctx == NULL)
		return;

	switch (signo) {
	case SIGHUP:
		ctx->cmd = CMD_RESTART;
		break;

	case SIGINT:
	case SIGTERM:
		ctx->cmd = CMD_STOP;
		break;

	case SIGUSR1:
		ctx->cmd = CMD_FORCED_UPDATE;
		break;

	case SIGUSR2:
		ctx->cmd = CMD_CHECK_NOW;
		break;

	default:
		break;
	}
}

/**
 * Install signal handler for signals HUP, INT, TERM and USR1
 *
 * Also block exactly the handled signals, only for the duration
 * of the handler.  All other signals are left alone.
 */
int os_install_signal_handler(void *ctx)
{
	int rc = 0;
	static int installed = 0;
	struct sigaction sa;

	if (!installed) {
		sa.sa_flags   = 0;
		sa.sa_handler = unix_signal_handler;

		rc = sigemptyset(&sa.sa_mask) ||
			sigaddset(&sa.sa_mask, SIGHUP)  ||
			sigaddset(&sa.sa_mask, SIGINT)  ||
			sigaddset(&sa.sa_mask, SIGTERM) ||
			sigaddset(&sa.sa_mask, SIGUSR1) ||
			sigaddset(&sa.sa_mask, SIGUSR2) ||
			sigaction(SIGHUP, &sa, NULL)    ||
			sigaction(SIGINT, &sa, NULL)    ||
			sigaction(SIGUSR1, &sa, NULL)   ||
			sigaction(SIGUSR2, &sa, NULL)   ||
			sigaction(SIGTERM, &sa, NULL);

		installed = 1;
	}

	if (rc) {
		logit(LOG_WARNING, "Failed installing signal handler: %s", errorcode_get_name(rc));
		return RC_OS_INSTALL_SIGHANDLER_FAILED;
	}

	param = ctx;
	return 0;
}

/*
    closes current console

  July 5th, 2004 - Krev
  ***
  This function is used to close the console window to start the
  software as a service on Windows. On Unix, closing the console window
  isn't used for a daemon, but rather it forks. Modified this function
  to fork into a daemon mode under Unix-compatible systems.

  Actions:
    - for child:
	- close in and err console
	- become session leader
	- change working directory
	- clear the file mode creation mask
    - for parent
	just exit
*/
int close_console_window(void)
{
	pid_t pid = fork();

	if (pid < 0) {
		return RC_OS_FORK_FAILURE;
	}

	if (pid == 0) {		/* child */
		fclose(stdin);
		fclose(stderr);
		setsid();

		if (-1 == chdir("/"))
			logit(LOG_WARNING, "Failed changing cwd to /: %s", strerror(errno));

		umask(0);

		return 0;
	}

	exit(0);

	return 0;		/* Never reached. */
}

/*
 * Check file system permissions
 *
 * Create pid and cache file repository, make sure we can write to it.  If
 * we are restarted we cannot otherwise make sure we've not already updated
 * the IP -- and the user will be locked-out of their DDNS server provider
 * for excessive updates.
 */
int os_check_perms(void *UNUSED(arg))
{
	/* Create files with permissions 0644 */
	umask(S_IWGRP | S_IWOTH);

	if (mkpath(cache_dir, 0755) && errno != EEXIST || access(cache_dir, W_OK)) {
		logit(LOG_ERR, "No write permission to %s, aborting.", cache_dir);
		logit(LOG_ERR, "Cannot guarantee DDNS server won't lock you out for excessive updates.");

		return RC_FILE_IO_ACCESS_ERROR;
	}

	return 0;
}


/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
