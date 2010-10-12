/*
Copyright (C) 2003-2004 Narcis Ilisei

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*interface for main dydns functions */
#ifndef _DYNDNS_INCLUDED
#define _DYNDNS_INCLUDED

#include "os.h"
#include "errorcode.h"
#include "http_client.h"
#include "debug_if.h"

#define DYNDNS_VERSION_STRING  "1.96.2"
#define DYNDNS_AGENT_NAME  "inadyn/" DYNDNS_VERSION_STRING
#define DYNDNS_EMAIL_ADDR	"inarcis2002@hotpop.com"

typedef enum
{
    DYNDNS_DYNAMIC,
    DYNDNS_STATIC,
    DYNDNS_CUSTOM,
    DYNDNS_DEFAULT,
    FREEDNS_AFRAID_ORG_DEFAULT,
	ZONE_EDIT_DEFAULT,
	CUSTOM_HTTP_BASIC_AUTH,
	NOIP_DEFAULT,	
    LAST_DNS_SYSTEM = -1
} DYNDNS_SYSTEM_ID;

/*test values*/
#define DYNDNS_DEFAULT_DEBUG_LEVEL	1
#define DYNDNS_DEFAULT_CONFIG_FILE	"/etc/inadyn.conf"

#define DYNDNS_MY_USERNAME		"test"
#define DYNDNS_MY_PASSWD		"test"
#define DYNDNS_MY_IP_SERVER		"checkip.dyndns.org"
#define DYNDNS_MY_IP_SERVER_URL	"/"
#define DYNDNS_MY_DNS_SERVER	"members.dyndns.org"
#define DYNDNS_MY_DNS_SERVER_URL "/nic/update?"
#define DYNDNS_MY_HOST_NAME_1	"test.homeip.net"
#define DYNDNS_MY_HOST_NAME_2	"test2.homeip.net"
#define DYNDNS_HTTP_PORT		80


/*REQ/RSP definitions*/

#define DYNDNS_IP_SERVER_RESPONSE_BEGIN "Current IP Address: "
#define DYNDNS_IP_ADDR_FORMAT    "%d.%d.%d.%d"
#define DYNDNS_ALL_DIGITS		 "0123456789"


#define DYNDNS_SYSTEM_CUSTOM	"custom"
#define DYNDNS_SYSTEM_DYNAMIC	"dyndns"
#define DYNDNS_SYSTEM_STATIC	"statdns" 

#define GENERIC_DNS_IP_SERVER_NAME DYNDNS_MY_IP_SERVER
#define  DYNDNS_MY_DNS_SYSTEM  DYNDNS_DEFAULT

/* Conversation with the IP server */
#define DYNDNS_GET_MY_IP_HTTP_REQUEST  \
	"GET http://%s:%d%s HTTP/1.0\r\n\r\n"

/* dyndns.org specific update address format */	
#define DYNDNS_GET_MY_IP_HTTP_REQUEST_FORMAT \
	"GET http://%s:%d%s" \
		"system=%s&" \
		"hostname=%s&" \
		"myip=%s&" \
		"wildcard=ON&" \
		"mx=%s&" \
	   "backmx=NO&" \
	   "offline=NO " \
	 "HTTP/1.0\r\n" \
	"Host: %s\r\n" \
	"Authorization: Basic %s\r\n" \
	"User-Agent: "DYNDNS_AGENT_NAME " " DYNDNS_EMAIL_ADDR"\r\n\r\n"
    
/*freedns.afraid.org specific update request format */    
#define FREEDNS_UPDATE_MY_IP_REQUEST_FORMAT \
    "GET http://%s:%d%s" \
    "%s " \
	 "HTTP/1.0\r\n" \
	"Host: %s\r\n" \
	"User-Agent: "DYNDNS_AGENT_NAME " " DYNDNS_EMAIL_ADDR"\r\n\r\n"

/** generic update format for sites that perform the update
	with:
	http://some.address.domain/somesubdir
	?some_param_name=MY_ALIAS
	and then the normal http stuff and basic base64 encoded auth.
	The parameter here is the entire request but NOT including the alias.
*/
#define GENERIC_DNS_BASIC_AUTH_MY_IP_REQUEST_FORMAT \
    "GET http://%s:%d%s%s " \
	 "HTTP/1.0\r\n" \
	"Authorization: Basic %s\r\n" \
	"Host: %s\r\n" \
	"User-Agent: "DYNDNS_AGENT_NAME " " DYNDNS_EMAIL_ADDR"\r\n\r\n"

