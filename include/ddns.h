/* Interface for main DDNS functions
 *
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
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

#ifndef DDNS_H_
#define DDNS_H_

#include <lite/lite.h>
#include <lite/queue.h>

#include "config.h"
#include "os.h"
#include "error.h"
#include "http.h"
#include "debug.h"
#include "plugin.h"

#define VERSION_STRING	PACKAGE_NAME " version " VERSION " -- Dynamic DNS update client."
#define AGENT_NAME	"inadyn/" VERSION
#define SUPPORT_ADDR	PACKAGE_BUGREPORT

/* Test values */
/* Some default configurations */
#define DEFAULT_CONFIG_FILE	"/etc/inadyn.conf"
#define DEFAULT_CACHE_DIR       "/var/cache/inadyn/"

#define DYNDNS_MY_IP_SERVER	"checkip.dyndns.org"
#define DYNDNS_MY_CHECKIP_URL	"/"

#define DDNS_DEFAULT_STARTUP_SLEEP        0       /* sec */
#define DDNS_DEFAULT_PERIOD               120     /* sec */
#define DDNS_MIN_PERIOD                   30      /* sec */
#define DDNS_MAX_PERIOD                   (10 * 24 * 3600)        /* 10 days in sec */
#define DDNS_ERROR_UPDATE_PERIOD          600     /* 10 min */
#define DDNS_FORCED_UPDATE_PERIOD         (30 * 24 * 3600)        /* 30 days in sec */
#define DDNS_DEFAULT_CMD_CHECK_PERIOD     1       /* sec */
#define DDNS_DEFAULT_ITERATIONS           0       /* Forever */
#define DDNS_HTTP_RESPONSE_BUFFER_SIZE    BUFSIZ  /* 8 kiB */
#define DDNS_HTTP_REQUEST_BUFFER_SIZE     2500    /* Bytes */
#define DDNS_MAX_ALIAS_NUMBER             10      /* maximum number of aliases per server that can be maintained */
#define DDNS_MAX_SERVER_NUMBER            5       /* maximum number of servers that can be maintained */

/* SSL support status in plugin definition */
#define DDNS_CHECKIP_SSL_NOT_SUPPORTED    0       /* SSL not supported by checkip-server (default) */
#define DDNS_CHECKIP_SSL_SUPPORTED        1       /* SSL supported by checip-server */
#define DDNS_CHECKIP_SSL_REQUIRED         3       /* SSL required for checip-server */

/* local configs */
#define USERNAME_LEN                      50      /* chars */
#define PASSWORD_LEN                      50      /* chars */
#define SERVER_NAME_LEN                   256     /* chars */
#define SERVER_URL_LEN                    256     /* chars */
#ifdef INET6_ADDRSTRLEN
# define MAX_ADDRESS_LEN                  INET6_ADDRSTRLEN
#else
# define MAX_ADDRESS_LEN                  46
#endif

#define MAX_NUM_RESPONSES                 5
#define MAX_RESPONSE_LEN                  32

typedef enum {
	NO_CMD = 0,
	CMD_STOP,
	CMD_RESTART,
	CMD_FORCED_UPDATE,
	CMD_CHECK_NOW,
} ddns_cmd_t;

typedef struct {
	char           username[USERNAME_LEN];
	char           password[PASSWORD_LEN];
	char          *encoded_password;
	int            size;
	int            encoded;
} ddns_creds_t;

/* Server name and port */
typedef struct {
	char           name[SERVER_NAME_LEN];
	int            port;
} ddns_name_t;

typedef struct {
	int            ip_has_changed;
	char           address[MAX_ADDRESS_LEN];

	char           name[SERVER_NAME_LEN];
	int            update_required;
	time_t         last_update;
} ddns_alias_t;

typedef struct di {
	LIST_ENTRY(di) link;
	int            id;

	ddns_creds_t   creds;
	ddns_system_t *system;

	/* Address of DDNS update service */
	ddns_name_t    server_name;
	char           server_url[SERVER_URL_LEN];
	http_t         server;

	/* Possble "OK" responses from DDNS provider */
	char           server_response[MAX_NUM_RESPONSES][MAX_RESPONSE_LEN];
	size_t         server_response_num;

	/* Address of "What's my IP" checker */
	ddns_name_t    checkip_name;
	char           checkip_url[SERVER_URL_LEN];
	int            checkip_ssl; /* checkip server ssl mode */
	http_t         checkip;

	/* Shell command for "What's my IP" checker */
	char          *checkip_cmd;

	ddns_name_t    proxy_server_name;

	ddns_alias_t   alias[DDNS_MAX_ALIAS_NUMBER];
	size_t         alias_count;

	int            wildcard;

	int            ssl_enabled;
	int            append_myip; /* For custom setups! */
} ddns_info_t;

/* Client context */
typedef struct {
	char          *cfgfile;

	ddns_cmd_t     cmd;
	int            update_period; /* time between 2 updates */
	int            normal_update_period_sec;
	int            error_update_period_sec;
	int            forced_update_period_sec;
	int            forced_update_fake_addr;
	int            cmd_check_period; /*time to wait for a command */
	int            total_iterations;
	int            num_iterations;
	int            initialized;
	int            change_persona;
	int            force_addr_update;
	int            use_proxy;
	int            abort;

	http_trans_t   http_transaction;

	char          *work_buf; /* for HTTP responses */
	int            work_buflen;

	char          *request_buf; /* for HTTP requests */
	int            request_buflen;

	ddns_user_t    sys_usr_info; /* info about the current account running inadyn */
} ddns_t;

extern int once;
extern int loglevel;
extern int ignore_errors;
extern int startup_delay;
extern int use_syslog;
extern int allow_ipv6;
extern int verify_addr;
extern char *iface;
extern char *script_cmd;
extern char *script_exec;
extern char *pidfile_name;
extern char *generic_responses[];

int ddns_main_loop (ddns_t *ctx);

int common_request (ddns_t       *ctx,   ddns_info_t *info, ddns_alias_t *alias);
int common_response(http_trans_t *trans, ddns_info_t *info, ddns_alias_t *alias);

#endif /* DDNS_H_ */

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
