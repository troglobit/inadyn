/*
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>		/* sysconf() */
#include <pwd.h>		/* getpwnam_r() */
#include <grp.h>		/* getgrnam_r() */

#include "dyndns.h"
#include "debug_if.h"
#include "base64.h"
#include "get_cmd.h"

static int curr_info;

/* command line options */
#define DYNDNS_INPUT_FILE_OPT_STRING "--input_file"

typedef enum
{
	NEW_LINE,
	COMMENT,
	DATA,
	SPACE,
	ESCAPE
} state_t;

typedef struct
{
	FILE    *fp;
	state_t  state;
} cfg_parser_t;

static int help_handler(cmd_data_t *cmd, int num, void *context);
static int startup_delay_handler(cmd_data_t *cmd, int num, void *context);
static int update_once_handler(cmd_data_t *cmd, int num, void *context);
static int get_wildcard_handler(cmd_data_t *cmd, int num, void *context);
static int get_username_handler(cmd_data_t *cmd, int num, void *context);
static int get_password_handler(cmd_data_t *cmd, int num, void *context);
static int get_alias_handler(cmd_data_t *cmd, int num, void *context);
static int get_dns_server_name_handler(cmd_data_t *cmd, int num, void *context);
static int get_dns_server_url_handler(cmd_data_t *cmd, int num, void *context);
static int get_ip_server_name_handler(cmd_data_t *cmd, int num, void *context);
static int get_dyndns_system_handler(cmd_data_t *cmd, int num, void *context);
static int get_update_period_handler(cmd_data_t *cmd, int num, void *context);
static int get_update_period_sec_handler(cmd_data_t *cmd, int num, void *context);
static int get_forced_update_period_handler(cmd_data_t *cmd, int num, void *context);
static int get_logfile_name(cmd_data_t *cmd, int num, void *context);
static int set_silent_handler(cmd_data_t *cmd, int num, void *context);
static int set_verbose_handler(cmd_data_t *cmd, int num, void *context);
static int get_proxy_server_handler(cmd_data_t *cmd, int num, void *context);
static int get_options_from_file_handler(cmd_data_t *cmd, int num, void *context);
static int set_iterations_handler(cmd_data_t *cmd, int num, void *context);
static int set_syslog_handler(cmd_data_t *cmd, int num, void *context);
static int set_change_persona_handler(cmd_data_t *cmd, int num, void *context);
static int set_bind_interface(cmd_data_t *cmd, int num, void *context);
static int set_check_interface(cmd_data_t *cmd, int num, void *context);
static int set_pidfile(cmd_data_t *cmd, int num, void *context);
static int set_cachefile(cmd_data_t *cmd, int num, void *context);
static int print_version_handler(cmd_data_t *cmd, int num, void *context);
static int get_exec_handler(cmd_data_t *cmd, int num, void *context);