#define GENERIC_NOIP_AUTH_MY_IP_REQUEST_FORMAT \
    "GET http://%s:%d%s%s&myip=%s " \
	 "HTTP/1.0\r\n" \
	"Authorization: Basic %s\r\n" \
	"Host: %s\r\n" \
	"User-Agent: "DYNDNS_AGENT_NAME " " DYNDNS_EMAIL_ADDR"\r\n\r\n"


#define DYNDNS_OK_RESPONSE	"good"
#define DYNDNS_OK_NOCHANGE	"nochg"


/* SOME DEFAULT CONFIGURATIONS */
#define DYNDNS_DEFAULT_SLEEP	(120) /*s*/
#define DYNDNS_MIN_SLEEP	(30) /*s*/
#define DYNDNS_MAX_SLEEP	(10 * 24 * 3600) /*10 days in s*/
#define DYNDNS_MY_FORCED_UPDATE_PERIOD_S   (30 * 24 * 3600) /* 30 days in sec*/
#define DYNDNS_DEFAULT_CMD_CHECK_PERIOD	(1) /*s*/
#define DYNDNS_DEFAULT_ITERATIONS	0 /*forever*/
#define DYNDNS_HTTP_RESPONSE_BUFFER_SIZE	(2500) /*Bytes*/
#define DYNDNS_HTTP_REQUEST_BUFFER_SIZE		(2500) /*Bytes*/
#define DYNDNS_MAX_ALIAS_NUMBER		10 /*maximum number of aliases that can be maintained*/

/*local configs*/
#define DYNDNS_MY_IP_ADDRESS_LENGTH	20 /*chars*/
#define DYNDNS_MY_USERNAME_LENGTH	50 /*chars*/
#define DYNDNS_MY_PASSWORD_LENGTH	50 /*chars*/
#define DYNDNS_SERVER_NAME_LENGTH	256 /*chars*/
#define DYNDNS_SERVER_URL_LENGTH	256 /*chars*/
#define DYNDNS_HASH_STRING_MAX_LENGTH	256 /*chars*/
#define IP_V4_MAX_LENGTH            16 /*chars: nnn.nnn.nnn.nnn\0*/


/* typedefs */
struct _DYN_DNS_CLIENT;
struct DYNDNS_SYSTEM;

/** Types used for DNS system specific configuration 
*/
/** Function to prepare DNS system specific server requests 
*/

typedef int (*DNS_SYSTEM_REQUEST_FUNC)(struct _DYN_DNS_CLIENT *this, int nr, struct DYNDNS_SYSTEM *p_sys_info);
typedef int (*DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)(struct _DYN_DNS_CLIENT *this, char *p_rsp, const char*p_ok_str);
typedef struct 
{
    const char* p_key;
    void* p_specific_data;
    DNS_SYSTEM_SRV_RESPONSE_OK_FUNC p_rsp_ok_func;
    DNS_SYSTEM_REQUEST_FUNC p_dns_update_req_func;
    const char *p_ip_server_name;
	const char *p_ip_server_url;
    const char *p_dyndns_server_name;
	const char *p_dyndns_server_url;
	const char *p_success_string;
}DYNDNS_SYSTEM;

typedef struct 
{
    DYNDNS_SYSTEM_ID id;
    DYNDNS_SYSTEM system;
} DYNDNS_SYSTEM_INFO;

typedef struct 
{
    const char *p_system;
} DYNDNS_ORG_SPECIFIC_DATA;

typedef enum 
{
	NO_CMD = 0,
	CMD_STOP = 1
} DYN_DNS_CMD;


typedef struct
{
    char str[DYNDNS_HASH_STRING_MAX_LENGTH];
} DYNDNS_HASH_TYPE;

typedef struct 
{
	char my_username[DYNDNS_MY_USERNAME_LENGTH];
	char my_password[DYNDNS_MY_PASSWORD_LENGTH];
	char *p_enc_usr_passwd_buffer;
	int size;
	BOOL encoded;
} DYNDNS_CREDENTIALS;

