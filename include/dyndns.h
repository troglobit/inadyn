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

#ifndef DYNDNS_H_
#define DYNDNS_H_

#include "os.h"
#include "errorcode.h"
#include "http.h"
#include "debug_if.h"

#define VERSION_STRING	"Inadyn version " VERSION " -- Dynamic DNS update client."
#define AGENT_NAME	"inadyn/" VERSION
#define SUPPORT_ADDR	"troglobit@gmail.com"

/* DDNS system ID's */
enum {
	DYNDNS_DEFAULT = 0,
	FREEDNS_AFRAID_ORG_DEFAULT,
	ZONE_EDIT_DEFAULT,
	CUSTOM_HTTP_BASIC_AUTH,
	NOIP_DEFAULT,
	EASYDNS_DEFAULT,
	TZO_DEFAULT,
	DYNDNS_3322_DYNAMIC,
	SITELUTIONS_DOMAIN,
	DNSOMATIC_DEFAULT,
	DNSEXIT_DEFAULT,
	HE_IPV6TB,
	HE_DYNDNS,
	CHANGEIP_DEFAULT,
	DYNSIP_DEFAULT,
	LAST_DNS_SYSTEM = -1
};

/* Test values */
#define DEFAULT_CONFIG_FILE	"/etc/inadyn.conf"
#define RUNTIME_DATA_DIR	"/var/run/inadyn"
#define DEFAULT_CACHE_FILE	RUNTIME_DATA_DIR "/inadyn.cache"
#define CACHE_FILE		RUNTIME_DATA_DIR "/%s.cache"
#define DEFAULT_PIDFILE		RUNTIME_DATA_DIR "/inadyn.pid"

#define DYNDNS_MY_IP_SERVER	"checkip.dyndns.org"
#define DYNDNS_MY_CHECKIP_URL	"/"

/* REQ/RSP definitions */

#define DEFAULT_DNS_SYSTEM	DYNDNS_DEFAULT

/* Conversation with the IP server */
#define DYNDNS_GET_IP_HTTP_REQUEST  					\
	"GET %s HTTP/1.0\r\n"						\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

#define GENERIC_HTTP_REQUEST                                      	\
	"GET %s HTTP/1.0\r\n"						\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

#define GENERIC_AUTH_HTTP_REQUEST					\
	"GET %s HTTP/1.0\r\n"						\
	"Host: %s\r\n"							\
	"Authorization: Basic %s\r\n"					\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/* dyndns.org specific update address format
 * 3322.org has the same parameters ... */
#define DYNDNS_UPDATE_IP_HTTP_REQUEST					\
	"GET %s?"							\
	"hostname=%s&"							\
	"myip=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"Authorization: Basic %s\r\n"					\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/* freedns.afraid.org specific update request format */
#define FREEDNS_UPDATE_IP_REQUEST					\
	"GET %s?"							\
	"%s&"								\
	"address=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/** generic update format for sites that perform the update
	with:
	http://some.address.domain/somesubdir
	?some_param_name=ALIAS
	and then the normal http stuff and basic base64 encoded auth.
	The parameter here is the entire request but NOT including the alias.
*/
#define GENERIC_BASIC_AUTH_UPDATE_IP_REQUEST				\
	"GET %s"							\
	"%s "								\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"Authorization: Basic %s\r\n"					\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

#define ZONEEDIT_UPDATE_IP_REQUEST					\
	"GET %s?"							\
	"host=%s&"							\
	"dnsto=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"Authorization: Basic %s\r\n"					\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/* dont ask me why easydns is so picky
 * http://support.easydns.com/tutorials/dynamicUpdateSpecs.php */
#define EASYDNS_UPDATE_IP_REQUEST					\
	"GET %s?"							\
	"hostname=%s&"							\
	"myip=%s&"							\
	"wildcard=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"Authorization: Basic %s\r\n"					\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/* tzo doesnt encode password */
#define TZO_UPDATE_IP_REQUEST						\
	"GET %s?"							\
	"tzoname=%s&"							\
	"email=%s&"							\
	"tzokey=%s&"							\
	"ipaddress=%s&"							\
	"system=tzodns&"						\
	"info=1 "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/* sitelutions.com specific update address format */
#define SITELUTIONS_UPDATE_IP_HTTP_REQUEST				\
	"GET %s?"							\
	"user=%s&"							\
	"pass=%s&"							\
	"id=%s&"							\
	"ip=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

#define DNSEXIT_UPDATE_IP_HTTP_REQUEST					\
	"GET %s?"							\
	"login=%s&"							\
	"password=%s&"							\
	"host=%s&"							\
	"myip=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/* HE tunnelbroker.com specific update request format */
#define HE_IPV6TB_UPDATE_IP_REQUEST					\
	"GET %s?"							\
	"ip=%s&"							\
	"apikey=%s&"							\
	"pass=%s&"							\
	"tid=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