static cmd_desc_t cmd_options_table[] =
{
	{"-a",			1,	{get_alias_handler, NULL},	"" },
	{"--alias",		1,	{get_alias_handler, NULL},	"<NAME>\n"
	 "\t\t\tAlias hostname. This option can appear multiple times." },

	{"-b",			0,	{set_silent_handler, NULL},	""},
	{"--background",	0,	{set_silent_handler, NULL},	"Run in background."},

	{"-B",			1,	{set_bind_interface, NULL}, ""},
	{"--bind",		1,	{set_bind_interface, NULL}, "<IFNAME>\n"
	 "\t\t\tSet interface to bind to, only on UNIX systems."},

	{"-c",                  1,      {set_cachefile, NULL}, ""},
	{"--cachefile",         1,      {set_cachefile, NULL}, "<FILE>\n"
	 "\t\t\tSet cachefile, default " DYNDNS_DEFAULT_CACHE_FILE},

	{"-d",			1,	{set_change_persona_handler, NULL}, ""},
	{"--drop-privs", 	1,	{set_change_persona_handler, NULL}, "<USER[:GROUP]>\n"
	 "\t\t\tAfter init switch to a new user/group.\n"
	 "\t\t\tOnly on UNIX systems."},
	{"--change_persona", 	1,	{set_change_persona_handler, NULL}, NULL}, /* COMPAT */

	{"-e",			1,	{get_exec_handler, NULL}, ""},
	{"--exec",		1,	{get_exec_handler, NULL}, "Full path to external command to run after an IP update."},

	{"-f",			   1,   {get_forced_update_period_handler, NULL}, ""},
	{"--forced-update",        1,   {get_forced_update_period_handler, NULL}, "<SEC>\n"
	 "\t\t\tForced DDNS server update interval. Default: 30 days"},
	{"--forced_update_period", 1,   {get_forced_update_period_handler, NULL}, NULL}, /* COMPAT */

	{"-F",				1, {get_options_from_file_handler, NULL}, ""},
	{"--config",			1, {get_options_from_file_handler, NULL}, "<FILE>\n"
	 "\t\t\tConfiguration file, containing further options.  Default\n"
	 "\t\t\tconfig file: " DYNDNS_DEFAULT_CONFIG_FILE ", is used if inadyn is\n"
	 "\t\t\tcalled without any command line options." },
	{DYNDNS_INPUT_FILE_OPT_STRING,	1, {get_options_from_file_handler, NULL}, NULL},

	{"-H",			2, {get_ip_server_name_handler, NULL}, ""},
	{"--checkip-url",	2, {get_ip_server_name_handler, NULL}, "<NAME[:PORT] URL>\n"
	 "\t\t\tLocal IP is detected by parsing the response after\n"
	 "\t\t\treturned by this server and URL.  The first IP found\n"
	 "\t\t\tin the HTTP response is considered 'my IP'.\n"
	 "\t\t\tDefault value: 'checkip.dyndns.org /'"},
	{"--ip_server_name",	2, {get_ip_server_name_handler, NULL}, NULL},

	{"-n",			1,	{set_iterations_handler, NULL},	""},
	{"--iterations",	1,	{set_iterations_handler, NULL},	"<NUM>\n"
	 "\t\t\tSet the number of DNS updates. Default: 0 (forever)"},

	{"-i",			1,	{set_check_interface, NULL}, ""},
	{"--iface",		1,	{set_check_interface, NULL}, "<IFNAME>\n"
	 "\t\t\tSet interface to check for IP, only on UNIX systems.\n"
	 "\t\t\tExternal IP check is not performed."},

	{"-L",			1,	{get_logfile_name, NULL}, ""},
	{"--logfile",		1,	{get_logfile_name, NULL}, "<FILE>\n"
	 "\t\t\tFull path to log file"},
	{"--log_file",		1,	{get_logfile_name, NULL}, NULL},

	{"-N",		 	 1, {get_dns_server_name_handler, NULL}, ""},
	{"--server-name", 	 1, {get_dns_server_name_handler, NULL}, "[<NAME>[:port]]\n"
	 "\t\t\tThe server that receives the update DNS request.\n"
	 "\t\t\tAllows the use of unknown DNS services that accept HTTP\n"
	 "\t\t\tupdates.  If no proxy is wanted, then it is enough to\n"
	 "\t\t\tset the dyndns system.  Default servers will be taken."},
	{"--dyndns_server_name", 1, {get_dns_server_name_handler, NULL}, NULL},

	{"-U",		 	1, {get_dns_server_url_handler, NULL}, ""},
	{"--server-url", 	1, {get_dns_server_url_handler, NULL}, "<URL>\n"
	 "\t\t\tFull URL relative to DynDNS server root.\n"
	 "\t\t\tEx: /some_script.php?hostname=\n"},
	{"--dyndns_server_url", 1, {get_dns_server_url_handler, NULL}, NULL},

	{"-S",			1,	{get_dyndns_system_handler, NULL}, ""},
	{"--system",		1,	{get_dyndns_system_handler, NULL}, "<PROVIDER>\n"
	 "\t\t\tSelect DDNS service provider, one of the following.\n"
	 "\t\t\to For dyndns.org:         default@dyndns.org\n"
	 "\t\t\to For freedns.afraid.org: default@freedns.afraid.org\n"
	 "\t\t\to For zoneedit.com:       default@zoneedit.com\n"
	 "\t\t\to For no-ip.com:          default@no-ip.com\n"
	 "\t\t\to For easydns.com:        default@easydns.com\n"
	 "\t\t\to For tzo.com:            default@tzo.com\n"
	 "\t\t\to For 3322.org:           dyndns@3322.org\n"
	 "\t\t\to For dnsomatic.com:      default@dnsomatic.com\n"
	 "\t\t\to For tunnelbroker.net:   ipv6tb@he.net\n"
	 "\t\t\to For dns.he.net:         dyndns@he.net\n"
	 "\t\t\to For dynsip.org:         default@dynsip.org\n"
	 "\t\t\to For sitelutions.com:    default@sitelutions.com\n"
	 "\t\t\to For dnsexit.com:   	  default@dnsexit.com\n"
	 "\t\t\to For generic:            custom@http_svr_basic_auth\n\n"
	 "\t\t\tDefault value:            default@dyndns.org"},
	{"--dyndns_system",	1,	{get_dyndns_system_handler, NULL}, NULL},

	{"-x",			1,	{get_proxy_server_handler, NULL}, ""},
	{"--proxy-server",	1,	{get_proxy_server_handler, NULL}, "[NAME[:port]]\n"
	 "\t\t\tHTTP proxy server name, and optional port. Default: N/A"},
	{"--proxy_server",	1,	{get_proxy_server_handler, NULL}, NULL}, /* COMPAT */

	{"-T",			1,	{get_update_period_sec_handler, NULL},	""},
	{"--period",		1,	{get_update_period_sec_handler, NULL},	"<SEC>\n"
	 "\t\t\tIP change check interval.  Default: 2 min. Max: 10 days"},
	{"--update_period_sec",	1,	{get_update_period_sec_handler, NULL},	NULL},
	{"--update_period",	1,	{get_update_period_handler, NULL},      NULL},

	{"-P",			1,	{set_pidfile, NULL}, ""},
	{"--pidfile",		1,	{set_pidfile, NULL}, "<FILE>\n"
	 "\t\t\tSet pidfile, default " DYNDNS_DEFAULT_PIDFILE},

	{"-s",			0,	{set_syslog_handler, NULL}, ""},
	{"--syslog",		0,	{set_syslog_handler, NULL},
	 "Force logging to syslog, e.g., /var/log/messages, only on UNIX systems"},

	{"-t",                  1,      {startup_delay_handler, NULL}, ""},
	{"--startup-delay",     1,      {startup_delay_handler, NULL}, "<SEC>\n"
	 "\t\t\tWait for network/NTP to come up at boot.  Default: 0 sec"},

	{"-o",			0,	{update_once_handler, NULL}, ""},
	{"--once",		0,	{update_once_handler, NULL}, "Force one update and quit."},

	{"-u",			1,	{get_username_handler, NULL},	""},
	{"--username",		1,	{get_username_handler, NULL},	"<USERNAME>\n"
	 "\t\t\tYour DDNS user name, or hash"},

	{"-p",			1,	{get_password_handler, NULL},	""},
	{"--password",		1,	{get_password_handler, NULL},	"<PASSWORD>\n"
	 "\t\t\tYour DDNS user password."},

	{"-w",			0,	{get_wildcard_handler, NULL}, ""},
	{"--wildcard",		0,	{get_wildcard_handler, NULL}, "Enable domain wildcarding for easydns.com."},

	{"-h",			0,	{help_handler, NULL},	"" },
	{"--help",		0,	{help_handler, NULL},	"This online help." },

	{"-V",			1,	{set_verbose_handler, NULL},   ""},
	{"--verbose",		1,	{set_verbose_handler, NULL},   "Debug level: 0 - 5"},

	{"-v",			0,	{print_version_handler, NULL}, ""},
	{"--version",		0,	{print_version_handler, NULL}, "Show inadyn version"},

	{NULL,			0,	{0, NULL},	NULL }
};


