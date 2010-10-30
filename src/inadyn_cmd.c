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

#define MODULE_TAG ""

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "dyndns.h"
#include "debug_if.h"
#include "base64.h"
#include "get_cmd.h"

static int curr_info;

/* command line options */
#define DYNDNS_INPUT_FILE_OPT_STRING "--input_file"

static RC_TYPE help_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE wildcard_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_username_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_password_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_alias_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_dns_server_name_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_dns_server_url_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_ip_server_name_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_dyndns_system_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_update_period_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_update_period_sec_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_forced_update_period_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_logfile_name(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE set_silent_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE set_verbose_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_proxy_server_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_options_from_file_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE set_iterations_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE set_syslog_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE set_change_persona_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE set_bind_interface(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE set_pidfile(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE print_version_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);
static RC_TYPE get_exec_handler(CMD_DATA *p_cmd, int current_nr, void *p_context);

static CMD_DESCRIPTION_TYPE cmd_options_table[] =
{
	{"--help",	0,	{help_handler, NULL},	"help" },
	{"-h",		0,	{help_handler, NULL},	"help" },

	{"--username",	1,	{get_username_handler, NULL},	"your  membername/ hash"},
	{"-u",		1,	{get_username_handler, NULL},	"your  membername / hash"},

	{"--password",	1,	{get_password_handler, NULL},	"your password. Optional."},
	{"-p",		1,	{get_password_handler, NULL},	"your password"},

	{"--alias",	1,	{get_alias_handler, NULL},	"alias host name. this option can appear multiple times." },
	{"-a",		1,	{get_alias_handler, NULL},	"alias host name. this option can appear multiple times." },

	{DYNDNS_INPUT_FILE_OPT_STRING, 1, {get_options_from_file_handler, NULL}, "<FILE>\n"
	 "\t\t\tThe file containing (further) options.  The default\n"
	 "\t\t\tconfig file, " DYNDNS_DEFAULT_CONFIG_FILE ", is used if inadyn is\n"
	 "\t\t\tcalled without any command line options." },

	{"--ip_server_name",	2, {get_ip_server_name_handler, NULL},
	 "<srv_name[:port] local_url>\n"
	 "\t\t\tLocal IP is detected by parsing the response after\n"
	 "\t\t\treturned by this server and URL.  The first IP found\n"
	 "\t\t\tin the HTTP response is considered 'my IP'.\n"
	 "\t\t\tDefault value: 'checkip.dyndns.org /"},

	{"--dyndns_server_name", 1, {get_dns_server_name_handler, NULL},
	 "[<NAME>[:port]]\n"
	 "\t\t\tThe server that receives the update DNS request.\n"
	 "\t\t\tAllows the use of unknown DNS services that accept HTTP\n"
	 "\t\t\tupdates.  If no proxy is wanted, then it is enough to\n"
	 "\t\t\tset the dyndns system.  Default servers will be taken."},

	{"--dyndns_server_url", 1, {get_dns_server_url_handler, NULL},
	 "<name>\n"
	 "\t\t\tFull URL relative to DynDNS server root.\n"
	 "\t\t\tEx: /some_script.php?hostname=\n"},

	{"--dyndns_system",	1,	{get_dyndns_system_handler, NULL},
	 "[NAME] - optional DYNDNS service type. SHOULD be one of the following: \n"
	 "\t\t\to For dyndns.org: dyndns@dyndns.org OR statdns@dyndns.org OR customdns@dyndns.org\n"
	 "\t\t\to For freedns.afraid.org: default@freedns.afraid.org\n"
	 "\t\t\to For www.zoneedit.com: default@zoneedit.com\n"
	 "\t\t\to For www.no-ip.com DNS system: default@no-ip.com\n"
	 "\t\t\to For easydns.com: default@easydns.com\n"
	 "\t\t\to For tzo.com: default@tzo.com\n"
	 "\t\t\to For 3322.org: dyndns@3322.org\n"
	 "\t\t\to For generic: custom@http_svr_basic_auth\n"
	 "\t\t\tDEFAULT value is intended for default service at dyndns.org (most users): dyndns@dyndns.org"},

	{"--proxy_server",	1,	{get_proxy_server_handler, NULL},
	 "[NAME[:port]]  - the http proxy server name and port. Default is none."},
	{"--update_period",	1,	{get_update_period_handler, NULL},
	 "how often the IP is checked. The period is in [ms]. Default is about 1 min. Max is 10 days"},
	{"--update_period_sec",	1,	{get_update_period_sec_handler, NULL},	"how often the IP is checked. The period is in [sec]. Default is about 1 min. Max is 10 days"},
	{"--forced_update_period", 1,   {get_forced_update_period_handler, NULL},"how often the IP is updated even if it is not changed. [in sec]"},

	{"--log_file",		1,	{get_logfile_name, NULL},	"log file path abd name"},
	{"--background",	0,	{set_silent_handler, NULL},	"run in background. output to log file or to syslog"},

	{"--verbose",		1,	{set_verbose_handler, NULL},	"set dbg level. 0 to 5"},

	{"--iterations",	1,	{set_iterations_handler, NULL},	"Set the number of DNS updates. Default is 0 (forever)."},
	{"--syslog",		0,	{set_syslog_handler, NULL},	"Force logging to syslog, e.g., /var/log/messages.  Works on UN*X systems only."},
	{"--change_persona", 	1,	{set_change_persona_handler, NULL}, "<uid[:gid]>\n"
	 "\t\t\tAfter init switch to a new user/group.\n"
	 "\t\t\tWorks on UN*X systems only."},
	{"--bind_interface",	1,	{set_bind_interface, NULL}, "<ifname>\n"
	 "\t\t\tSet interface to bind. Parameters: <interface>.\n"
	 "\t\t\tWorks on UN*X systems only."},
	{"--pidfile",		1,	{set_pidfile, NULL}, "<FILE>\n"
	 "\t\t\tSet pidfile, default /var/run/inadyn.pid."},
	{"--version",		0,	{print_version_handler, NULL}, "Print the version number\n"},
	{"--wildcard",		0,	{wildcard_handler, NULL}, "Enable domain wildcarding for dyndns.org, 3322.org, or easydns.com."},
	{"--exec",		1,	{get_exec_handler, NULL}, "Full path to external command to run after an IP update."},
	{NULL,			0,	{0, NULL},	NULL }
};


void print_help_page(void)
{
	CMD_DESCRIPTION_TYPE *it;

	puts("Inadyn is a dynamic DNS (DDNS) client.  It does periodic and/or on-demand checks\n"
	     "of your externally visible IP address and updates the host name to IP mapping at\n"
	     "your DDNS service provider when necessary.\n");
	puts("dyndns.org:\n"
	     "\tinadyn -u username -p password -a my.registrated.name\n"
	     "\n"
	     "freedns.afraid.org:\n"
	     "\t inadyn --dyndns_system default@freedns.afraid.org \\\n"
	     "\t        -a my.registrated.name,hash -a anothername,hash2\n"
	     "\n"
	     "The 'hash' is extracted from the grab URL batch file that is downloaded from\n"
	     "freedns.afraid.org\n");

	it = cmd_options_table;
	while (it->p_option != NULL)
	{
		if (it->p_description)
			printf("  %-20s  %s\n\r", it->p_option, it->p_description);
		++it;
	}
}

static RC_TYPE help_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	(void)p_cmd;
	(void)current_nr;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	p_self->abort = TRUE;
	print_help_page();

	return RC_OK;
}

static RC_TYPE wildcard_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	(void)p_cmd;
	(void)current_nr;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	p_self->wildcard = TRUE;

	return RC_OK;
}

