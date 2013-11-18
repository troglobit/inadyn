/*
 * Copyright (C) 2003-2004  Narcis Ilisei
 * Copyright (C) 2006  Steve Horbachuk
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

#include "debug_if.h"

#include "os.h"
#include "dyndns.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "debug_if.h"

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

int os_syslog_open(const char *name)
{
	openlog(name, LOG_PID, LOG_USER);
	return 0;
}

int os_syslog_close(void)
{
	closelog();
	return 0;
}

int os_change_persona(ddns_user_t *user)
{
	int rc;

	do {
		if (user->gid != getgid()) {
			if ((rc = setgid(user->gid)) != 0)
				break;
		}

		if (user->uid != getuid()) {
			if ((rc = setuid(user->uid)) != 0)
				break;
		}
	}
	while (0);

	if (rc != 0) {
		logit(LOG_WARNING, "Failed dropping privileges: %s", strerror(errno));
		return RC_OS_CHANGE_PERSONA_FAILURE;
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