void print_help_page(void)
{
	cmd_desc_t *it;

	puts("Inadyn is a dynamic DNS (DDNS) client.  It does periodic and/or on-demand checks\n"
	     "of your externally visible IP address and updates the hostname to IP mapping at\n"
	     "your DDNS service provider when necessary.\n");
	puts("dyndns.org:\n"
	     "\tinadyn -u username -p password -a my.registrated.name\n");

	it = cmd_options_table;
	while (it->option != NULL)
	{
		if (it->description)
		{
			if (strlen(it->option) == 2)
				printf("  %s, ", it->option);
			else
				printf("%-16s  %s\n", it->option, it->description);
		}
		++it;
	}
}

static int help_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	(void)cmd;
	(void)num;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->abort = 1;
	print_help_page();

	return 0;
}

static int startup_delay_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sscanf(cmd->argv[num], "%d", &ctx->startup_delay_sec) != 1)
		return RC_DYNDNS_INVALID_OPTION;

	return 0;
}

static int update_once_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	(void)cmd;
	(void)num;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->update_once = 1;
	ctx->total_iterations = 1;
	ctx->dbg.level = 5;

	return 0;
}

static int get_wildcard_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	(void)cmd;
	(void)num;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->info[curr_info].wildcard = 1;

	return 0;
}