static RC_TYPE set_verbose_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (sscanf(p_cmd->argv[current_nr], "%d", &p_self->dbg.level) != 1)
	{
		return RC_DYNDNS_INVALID_OPTION;
	}
	return RC_OK;
}

static RC_TYPE set_iterations_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (sscanf(p_cmd->argv[current_nr], "%d", &p_self->total_iterations) != 1)
	{
		return RC_DYNDNS_INVALID_OPTION;
	}

	p_self->total_iterations = (p_self->sleep_sec < 0) ?  DYNDNS_DEFAULT_ITERATIONS : p_self->total_iterations;
	return RC_OK;
}

static RC_TYPE set_silent_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	(void)p_cmd;
	(void)current_nr;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	p_self->run_in_background = TRUE;
	return RC_OK;
}

static RC_TYPE get_logfile_name(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (sizeof(p_self->dbg.p_logfilename) < strlen(p_cmd->argv[current_nr]))
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}
	strcpy(p_self->dbg.p_logfilename, p_cmd->argv[current_nr]);
	return RC_OK;
}

static RC_TYPE get_username_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	/*user*/
	if (sizeof(p_self->info[curr_info].credentials.my_username) < strlen(p_cmd->argv[current_nr]))
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}
	strcpy(p_self->info[curr_info].credentials.my_username, p_cmd->argv[current_nr]);

	return RC_OK;
}