typedef struct 
{
	char name[DYNDNS_SERVER_NAME_LENGTH];
    int port;
} DYNDNS_SERVER_NAME;

typedef struct 
{
    BOOL my_ip_has_changed;
	DYNDNS_SERVER_NAME my_ip_address;
	DYNDNS_CREDENTIALS credentials;
	DYNDNS_SYSTEM *p_dns_system;
	DYNDNS_SERVER_NAME dyndns_server_name;
	char dyndns_server_url[DYNDNS_SERVER_URL_LENGTH];
	DYNDNS_SERVER_NAME ip_server_name;
	char ip_server_url[DYNDNS_SERVER_URL_LENGTH];
	DYNDNS_SERVER_NAME proxy_server_name;
} DYNDNS_INFO_TYPE;

typedef struct 
{
	DYNDNS_SERVER_NAME names[DYNDNS_MAX_ALIAS_NUMBER];
	int update_required[DYNDNS_MAX_ALIAS_NUMBER];
	DYNDNS_HASH_TYPE hashes[DYNDNS_MAX_ALIAS_NUMBER]; 
	int count;
} DYNDNS_ALIAS_INFO;

typedef struct
{
	int uid;
	int gid;
} USER_INFO;

typedef struct DYN_DNS_CLIENT
{
	DYN_DNS_CMD cmd;
	int sleep_sec; /* time between 2 updates*/
	int forced_update_period_sec; 
	int times_since_last_update;
	int forced_update_times; /* the same forced update period counted in sleep periods*/
	int cmd_check_period; /*time to wait for a command*/
	int total_iterations;
	BOOL initialized;
	BOOL run_in_background;
	BOOL debug_to_syslog;
	BOOL change_persona;

	HTTP_CLIENT http_to_ip_server;
	HTTP_CLIENT http_to_dyndns;
	HTTP_TRANSACTION http_tr;
	char *p_work_buffer; /* for HTTP responses*/
	int work_buffer_size;
	char *p_req_buffer; /* for HTTP requests*/
	int req_buffer_size;


	USER_INFO sys_usr_info; /*info about the current account running inadyn*/
	DYNDNS_INFO_TYPE info; /*servers, names, passwd*/
	DYNDNS_ALIAS_INFO alias_info;

	BOOL abort_on_network_errors;
	BOOL force_addr_update;
    BOOL use_proxy;
	BOOL abort;

	/*dbg*/
	DBG_TYPE dbg;
} DYN_DNS_CLIENT;


/*public functions*/
/** Returns the table of supported dyndns systems
*/
DYNDNS_SYSTEM_INFO* get_dyndns_system_table(void);

/**
 *  Returns the default DYNDNS client config data.
*/
RC_TYPE get_default_config_data(DYN_DNS_CLIENT *p_self);

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
*/
RC_TYPE get_config_data(DYN_DNS_CLIENT *p_self, int argc, char** argv);

/*
	printout of version
*/
void dyn_dns_print_hello(void*p);

/*
	 basic resource allocations for the dyn_dns object
*/
RC_TYPE dyn_dns_construct(DYN_DNS_CLIENT **pp_self);

/*
	Resource free.
*/	
RC_TYPE dyn_dns_destruct(DYN_DNS_CLIENT *p_self);

/* 
	Sets up the object.
	- sets the IPs of the DYN DNS server 
	- ...
*/
RC_TYPE dyn_dns_init(DYN_DNS_CLIENT *p_self);

/* 
	Disconnect and some other clean up.
*/
RC_TYPE dyn_dns_shutdown(DYN_DNS_CLIENT *p_self);



/* the real action:
	- detect current IP
		- connect to an HTTP server 
		- parse the response for IP addr

	- for all the names that have to be maintained
		- get the current DYN DNS address from DYN DNS server
		- compare and update if neccessary
*/
RC_TYPE dyn_dns_update_ip(DYN_DNS_CLIENT *p_self);

/* MAIN - Dyn DNS update entry point.*/

/* 
	Actions:
		- read the configuration options
		- create and init dyn_dns object.
		- launch the IP update action
*/		
int dyn_dns_main(DYN_DNS_CLIENT *p_self, int argc, char* argv[]);


/*
	help.
*/
void print_help_page(void);

/*
    main entry point
*/
int inadyn_main(int argc, char* argv[]);

#endif /*_DYNDNS_INCLUDED*/