static int set_verbose_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sscanf(cmd->argv[num], "%d", &ctx->dbg.level) != 1)
		return RC_DYNDNS_INVALID_OPTION;

	return 0;
}

static int set_iterations_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sscanf(cmd->argv[num], "%d", &ctx->total_iterations) != 1)
		return RC_DYNDNS_INVALID_OPTION;

	ctx->total_iterations = (ctx->sleep_sec < 0) 
		? DYNDNS_DEFAULT_ITERATIONS
		: ctx->total_iterations;

	return 0;
}

static int set_silent_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	(void)cmd;
	(void)num;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->run_in_background = 1;

	return 0;
}

static int get_logfile_name(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sizeof(ctx->dbg.p_logfilename) < strlen(cmd->argv[num]))
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	strcpy(ctx->dbg.p_logfilename, cmd->argv[num]);

	return 0;
}

static int get_username_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sizeof(ctx->info[curr_info].creds.username) < strlen(cmd->argv[num]))
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	strcpy(ctx->info[curr_info].creds.username, cmd->argv[num]);

	return 0;
}

static int get_password_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sizeof(ctx->info[curr_info].creds.password) < strlen(cmd->argv[num]))
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	strcpy(ctx->info[curr_info].creds.password, (cmd->argv[num]));

	return 0;
}

static int get_alias_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (ctx->info[curr_info].alias_count >= DYNDNS_MAX_ALIAS_NUMBER)
		return RC_DYNDNS_TOO_MANY_ALIASES;

	if (sizeof(ctx->info[curr_info].alias[ctx->info[curr_info].alias_count].names) < strlen(cmd->argv[num]))
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	strcpy(ctx->info[curr_info].alias[ctx->info[curr_info].alias_count].names.name, (cmd->argv[num]));
	ctx->info[curr_info].alias_count++;

	return 0;
}

static int get_name_and_port(char *p_src, char *p_dest_name, int *p_dest_port)
{
	const char *p_port = strstr(p_src,":");

	if (p_port)
	{
		int port_nr, len;
		int port_ok = sscanf(p_port + 1, "%d",&port_nr);

		if (port_ok != 1)
			return RC_DYNDNS_INVALID_OPTION;

		*p_dest_port = port_nr;
		len = p_port - p_src;
		memcpy(p_dest_name, p_src, len);
		p_dest_name[len] = 0;
	}
	else
	{
		strcpy(p_dest_name, p_src);
	}

	return 0;
}

/** Returns the svr name and port if the format is :
 * name[:port] url.
 */
static int get_ip_server_name_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;
	int rc;
	int port = -1;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	/*ip_server_name*/
	if (sizeof(ctx->info[curr_info].ip_server_name) < strlen(cmd->argv[num]) + 1)
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	ctx->info[curr_info].ip_server_name.port = HTTP_DEFAULT_PORT;
	rc = get_name_and_port(cmd->argv[num], ctx->info[curr_info].ip_server_name.name, &port);
	if (rc == 0 && port != -1)
		ctx->info[curr_info].ip_server_name.port = port;

	if (sizeof(ctx->info[curr_info].ip_server_url) < strlen(cmd->argv[num + 1]) + 1)
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	strcpy(ctx->info[curr_info].ip_server_url, cmd->argv[num + 1]);

	return rc;
}

static int get_dns_server_name_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;
	int rc;
	int port = -1;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sizeof(ctx->info[curr_info].dyndns_server_name) < strlen(cmd->argv[num]))
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	ctx->info[curr_info].dyndns_server_name.port = HTTP_DEFAULT_PORT;
	rc = get_name_and_port(cmd->argv[num], ctx->info[curr_info].dyndns_server_name.name, &port);
	if (rc == 0 && port != -1)
		ctx->info[curr_info].dyndns_server_name.port = port;

	return rc;
}

int get_dns_server_url_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sizeof(ctx->info[curr_info].dyndns_server_url) < strlen(cmd->argv[num]))
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	strcpy(ctx->info[curr_info].dyndns_server_url, cmd->argv[num]);

	return 0;
}

/* returns the proxy server nme and port
 */