static RC_TYPE get_password_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	/*password*/
	if (sizeof(p_self->info[curr_info].credentials.my_password) < strlen(p_cmd->argv[current_nr]))
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}

	strcpy(p_self->info[curr_info].credentials.my_password, (p_cmd->argv[current_nr]));
	return RC_OK;
}

/**
   Parses alias,hash.
   Example: blabla.domain.com,hashahashshahah
   Action:
   -search by ',' and replace the ',' with 0
   -read hash and alias
*/
static RC_TYPE get_alias_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	char *p_hash = NULL;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (p_self->info[curr_info].alias_count >= DYNDNS_MAX_ALIAS_NUMBER)
	{
		return RC_DYNDNS_TOO_MANY_ALIASES;
	}

	/*hash*/
	p_hash = strstr(p_cmd->argv[current_nr],",");
	if (p_hash)
	{
		if (sizeof(p_self->info[curr_info].alias_info[p_self->info[curr_info].alias_count].hashes) < strlen(p_hash))
		{
			return RC_DYNDNS_BUFFER_TOO_SMALL;
		}
		strcpy(p_self->info[curr_info].alias_info[p_self->info[curr_info].alias_count].hashes.str, p_hash);
		*p_hash = '\0';
	}


	/*user*/
	if (sizeof(p_self->info[curr_info].alias_info[p_self->info[curr_info].alias_count].names) < strlen(p_cmd->argv[current_nr]))
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}
	strcpy(p_self->info[curr_info].alias_info[p_self->info[curr_info].alias_count].names.name, (p_cmd->argv[current_nr]));

	p_self->info[curr_info].alias_count++;
	return RC_OK;
}

static RC_TYPE get_name_and_port(char *p_src, char *p_dest_name, int *p_dest_port)
{
	const char *p_port = NULL;
	p_port = strstr(p_src,":");
	if (p_port)
	{
		int port_nr, len;
		int port_ok = sscanf(p_port + 1, "%d",&port_nr);
		if (port_ok != 1)
		{
			return RC_DYNDNS_INVALID_OPTION;
		}
		*p_dest_port = port_nr;
		len = p_port - p_src;
		memcpy(p_dest_name, p_src, len);
		p_dest_name[len] = 0;
	}
	else
	{
		strcpy(p_dest_name, p_src);
	}
	return RC_OK;
}

/** Returns the svr name and port if the format is :
 * name[:port] url.
 */