#define CHANGEIP_UPDATE_IP_HTTP_REQUEST					\
	"GET %s?"							\
	"system=dyndns&"						\
	"hostname=%s&"							\
	"myip=%s "							\
	"HTTP/1.0\r\n"							\
	"Host: %s\r\n"							\
	"Authorization: Basic %s\r\n"					\
	"User-Agent: " AGENT_NAME " " SUPPORT_ADDR "\r\n\r\n"

/* Some default configurations */
#define DYNDNS_DEFAULT_STARTUP_SLEEP      0       /* sec */
#define DYNDNS_DEFAULT_SLEEP              120     /* sec */
#define DYNDNS_MIN_SLEEP                  30      /* sec */
#define DYNDNS_MAX_SLEEP                  (10 * 24 * 3600)        /* 10 days in sec */
#define DYNDNS_ERROR_UPDATE_PERIOD        600     /* 10 min */
#define DYNDNS_FORCED_UPDATE_PERIOD       (30 * 24 * 3600)        /* 30 days in sec */
#define DYNDNS_DEFAULT_CMD_CHECK_PERIOD   1       /* sec */
#define DYNDNS_DEFAULT_ITERATIONS         0       /* Forever */
#define DYNDNS_HTTP_RESPONSE_BUFFER_SIZE  2500    /* Bytes */
#define DYNDNS_HTTP_REQUEST_BUFFER_SIZE   2500    /* Bytes */
#define DYNDNS_MAX_ALIAS_NUMBER           10      /* maximum number of aliases per server that can be maintained */
#define DYNDNS_MAX_SERVER_NUMBER          5       /* maximum number of servers that can be maintained */

/* local configs */
#define USERNAME_LEN               50      /* chars */
#define PASSWORD_LEN               50      /* chars */
#define SERVER_NAME_LEN                   256     /* chars */
#define SERVER_URL_LEN                    256     /* chars */
#define IP_V4_MAX_LEN                     16      /* chars: nnn.nnn.nnn.nnn\0 */

/* Types used for DNS system specific configuration */
/* Function to prepare DNS system specific server requests */
typedef int (*req_fn_t) (void *this, int infnr, int alnr);
typedef int (*rsp_fn_t) (void *this, http_trans_t *p_tr, int infnr);

typedef struct {
	const char    *key;
	rsp_fn_t       rsp_fn;
	req_fn_t       update_fn;
	const char    *checkip_name;
	const char    *checkip_url;
	const char    *server_name;
	const char    *server_url;
} ddns_system_t;

typedef struct {
	int            id;
	ddns_system_t  system;
} ddns_sysinfo_t;

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
	ddns_name_t    names;
	int            update_required;
} ddns_alias_t;

typedef struct {
	int            ip_has_changed;
	ddns_name_t    my_ip_address;

	ddns_creds_t   creds;
	ddns_system_t *system;

	/* Address of DDNS update service */
	ddns_name_t    server_name;
	char           server_url[SERVER_URL_LEN];

	/* Address of "What's my IP" checker */
	ddns_name_t    checkip_name;
	char           checkip_url[SERVER_URL_LEN];

	ddns_name_t    proxy_server_name;

	ddns_alias_t   alias[DYNDNS_MAX_ALIAS_NUMBER];
	int            alias_count;

	int            wildcard;
} ddns_info_t;

/* Client context */
typedef struct {
	char          *cfgfile;
	char          *pidfile;
	char          *external_command;

	ddns_dbg_t     dbg;

	ddns_cmd_t     cmd;
	int            startup_delay_sec;
	int            sleep_sec; /* time between 2 updates */
	int            normal_update_period_sec;
	int            error_update_period_sec;
	int            forced_update_period_sec;
	int            forced_update_fake_addr;
	int            time_since_last_update;
	int            cmd_check_period; /*time to wait for a command */
	int            total_iterations;
	int            num_iterations;
	char          *cache_file;
	char          *bind_interface;
	char          *check_interface;
	int            initialized;
	int            run_in_background;
	int            debug_to_syslog;
	int            change_persona;
	int            update_once;
	int            force_addr_update;
	int            use_proxy;
	int            abort;

	http_t         http_to_ip_server[DYNDNS_MAX_SERVER_NUMBER];
	http_t         http_to_dyndns[DYNDNS_MAX_SERVER_NUMBER];
	http_trans_t   http_transaction;

	char          *work_buf; /* for HTTP responses */
	int            work_buflen;

	char          *request_buf; /* for HTTP requests */
	int            request_buflen;

	ddns_user_t    sys_usr_info; /* info about the current account running inadyn */

	ddns_info_t    info[DYNDNS_MAX_SERVER_NUMBER]; /* servers, names, passwd */
	int            info_count;
} ddns_t;

ddns_sysinfo_t *ddns_system_table (void);
int             ddns_main_loop    (ddns_t *ctx, int argc, char *argv[]);
int             get_config_data   (ddns_t *ctx, int argc, char *argv[]);

#endif /* DYNDNS_H_ */

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