static int get_proxy_server_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;
	int rc;
	int port = -1;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sizeof(ctx->info[curr_info].proxy_server_name) < strlen(cmd->argv[num]))
		return  RC_DYNDNS_BUFFER_TOO_SMALL;

	ctx->info[curr_info].proxy_server_name.port = HTTP_DEFAULT_PORT;
	rc = get_name_and_port(cmd->argv[num], ctx->info[curr_info].proxy_server_name.name, &port);
	if (rc == 0 && port != -1)
		ctx->info[curr_info].proxy_server_name.port = port;

	return rc;
}
/* Read the dyndnds name update period.
   and impose the max and min limits
*/
static int get_update_period_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sscanf(cmd->argv[num], "%d", &ctx->sleep_sec) != 1)
		return RC_DYNDNS_INVALID_OPTION;

	ctx->sleep_sec /= 1000;
	ctx->sleep_sec = (ctx->sleep_sec < DYNDNS_MIN_SLEEP) ?  DYNDNS_MIN_SLEEP: ctx->sleep_sec;
	(ctx->sleep_sec > DYNDNS_MAX_SLEEP) ?  ctx->sleep_sec = DYNDNS_MAX_SLEEP: 1;

	return 0;
}

static int get_update_period_sec_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sscanf(cmd->argv[num], "%d", &ctx->normal_update_period_sec) != 1)
		return RC_DYNDNS_INVALID_OPTION;

	ctx->normal_update_period_sec = (ctx->normal_update_period_sec < DYNDNS_MIN_SLEEP) ?  DYNDNS_MIN_SLEEP: ctx->normal_update_period_sec;
	(ctx->normal_update_period_sec > DYNDNS_MAX_SLEEP) ?  ctx->normal_update_period_sec = DYNDNS_MAX_SLEEP: 1;

	return 0;
}

static int get_forced_update_period_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	if (sscanf(cmd->argv[num], "%d", &ctx->forced_update_period_sec) != 1)
		return RC_DYNDNS_INVALID_OPTION;

	return 0;
}

static int set_syslog_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	(void)cmd;
	(void)num;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->debug_to_syslog = 1;

	return 0;
}

/**
 * Reads the params for change persona. Format:
 * <uid[:gid]>
 */
static int set_change_persona_handler(cmd_data_t *cmd, int num, void *context)
{
	int s, result = 0;
	char *arg, *buf, *p_gid;
	ssize_t bufsize;
	gid_t gid;
	uid_t uid;
	struct passwd pwd, *pwd_res;
	long login_len_max;
	ddns_t *ctx = (ddns_t *) context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	/* Determine max length of a username */
	login_len_max = sysconf(_SC_LOGIN_NAME_MAX);
	if (login_len_max <= 0)
		login_len_max = 32;

	arg = cmd->argv[num];
	{
		char groupname[33] = ""; /* MAX 32 chars + '\0', groupadd(8) */
		char username[login_len_max+1];
		char fmt[65]; /* Conversion string for username */

		uid = getuid();
		gid = getgid();

		p_gid = strstr(arg, ":");
		if (p_gid)
		{
			if ((strlen(p_gid + 1) > 0) &&  /* if something is present after : */
			    sscanf(p_gid + 1, "%32[a-zA-Z-]", groupname) != 1)
				return RC_DYNDNS_INVALID_OPTION;
		}

		snprintf(fmt, sizeof(fmt), "%%%ld[a-zA-Z-]", login_len_max);
		if (sscanf(arg, fmt, username) != 1)
			return RC_DYNDNS_INVALID_OPTION;

		/* Get uid and gid by their names */
		if (strlen(groupname) > 0)
		{
			struct group grp;
			struct group *grp_res;

			bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
			if (bufsize == -1)
				bufsize = 16384;        /* Should be more than enough */

			buf = malloc(bufsize);
			if (buf == NULL)
				return RC_OUT_OF_MEMORY;

			s = getgrnam_r(groupname, &grp, buf, bufsize, &grp_res);
			if (grp_res != NULL)
			{
				gid = grp.gr_gid;
			}
			else
			{
				if (s == 0)
				{
					logit(LOG_ERR, "Cannot find GROUP %s", groupname);
					result = RC_OS_INVALID_GID;
				}
				else
				{
					result = RC_ERROR;
				}
			}
			free(buf);

			if (0 != result)
				return result;
		}

		bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (bufsize == -1)          /* Value was indeterminate */
			bufsize = 16384;    /* Should be more than enough */

		buf = malloc(bufsize);
		if (buf == NULL)
			return RC_OUT_OF_MEMORY;

		s = getpwnam_r(username, &pwd, buf, bufsize, &pwd_res);
		if (pwd_res != NULL)
		{
			uid = pwd.pw_uid;
			if (gid == getgid())
				gid = pwd.pw_gid;
		}
		else
		{
			if (s == 0)
			{
				logit(LOG_ERR, "Cannot find USER %s", username);
				result = RC_OS_INVALID_UID;
			}
			else
			{
				result = RC_ERROR;
			}
		}
		free(buf);

		if (0 != result)
			return result;

		ctx->change_persona = 1;
		ctx->sys_usr_info.gid = gid;
		ctx->sys_usr_info.uid = uid;
	}

	return 0;
}