static RC_TYPE get_ip_server_name_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	RC_TYPE rc;
	int port = -1;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	/*ip_server_name*/
	if (sizeof(p_self->info[curr_info].ip_server_name) < strlen(p_cmd->argv[current_nr]) + 1)
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}

	p_self->info[curr_info].ip_server_name.port = HTTP_DEFAULT_PORT;
	rc = get_name_and_port(p_cmd->argv[current_nr], p_self->info[curr_info].ip_server_name.name, &port);
	if (rc == RC_OK && port != -1)
	{
		p_self->info[curr_info].ip_server_name.port = port;
	}

	if (sizeof(p_self->info[curr_info].ip_server_url) < strlen(p_cmd->argv[current_nr + 1]) + 1)
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}
	strcpy(p_self->info[curr_info].ip_server_url, p_cmd->argv[current_nr + 1]);

	return rc;
}
static RC_TYPE get_dns_server_name_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	RC_TYPE rc;
	int port = -1;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	/*dyndns_server_name*/
	if (sizeof(p_self->info[curr_info].dyndns_server_name) < strlen(p_cmd->argv[current_nr]))
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}

	p_self->info[curr_info].dyndns_server_name.port = HTTP_DEFAULT_PORT;
	rc = get_name_and_port(p_cmd->argv[current_nr], p_self->info[curr_info].dyndns_server_name.name, &port);
	if (rc == RC_OK && port != -1)
	{
		p_self->info[curr_info].dyndns_server_name.port = port;
	}
	return rc;
}
RC_TYPE get_dns_server_url_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	/*dyndns_server_url*/
	if (sizeof(p_self->info[curr_info].dyndns_server_url) < strlen(p_cmd->argv[current_nr]))
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}
	strcpy(p_self->info[curr_info].dyndns_server_url, p_cmd->argv[current_nr]);
	return RC_OK;
}

/* returns the proxy server nme and port
 */
static RC_TYPE get_proxy_server_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	RC_TYPE rc;
	int port = -1;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	/*proxy_server_name*/
	if (sizeof(p_self->info[curr_info].proxy_server_name) < strlen(p_cmd->argv[current_nr]))
	{
		return  RC_DYNDNS_BUFFER_TOO_SMALL;
	}

	p_self->info[curr_info].proxy_server_name.port = HTTP_DEFAULT_PORT;
	rc = get_name_and_port(p_cmd->argv[current_nr], p_self->info[curr_info].proxy_server_name.name, &port);
	if (rc == RC_OK && port != -1)
	{
		p_self->info[curr_info].proxy_server_name.port = port;
	}
	return rc;
}
/* Read the dyndnds name update period.
   and impose the max and min limits
*/
static RC_TYPE get_update_period_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (sscanf(p_cmd->argv[current_nr], "%d", &p_self->sleep_sec) != 1)
	{
		return RC_DYNDNS_INVALID_OPTION;
	}

	p_self->sleep_sec /= 1000;
	p_self->sleep_sec = (p_self->sleep_sec < DYNDNS_MIN_SLEEP) ?  DYNDNS_MIN_SLEEP: p_self->sleep_sec;
	(p_self->sleep_sec > DYNDNS_MAX_SLEEP) ?  p_self->sleep_sec = DYNDNS_MAX_SLEEP: 1;

	return RC_OK;
}

static RC_TYPE get_update_period_sec_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (sscanf(p_cmd->argv[current_nr], "%d", &p_self->sleep_sec) != 1)
	{
		return RC_DYNDNS_INVALID_OPTION;
	}

	p_self->sleep_sec = (p_self->sleep_sec < DYNDNS_MIN_SLEEP) ?  DYNDNS_MIN_SLEEP: p_self->sleep_sec;
	(p_self->sleep_sec > DYNDNS_MAX_SLEEP) ?  p_self->sleep_sec = DYNDNS_MAX_SLEEP: 1;

	return RC_OK;
}

static RC_TYPE get_forced_update_period_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (sscanf(p_cmd->argv[current_nr], "%d", &p_self->forced_update_period_sec) != 1)
	{
		return RC_DYNDNS_INVALID_OPTION;
	}

	return RC_OK;
}

static RC_TYPE set_syslog_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	(void)p_cmd;
	(void)current_nr;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}
	p_self->debug_to_syslog = TRUE;

	return RC_OK;
}

/**
 * Reads the params for change persona. Format:
 * <uid[:gid]>
 */
static RC_TYPE set_change_persona_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	{
		gid_t gid = getuid();
		uid_t uid = getgid();

		char *p_gid = strstr(p_cmd->argv[current_nr],":");
		if (p_gid)
		{
			if ((strlen(p_gid + 1) > 0) &&  /* if something is present after :*/
			    sscanf(p_gid + 1, "%u",&gid) != 1)
			{
				return RC_DYNDNS_INVALID_OPTION;
			}
		}
		if (sscanf(p_cmd->argv[current_nr], "%u",&uid) != 1)
		{
			return RC_DYNDNS_INVALID_OPTION;
		}

		p_self->change_persona = TRUE;
		p_self->sys_usr_info.gid = gid;
		p_self->sys_usr_info.uid = uid;
	}
	return RC_OK;
}

static RC_TYPE set_bind_interface(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	p_self->interface = strdup(p_cmd->argv[current_nr]);

	return RC_OK;
}

static RC_TYPE set_pidfile(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	p_self->pidfile = strdup (p_cmd->argv[current_nr]);

	return RC_OK;
}

RC_TYPE print_version_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	(void)p_cmd;
	(void)current_nr;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	logit(LOG_INFO, "Version: %s\n", DYNDNS_VERSION_STRING);
	p_self->abort = TRUE;

	return RC_OK;
}

static RC_TYPE get_exec_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *)p_context;

	(void)p_cmd;
	(void)current_nr;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	p_self->external_command = strdup(p_cmd->argv[current_nr]);

	return RC_OK;
}

/**
   Searches the DYNDNS system by the argument.
   Input is like: system@server.name
   system=statdns|custom|dyndns|default
   server name = dyndns.org | freedns.afraid.org
   The result is a pointer in the table of DNS systems.
*/
static RC_TYPE get_dyndns_system_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	DYNDNS_SYSTEM *p_dns_system = NULL;
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	{
		DYNDNS_SYSTEM_INFO *it = get_dyndns_system_table();
		for (; it != NULL && it->id != LAST_DNS_SYSTEM; ++it)
		{
			if (strcmp(it->system.p_key, p_cmd->argv[current_nr]) == 0)
			{
				p_dns_system = &it->system;
			}
		}
	}

	if (p_dns_system == NULL)
	{
		return RC_DYNDNS_INVALID_OPTION;
	}

	for (curr_info = 0; curr_info < p_self->info_count &&
		     curr_info < DYNDNS_MAX_SERVER_NUMBER &&
		     p_self->info[curr_info].p_dns_system != p_dns_system; curr_info++)
	{
	}
	if (curr_info >= p_self->info_count)
	{
		if (curr_info < DYNDNS_MAX_SERVER_NUMBER)
		{
			p_self->info_count++;
			p_self->info[curr_info].p_dns_system = p_dns_system;
		}
	   	else
			return RC_DYNDNS_BUFFER_TOO_SMALL;
	}

	return RC_OK;
}
static RC_TYPE push_in_buffer(char* p_src, int src_len, char *p_buffer, int* p_act_len, int max_len)
{
	if (*p_act_len + src_len > max_len)
	{
		return RC_FILE_IO_OUT_OF_BUFFER;
	}
	memcpy(p_buffer + *p_act_len,p_src, src_len);
	*p_act_len += src_len;
	return RC_OK;
}

typedef enum
{
	NEW_LINE,
	COMMENT,
	DATA,
	SPACE,
	ESCAPE
} PARSER_STATE;

typedef struct
{
	FILE *p_file;
	PARSER_STATE state;
} OPTION_FILE_PARSER;