static int set_bind_interface(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->bind_interface = strdup(cmd->argv[num]);

	return 0;
}

static int set_check_interface(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->check_interface = strdup(cmd->argv[num]);

	return 0;
}

static int set_pidfile(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->pidfile = strdup(cmd->argv[num]);

	return 0;
}

static int set_cachefile(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->cache_file = strdup(cmd->argv[num]);

	return 0;
}

int print_version_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	(void)cmd;
	(void)num;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	printf("%s\n", DYNDNS_VERSION_STRING);
	ctx->abort = 1;

	return 0;
}

static int get_exec_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_t *ctx = (ddns_t *)context;

	(void)cmd;
	(void)num;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	ctx->external_command = strdup(cmd->argv[num]);

	return 0;
}

/**
   Searches the DYNDNS system by the argument.
   Input is like: system@server.name
   system=statdns|custom|dyndns|default
   server name = dyndns.org | freedns.afraid.org
   The result is a pointer in the table of DNS systems.
*/
static int get_dyndns_system_handler(cmd_data_t *cmd, int num, void *context)
{
	ddns_system_t *system = NULL;
	ddns_t *ctx = (ddns_t *)context;
	ddns_sysinfo_t *it;

	if (ctx == NULL)
		return RC_INVALID_POINTER;

	it = ddns_system_table();
	for (; it != NULL && it->id != LAST_DNS_SYSTEM; ++it)
	{
		if (strcmp(it->system.key, cmd->argv[num]) == 0)
		{
			system = &it->system;
			break;
		}
	}

	if (system == NULL)
	{
		logit(LOG_ERR, "Cannot find DDNS provider %s, check your spelling.", cmd->argv[num]);
		return RC_CMD_PARSER_INVALID_OPTION_ARGUMENT;
	}

	for (curr_info = 0; curr_info < ctx->info_count &&
		     curr_info < DYNDNS_MAX_SERVER_NUMBER &&
		     ctx->info[curr_info].system != system; curr_info++)
		;

	if (curr_info >= ctx->info_count)
	{
		if (curr_info < DYNDNS_MAX_SERVER_NUMBER)
		{
			ctx->info_count++;
			ctx->info[curr_info].system = system;
		}
	   	else
		{
			return RC_DYNDNS_BUFFER_TOO_SMALL;
		}
	}

	return 0;
}

static int push_in_buffer(char* p_src, int src_len, char *p_buffer, int* p_act_len, int max_len)
{
	if (*p_act_len + src_len > max_len)
		return RC_FILE_IO_OUT_OF_BUFFER;

	memcpy(p_buffer + *p_act_len,p_src, src_len);
	*p_act_len += src_len;

	return 0;
}

static int parser_init(cfg_parser_t *cfg, FILE *fp)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->state = NEW_LINE;
	cfg->fp = fp;

	return 0;
}

/** Read one single option from file into the given buffer.
    When the first separator is encountered it returns.
    Actions:
    - read chars while not eof
    - skip comments (parts beginning with '#' and ending with '\n')
    - switch to DATA STATE if non space char is encountered
    - assume first name in lines to be a long option name by adding '--' if necesssary
    - add data to buffer
    - do not forget a 0 at the end
    * States:
    * NEW_LINE - wait here until some option. Add '--' if not already there
    * SPACE - between options. Like NEW_LINE but no additions
    * DATA - real data. Stop on space.
    * COMMENT - everything beginning with # until EOLine
    * ESCAPE - everything that is otherwise (incl. spaces). Next char is raw copied.
    */