static RC_TYPE parser_init(OPTION_FILE_PARSER *p_cfg, FILE *p_file)
{
	memset(p_cfg, 0, sizeof(*p_cfg));
	p_cfg->state = NEW_LINE;
	p_cfg->p_file = p_file;
	return RC_OK;
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
static RC_TYPE parser_read_option(OPTION_FILE_PARSER *p_cfg, char *p_buffer, int maxlen)
{
	RC_TYPE rc = RC_OK;
	BOOL parse_end = FALSE;
	int count = 0;
	*p_buffer = 0;

	while (!parse_end)
	{
		char ch;
		{
			int n;
			if ((n = fscanf(p_cfg->p_file, "%c", &ch)) < 0)
			{
				if (feof(p_cfg->p_file))
				{
					break;
				}
				rc = RC_FILE_IO_READ_ERROR;
				break;
			}
		}

		switch (p_cfg->state)
		{
			case NEW_LINE:
				if (ch == '\\')
				{
					p_cfg->state = ESCAPE;
					break;
				}
				if (ch == '#') /*comment*/
				{
					p_cfg->state = COMMENT;
					break;
				}
				if (!isspace(ch))
				{
					if (ch != '-')/*add '--' to first word in line*/
					{
						if ((rc = push_in_buffer("--", 2, p_buffer, &count, maxlen)) != RC_OK)
						{
							break;
						}
					}
					if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != RC_OK)
					{
						break;
					}
					p_cfg->state = DATA;
					break;
				}
				/*skip actual leading  spaces*/
				break;

			case SPACE:
				if (ch == '\\')
				{
					p_cfg->state = ESCAPE;
					break;
				}
				if (ch == '#') /*comment*/
				{
					p_cfg->state = COMMENT;
					break;
				}
				if (ch == '\n' || ch == '\r')
				{
					p_cfg->state = NEW_LINE;
					break;
				}
				if (!isspace(ch))
				{
					if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != RC_OK)
					{
						break;
					}
					p_cfg->state = DATA;
					break;
				}
				break;

			case COMMENT:
				if (ch == '\n' || ch == '\r')
				{
					p_cfg->state = NEW_LINE;
				}
				/*skip comments*/
				break;

			case DATA:
				if (ch == '\\')
				{
					p_cfg->state = ESCAPE;
					break;
				}
				if (ch == '#')
				{
					p_cfg->state = COMMENT;
					break;
				}
				if (ch == '\n' || ch == '\r')
				{
					p_cfg->state = NEW_LINE;
					parse_end = TRUE;
					break;
				}
				if (isspace(ch))
				{
					p_cfg->state = SPACE;
					parse_end = TRUE;
					break;
				}
				/*actual data*/
				if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != RC_OK)
				{
					break;
				}
				break;
			case ESCAPE:
				if ((rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen)) != RC_OK)
				{
					break;
				}
				p_cfg->state = DATA;
				break;

			default:
				rc = RC_CMD_PARSER_INVALID_OPTION;
		}
		if (rc != RC_OK)
		{
			break;
		}
	}
	if (rc == RC_OK)
	{
		char ch = 0;
		rc = push_in_buffer(&ch, 1, p_buffer, &count, maxlen);
	}
	return rc;
}