static int parser_read_option(cfg_parser_t *cfg, char *p_buffer, int maxlen)
{
	int rc = 0;
	int parse_end = 0;
	int count = 0;
	*p_buffer = 0;

	while (!parse_end)
	{
		char ch;
		{
			int n;
			if ((n = fscanf(cfg->fp, "%c", &ch)) < 0)
			{
				if (feof(cfg->fp))
					break;

				rc = RC_FILE_IO_READ_ERROR;

				break;
			}
		}

		switch (cfg->state)
		{
			case NEW_LINE:
				if (ch == '\\')
				{
					cfg->state = ESCAPE;
					break;
				}

				if (ch == '#') /*comment*/
				{
					cfg->state = COMMENT;
					break;
				}

				if (!isspace(ch))
				{
					if (ch != '-')/*add '--' to first word in line*/
					{
						if ((rc = push_in_buffer("--", 2, p_buffer, &count, maxlen)) != 0)
							break;
					}

					if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != 0)
						break;

					cfg->state = DATA;
					break;
				}
				/*skip actual leading  spaces*/
				break;

			case SPACE:
				if (ch == '\\')
				{
					cfg->state = ESCAPE;
					break;
				}

				if (ch == '#') /*comment*/
				{
					cfg->state = COMMENT;
					break;
				}

				if (ch == '\n' || ch == '\r')
				{
					cfg->state = NEW_LINE;
					break;
				}

				if (!isspace(ch))
				{
					if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != 0)
						break;

					cfg->state = DATA;
					break;
				}
				break;

			case COMMENT: /*skip comments*/
				if (ch == '\n' || ch == '\r')
					cfg->state = NEW_LINE;
				break;

			case DATA:
				if (ch == '\\')
				{
					cfg->state = ESCAPE;
					break;
				}

				if (ch == '#')
				{
					cfg->state = COMMENT;
					break;
				}

				if (ch == '\n' || ch == '\r')
				{
					cfg->state = NEW_LINE;
					parse_end = 1;
					break;
				}

				if (isspace(ch))
				{
					cfg->state = SPACE;
					parse_end = 1;
					break;
				}

				/*actual data*/
				if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != 0)
					break;
				break;

			case ESCAPE:
				if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != 0)
					break;

				cfg->state = DATA;
				break;

			default:
				rc = RC_CMD_PARSER_INVALID_OPTION;
				break;
		}

		if (rc != 0)
			break;
	}

	if (rc == 0)
	{
		char ch = 0;
		rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen);
	}

	return rc;
}


/**
   This handler reads the data in the passed file name.
   Then appends the words in the table (cutting all spaces) to the existing cmd line options.
   It adds to the cmd_data_t struct.
   Actions:
   - open file
   - read characters and cut spaces away
   - add values one by one to the existing cmd data
*/
static int get_options_from_file_handler(cmd_data_t *cmd, int num, void *context)
{
	int rc = 0;
	FILE *fp = NULL;
	char *buf = NULL;
	const size_t buflen = DYNDNS_SERVER_NAME_LEN;
	ddns_t *ctx = (ddns_t *)context;
	cfg_parser_t parser;

	if (!ctx || !cmd)
		return RC_INVALID_POINTER;

	do
	{
 		buf = malloc(buflen);
   		if (!buf)
	 	{
	 		rc = RC_OUT_OF_MEMORY;
	 		break;
	  	}

	  	fp = fopen(cmd->argv[num], "r");
	  	if (!fp)
	  	{
			logit(LOG_ERR, "Cannot open config file %s: %s", cmd->argv[num], strerror(errno));
	  		rc = RC_FILE_IO_OPEN_ERROR;
	  		break;
	  	}

		/* Save for later... */
		if (ctx->cfgfile)
			free(ctx->cfgfile);
		ctx->cfgfile = strdup(cmd->argv[num]);

		if ((rc = parser_init(&parser, fp)) != 0)
			break;

		while (!feof(fp))
		{
			rc = parser_read_option(&parser,buf, buflen);
			if (rc != 0)
				break;

			if (!strlen(buf))
				break;

			rc = cmd_add_val(cmd, buf);
			if (rc != 0)
				break;
   		}
	}
	while (0);

	if (fp)
		fclose(fp);

	if (buf)
		free(buf);

	return rc;
}

static void check_setting(int cond, int no, char *msg, int *ok)
{
	if (!cond)
	{
		logit(LOG_WARNING, "%s in account %d", msg, no + 1);
		*ok = 0;
	}
}