/**
   This handler reads the data in the passed file name.
   Then appends the words in the table (cutting all spaces) to the existing cmd line options.
   It adds to the CMD_DATA struct.
   Actions:
   - open file
   - read characters and cut spaces away
   - add values one by one to the existing p_cmd data
*/
static RC_TYPE get_options_from_file_handler(CMD_DATA *p_cmd, int current_nr, void *p_context)
{
	RC_TYPE rc = RC_OK;
	DYN_DNS_CLIENT *p_self = (DYN_DNS_CLIENT *) p_context;
	FILE *p_file = NULL;
	char *p_tmp_buffer = NULL;
	const int buffer_size = DYNDNS_SERVER_NAME_LENGTH;
	OPTION_FILE_PARSER parser;

	if (!p_self || !p_cmd)
	{
		return RC_INVALID_POINTER;
	}

	do
	{
 		p_tmp_buffer = malloc(buffer_size);
   		if (!p_tmp_buffer)
	 	{
	 		rc = RC_OUT_OF_MEMORY;
	 		break;
	  	}
	  	p_file = fopen(p_cmd->argv[current_nr], "r");
	  	if (!p_file)
	  	{
			logit(LOG_ERR, MODULE_TAG "Cannot open conf file %s: \n", p_cmd->argv[current_nr], strerror(errno));
	  		rc = RC_FILE_IO_OPEN_ERROR;
	  		break;
	  	}

		/* Save for later... */
		if (p_self->cfgfile)
			free(p_self->cfgfile);
		p_self->cfgfile = strdup(p_cmd->argv[current_nr]);

		if ((rc = parser_init(&parser, p_file)) != RC_OK)
		{
			break;
		}

		while (!feof(p_file))
		{
			rc = parser_read_option(&parser,p_tmp_buffer, buffer_size);
			if (rc != RC_OK)
			{
				break;
			}

			if (!strlen(p_tmp_buffer))
			{
				break;
			}

			rc = cmd_add_val(p_cmd, p_tmp_buffer);
			if (rc != RC_OK)
			{
				break;
			}
   		}
	}
	while (0);

	if (p_file)
	{
		fclose(p_file);
	}
	if (p_tmp_buffer)
	{
		free(p_tmp_buffer);
	}

	return rc;
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
RC_TYPE get_config_data(DYN_DNS_CLIENT *p_self, int argc, char** argv)
{
	int i;
	RC_TYPE rc = RC_OK;

	do
	{
		/*load default data */
		rc = get_default_config_data(p_self);
		if (rc != RC_OK)
		{
			break;
		}
		/*set up the context pointers */
		{
			CMD_DESCRIPTION_TYPE *it = cmd_options_table;
			while (it->p_option != NULL)
			{
				it->p_handler.p_context = (void*) p_self;
				++it;
			}
		}

		/* in case of no options, assume the default cfg file may be present */
		if (argc == 1)
		{
			char *custom_argv[] = {"", DYNDNS_INPUT_FILE_OPT_STRING, DYNDNS_DEFAULT_CONFIG_FILE};
			int custom_argc = sizeof(custom_argv) / sizeof(char*);

			if (p_self->dbg.level > 0)
			{
				logit(LOG_NOTICE, MODULE_TAG "Using default config file %s\n", DYNDNS_DEFAULT_CONFIG_FILE);
			}

			if (p_self->cfgfile)
				free(p_self->cfgfile);
			p_self->cfgfile = strdup(DYNDNS_DEFAULT_CONFIG_FILE);
			rc = get_cmd_parse_data(custom_argv, custom_argc, cmd_options_table);
		}
		else
		{
			rc = get_cmd_parse_data(argv, argc, cmd_options_table);
		}

		if (rc != RC_OK || p_self->abort)
		{
			break;
		}

		/* settings that may change due to cmd line options */
		i = 0;
		do
		{
			/*ip server*/
			if (strlen(p_self->info[i].ip_server_name.name) == 0)
			{
				if (sizeof(p_self->info[i].ip_server_name.name) < strlen(p_self->info[i].p_dns_system->p_ip_server_name))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(p_self->info[i].ip_server_name.name, p_self->info[i].p_dns_system->p_ip_server_name);

				if (sizeof(p_self->info[i].ip_server_url) < strlen(p_self->info[i].p_dns_system->p_ip_server_url))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(p_self->info[i].ip_server_url, p_self->info[i].p_dns_system->p_ip_server_url);
			}

			/*dyndns server*/
			if (strlen(p_self->info[i].dyndns_server_name.name) == 0)
			{
				if (sizeof(p_self->info[i].dyndns_server_name.name) < strlen(p_self->info[i].p_dns_system->p_dyndns_server_name))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(p_self->info[i].dyndns_server_name.name, p_self->info[i].p_dns_system->p_dyndns_server_name);

				if (sizeof(p_self->info[i].dyndns_server_url) < strlen(p_self->info[i].p_dns_system->p_dyndns_server_url))
				{
					rc = RC_DYNDNS_BUFFER_TOO_SMALL;
					break;
				}
				strcpy(p_self->info[i].dyndns_server_url, p_self->info[i].p_dns_system->p_dyndns_server_url);
			}
		}
		while(++i < p_self->info_count);

		/* Check if the neccessary params have been provided */
		if ((p_self->info_count == 0) ||
		    (p_self->info[0].alias_count == 0) ||
		    (strlen(p_self->info[0].dyndns_server_name.name) == 0)  ||
		    (strlen(p_self->info[0].ip_server_name.name) == 0))
		{
			rc = RC_DYNDNS_INVALID_OR_MISSING_PARAMETERS;
			break;
		}

		/* Forced update */
		p_self->times_since_last_update = 0;
		p_self->forced_update_times = p_self->forced_update_period_sec / p_self->sleep_sec;

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