/* Returns POSIX OK(0) on success, non-zero on validation failure. */
static int validate_configuration(ddns_t *ctx)
{
	int i, num = 0;

	if (!ctx->info_count)
	{
		logit(LOG_ERR, "No DDNS provider setup in configuration.");
		return 1;
	}

	for (i = 0; i < ctx->info_count; i++)
	{
		int ok = 1;
		ddns_info_t *account = &ctx->info[i];

		check_setting(strlen(account->creds.username), i, "Missing username", &ok);
		check_setting(strlen(account->creds.password), i, "Missing password", &ok);
		check_setting(account->alias_count, i, "Missing your alias/hostname", &ok);
		check_setting(strlen(account->dyndns_server_name.name), i, "Missing DDNS server address, check DDNS provider", &ok);
		check_setting(strlen(account->ip_server_name.name), i, "Missing check IP address, check DDNS provider", &ok);

		if (ok)
			num++;
	}

	if (!num)
	{
		logit(LOG_ERR, "No valid DDNS setup exists.");
		return 1;
	}

	if (num != ctx->info_count)
		logit(LOG_WARNING, "Not all account setups are valid, please check configuration.");

	return 0;
}


/*
  Set up all details:
  - ip server name
  - dns server name
  - username, passwd
  - ...
  Implementation:
  - load defaults
  - parse cmd line
  - assign settings that may change due to cmd line options
  - check data
  Note:
  - if no argument is specified tries to call the cmd line parser
  with the default cfg file path.
*/
int get_config_data(ddns_t *ctx, int argc, char** argv)
{
	int i;
	int rc = 0;
	int cache_file_len;
	cmd_desc_t *it;

	do
	{
		rc = ddns_default_config_data(ctx);
		if (rc != 0)
			break;

		it = cmd_options_table;
		while (it->option != NULL)
		{
			it->handler.context = ctx;
			++it;
		}

		/* in case of no options, assume the default cfg file may be present */
		if (argc == 1)
		{
			char *custom_argv[] = {"", DYNDNS_INPUT_FILE_OPT_STRING, DYNDNS_DEFAULT_CONFIG_FILE};
			int custom_argc = sizeof(custom_argv) / sizeof(char*);

			if (ctx->dbg.level)
				logit(LOG_NOTICE, "Using default config file %s", DYNDNS_DEFAULT_CONFIG_FILE);

			if (ctx->cfgfile)
				free(ctx->cfgfile);
			ctx->cfgfile = strdup(DYNDNS_DEFAULT_CONFIG_FILE);
			rc = get_cmd_parse_data(custom_argv, custom_argc, cmd_options_table);
		}
		else
		{
			rc = get_cmd_parse_data(argv, argc, cmd_options_table);
		}

		if (rc != 0 || ctx->abort)
			break;

		/* settings that may change due to cmd line options */
		i = 0;
		do
		{
			/*ip server*/
			if (strlen(ctx->info[i].ip_server_name.name) == 0)
			{
				if (sizeof(ctx->info[i].ip_server_name.name) < strlen(ctx->info[i].system->ip_server_name))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(ctx->info[i].ip_server_name.name, ctx->info[i].system->ip_server_name);

				if (sizeof(ctx->info[i].ip_server_url) < strlen(ctx->info[i].system->ip_server_url))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(ctx->info[i].ip_server_url, ctx->info[i].system->ip_server_url);
			}

			/*dyndns server*/
			if (strlen(ctx->info[i].dyndns_server_name.name) == 0)
			{
				if (sizeof(ctx->info[i].dyndns_server_name.name) < strlen(ctx->info[i].system->ddns_server_name))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(ctx->info[i].dyndns_server_name.name, ctx->info[i].system->ddns_server_name);

				if (sizeof(ctx->info[i].dyndns_server_url) < strlen(ctx->info[i].system->ddns_server_url))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(ctx->info[i].dyndns_server_url, ctx->info[i].system->ddns_server_url);
			}
		}
		while(++i < ctx->info_count);

		/* Check if the neccessary params have been provided */
		if (validate_configuration(ctx))
		{
			rc = RC_DYNDNS_INVALID_OR_MISSING_PARAMETERS;
			break;
		}

		/* Setup a default cache file, unless the user provided one for us. */
		if (ctx->bind_interface && !ctx->cache_file)
		{
			cache_file_len = (strlen(DYNDNS_CACHE_FILE) - 2) + strlen(ctx->bind_interface);
			if ((ctx->cache_file = malloc(cache_file_len + 1)) == NULL)
			{
				rc = RC_OUT_OF_MEMORY;
				break;
			}

			if (snprintf(ctx->cache_file, cache_file_len + 1, DYNDNS_CACHE_FILE,
				     ctx->bind_interface) != cache_file_len)
			{
				rc = RC_ERROR;
				break;
			}
		}
		else
		{
			ctx->cache_file = strdup(DYNDNS_DEFAULT_CACHE_FILE);
		}

	}
	while (0);

	return rc;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "ellemtel"
 *  c-basic-offset: 8
 * End:
 */
