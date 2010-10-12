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
/*
Copyright (C) 2003-2006 INAtech
Author: Narcis Ilisei

*/
/**
	Dyn Dns update main implementation file 
	Author: narcis Ilisei
	Date: May 2003

	History:
		- first implemetnation
		- 18 May 2003 : cmd line option reading added - 
        - Nov 2003 - new version
        - April 2004 - freedns.afraid.org system added.
        - October 2004 - Unix syslog capability added.
*/
#define MODULE_TAG      "INADYN: "  
#include <stdlib.h>
#include <string.h>
#include "dyndns.h"
#include "debug_if.h"
#include "base64.h"
#include "get_cmd.h"

/* DNS systems specific configurations*/

DYNDNS_ORG_SPECIFIC_DATA dyndns_org_dynamic = {"dyndns"};
DYNDNS_ORG_SPECIFIC_DATA dyndns_org_custom = {"custom"};
DYNDNS_ORG_SPECIFIC_DATA dyndns_org_static = {"statdns"};

static int get_req_for_dyndns_server(DYN_DNS_CLIENT *this, int nr, DYNDNS_SYSTEM *p_sys_info);
static int get_req_for_freedns_server(DYN_DNS_CLIENT *p_self, int cnt,  DYNDNS_SYSTEM *p_sys_info);
static int get_req_for_generic_http_dns_server(DYN_DNS_CLIENT *p_self, int cnt,  DYNDNS_SYSTEM *p_sys_info);
static int get_req_for_noip_http_dns_server(DYN_DNS_CLIENT *p_self, int cnt,  DYNDNS_SYSTEM *p_sys_info);

static BOOL is_dyndns_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string);
static BOOL is_freedns_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string);
static BOOL is_generic_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string);
static BOOL is_zoneedit_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string);

DYNDNS_SYSTEM_INFO dns_system_table[] = 
{ 
    {DYNDNS_DEFAULT, 
        {"default@dyndns.org", &dyndns_org_dynamic, 
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_dyndns_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC)get_req_for_dyndns_server,
             DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
			DYNDNS_MY_DNS_SERVER, DYNDNS_MY_DNS_SERVER_URL, NULL}},
    {DYNDNS_DYNAMIC, 
        {"dyndns@dyndns.org", &dyndns_org_dynamic,  
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_dyndns_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC) get_req_for_dyndns_server,
             DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
			DYNDNS_MY_DNS_SERVER, DYNDNS_MY_DNS_SERVER_URL, NULL}},    
    {DYNDNS_CUSTOM, 
        {"custom@dyndns.org", &dyndns_org_custom,  
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_dyndns_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC) get_req_for_dyndns_server,
             DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
			DYNDNS_MY_DNS_SERVER, DYNDNS_MY_DNS_SERVER_URL, NULL}},
    {DYNDNS_STATIC, 
        {"statdns@dyndns.org", &dyndns_org_static,  
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_dyndns_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC) get_req_for_dyndns_server,
             DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
				DYNDNS_MY_DNS_SERVER, DYNDNS_MY_DNS_SERVER_URL, NULL}},

    {FREEDNS_AFRAID_ORG_DEFAULT, 
        {"default@freedns.afraid.org", NULL,  
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_freedns_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC) get_req_for_freedns_server,
            DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
			"freedns.afraid.org", "/dynamic/update.php?", NULL}},

    {ZONE_EDIT_DEFAULT, 
        {"default@zoneedit.com", NULL,  
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_zoneedit_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC) get_req_for_generic_http_dns_server,
            "dynamic.zoneedit.com", "/checkip.html", 
			"dynamic.zoneedit.com", "/auth/dynamic.html?host=", ""}},

    {NOIP_DEFAULT, 
        {"default@no-ip.com", NULL,  
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_dyndns_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC) get_req_for_noip_http_dns_server,
            "ip1.dynupdate.no-ip.com", "/", 
			"dynupdate.no-ip.com", "/nic/update?hostname=", ""}},

    {CUSTOM_HTTP_BASIC_AUTH, 
        {"custom@http_svr_basic_auth", NULL,  
            (DNS_SYSTEM_SRV_RESPONSE_OK_FUNC)is_generic_server_rsp_ok, 
            (DNS_SYSTEM_REQUEST_FUNC) get_req_for_generic_http_dns_server,
            GENERIC_DNS_IP_SERVER_NAME, DYNDNS_MY_IP_SERVER_URL,
			"", "", "OK"}},

    {LAST_DNS_SYSTEM, {NULL, NULL, NULL, NULL, NULL, NULL}}
};

static DYNDNS_SYSTEM* get_dns_system_by_id(DYNDNS_SYSTEM_ID id)
{
    {
        DYNDNS_SYSTEM_INFO *it;
        for (it = dns_system_table; it->id != LAST_DNS_SYSTEM; ++it)
        {
            if (it->id == id)
            {
                return &it->system;
            }
        }    
    } 
    return NULL;
}

DYNDNS_SYSTEM_INFO* get_dyndns_system_table(void)
{
    return dns_system_table;
}

/*************PRIVATE FUNCTIONS ******************/
static RC_TYPE dyn_dns_wait_for_cmd(DYN_DNS_CLIENT *p_self)
{
	int counter = p_self->sleep_sec / p_self->cmd_check_period;
	DYN_DNS_CMD old_cmd = p_self->cmd;

	if (old_cmd != NO_CMD)
	{
		return RC_OK;
	}

	while( counter --)
	{
		if (p_self->cmd != old_cmd)
		{
			break;
		}
		os_sleep_ms(p_self->cmd_check_period * 1000);
	}
	return RC_OK;
}

static int get_req_for_dyndns_server(DYN_DNS_CLIENT *p_self, int cnt,DYNDNS_SYSTEM *p_sys_info)
{	
    DYNDNS_ORG_SPECIFIC_DATA *p_dyndns_specific = 
		(DYNDNS_ORG_SPECIFIC_DATA*) p_sys_info->p_specific_data;
	return sprintf(p_self->p_req_buffer, DYNDNS_GET_MY_IP_HTTP_REQUEST_FORMAT,
        p_self->info.dyndns_server_name.name,
        p_self->info.dyndns_server_name.port,
		p_self->info.dyndns_server_url,
		p_dyndns_specific->p_system,
		p_self->alias_info.names[cnt].name,
		p_self->info.my_ip_address.name,
		p_self->alias_info.names[cnt].name,
        p_self->info.dyndns_server_name.name,
		p_self->info.credentials.p_enc_usr_passwd_buffer
		);
}

static int get_req_for_freedns_server(DYN_DNS_CLIENT *p_self, int cnt, DYNDNS_SYSTEM *p_sys_info)
{
	(void)p_sys_info;
	return sprintf(p_self->p_req_buffer, FREEDNS_UPDATE_MY_IP_REQUEST_FORMAT,
        p_self->info.dyndns_server_name.name,
        p_self->info.dyndns_server_name.port,
		p_self->info.dyndns_server_url,
		p_self->alias_info.hashes[cnt].str,
        p_self->info.dyndns_server_name.name);
}


static int get_req_for_generic_http_dns_server(DYN_DNS_CLIENT *p_self, int cnt, DYNDNS_SYSTEM *p_sys_info)
{
	(void)p_sys_info;
	return sprintf(p_self->p_req_buffer, GENERIC_DNS_BASIC_AUTH_MY_IP_REQUEST_FORMAT,
        p_self->info.dyndns_server_name.name,
        p_self->info.dyndns_server_name.port,
		p_self->info.dyndns_server_url,		
		p_self->alias_info.names[cnt].name,
        p_self->info.credentials.p_enc_usr_passwd_buffer,
		p_self->info.dyndns_server_name.name);
}
static int get_req_for_noip_http_dns_server(DYN_DNS_CLIENT *p_self, int cnt,  DYNDNS_SYSTEM *p_sys_info)
{
	(void)p_sys_info;
	return sprintf(p_self->p_req_buffer, GENERIC_NOIP_AUTH_MY_IP_REQUEST_FORMAT,
        p_self->info.dyndns_server_name.name,
        p_self->info.dyndns_server_name.port,
		p_self->info.dyndns_server_url,		
		p_self->alias_info.names[cnt].name,
		p_self->info.my_ip_address.name,
        p_self->info.credentials.p_enc_usr_passwd_buffer,
		p_self->info.dyndns_server_name.name		
		);
}

static int get_req_for_ip_server(DYN_DNS_CLIENT *p_self, void *p_specific_data)
{
    return sprintf(p_self->p_req_buffer, DYNDNS_GET_MY_IP_HTTP_REQUEST,
        p_self->info.ip_server_name.name, p_self->info.ip_server_name.port, p_self->info.ip_server_url);
}

/* 
	Send req to IP server and get the response
*/
static RC_TYPE do_ip_server_transaction(DYN_DNS_CLIENT *p_self)
{
	RC_TYPE rc = RC_OK;
	HTTP_CLIENT *p_http;

	p_http = &p_self->http_to_ip_server;

	rc = http_client_init(&p_self->http_to_ip_server);
	if (rc != RC_OK)
	{
		return rc;
	}

	do
	{
		/*prepare request for IP server*/
		{
			HTTP_TRANSACTION *p_tr = &p_self->http_tr;

            p_tr->req_len = get_req_for_ip_server((DYN_DNS_CLIENT*) p_self,
                                                     p_self->info.p_dns_system->p_specific_data);
			if (p_self->dbg.level > 2) 
			{
				DBG_PRINTF((LOG_DEBUG, "The request for IP server:\n%s\n",p_self->p_req_buffer));
			}
            p_tr->p_req = (char*) p_self->p_req_buffer;		
			p_tr->p_rsp = (char*) p_self->p_work_buffer;
			p_tr->max_rsp_len = p_self->work_buffer_size - 1;/*save place for a \0 at the end*/
			p_tr->rsp_len = 0;

			rc = http_client_transaction(&p_self->http_to_ip_server, &p_self->http_tr);		
			p_self->p_work_buffer[p_tr->rsp_len] = 0;
		}
	}
	while(0);

	/*close*/
	http_client_shutdown(&p_self->http_to_ip_server);
	
	return rc;
}


/* 
	Read in 4 integers the ip addr numbers.
	construct then the IP address from those numbers.
    Note:
        it updates the flag: info->'my_ip_has_changed' if the old address was different 
*/
static RC_TYPE do_parse_my_ip_address(DYN_DNS_CLIENT *p_self)
{
	int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
	int count;
	char *p_ip;
	char *p_current_str = p_self->http_tr.p_rsp;
	BOOL found;
    char new_ip_str[IP_V4_MAX_LENGTH];

	if (p_self->http_tr.rsp_len <= 0 || 
		p_self->http_tr.p_rsp == NULL)
	{
		return RC_INVALID_POINTER;
	}

	found = FALSE;
	do
	{
		/*try to find first decimal number (begin of IP)*/
		p_ip = strpbrk(p_current_str, DYNDNS_ALL_DIGITS);
		if (p_ip != NULL)
		{
			/*maybe I found it*/
			count = sscanf(p_ip, DYNDNS_IP_ADDR_FORMAT,
							&ip1, &ip2, &ip3, &ip4);
			if (count != 4 ||
				ip1 <= 0 || ip1 > 255 ||
				ip2 < 0 || ip2 > 255 ||
				ip3 < 0 || ip3 > 255 ||
				ip4 < 0 || ip4 > 255 )
			{
				p_current_str = p_ip + 1;
			}
			else
			{
				/* FIRST occurence of a valid IP found*/
				found = TRUE;
				break;
			}
		}
	}
	while(p_ip != NULL);
		   

	if (found)
	{        
        sprintf(new_ip_str, DYNDNS_IP_ADDR_FORMAT, ip1, ip2, ip3, ip4);
        p_self->info.my_ip_has_changed = (strcmp(new_ip_str, p_self->info.my_ip_address.name) != 0);
		strcpy(p_self->info.my_ip_address.name, new_ip_str);
		return RC_OK;
	}
	else
	{
		return RC_DYNDNS_INVALID_RSP_FROM_IP_SERVER;
	}	
}

/*
    Updates for every maintained name the property: 'update_required'.
    The property will be checked in another function and updates performed.
        
      Action:
        Check if my IP address has changed. -> ALL names have to be updated.
        Nothing else.
        Note: In the update function the property will set to false if update was successful.
*/
static RC_TYPE do_check_alias_update_table(DYN_DNS_CLIENT *p_self)
{
	int i;	

    if (p_self->info.my_ip_has_changed ||
        p_self->force_addr_update ||
	(p_self->times_since_last_update > p_self->forced_update_times)
       )
    {
        for (i = 0; i < p_self->alias_info.count; ++i)
	    {
            p_self->alias_info.update_required[i] = TRUE;
			{
				DBG_PRINTF((LOG_WARNING,"I:" MODULE_TAG "IP address for alias '%s' needs update to '%s'\n",
					p_self->alias_info.names[i].name,
					p_self->info.my_ip_address.name ));
			}
        }
    }
    return RC_OK;
}


/* DynDNS org.specific response validator.
    'good' or 'nochange' are the good answers,
*/
static BOOL is_dyndns_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string)
{
	(void) p_ok_string;
    return ( (strstr(p_rsp, DYNDNS_OK_RESPONSE) != NULL) ||
             (strstr(p_rsp, DYNDNS_OK_NOCHANGE) != NULL) );
}

/* Freedns afraid.org.specific response validator.
    ok blabla and n.n.n.n
    fail blabla and n.n.n.n
    are the good answers. We search our own IP address in response and that's enough.
*/
static BOOL is_freedns_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string)
{
	(void) p_ok_string;
    return (strstr(p_rsp, p_self->info.my_ip_address.name) != NULL);
}

/** generic http dns server ok parser 
	parses a given string. If found is ok,
	Example : 'SUCCESS CODE='
*/
static BOOL is_generic_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string)
{
	if (p_ok_string == NULL)
	{
		return FALSE;
	}
    return (strstr(p_rsp, p_ok_string) != NULL);
}

/**
	the OK codes are:
		CODE=200
		CODE=707, for duplicated updates
*/
BOOL is_zoneedit_server_rsp_ok( DYN_DNS_CLIENT *p_self, char*p_rsp, char* p_ok_string)
{
	return 
	(		
		(strstr(p_rsp, "CODE=\"200\"") != NULL) ||
		(strstr(p_rsp, "CODE=\"707\"") != NULL)
	);	
}

static RC_TYPE do_update_alias_table(DYN_DNS_CLIENT *p_self)
{
	int i;
	RC_TYPE rc = RC_OK;
	
	do 
	{			
		for (i = 0; i < p_self->alias_info.count; ++i)
		{
			if (p_self->alias_info.update_required[i] != TRUE)
			{
				continue;
			}	
			
			rc = http_client_init(&p_self->http_to_dyndns);
			if (rc != RC_OK)
			{
				break;
			}
			
			/*build dyndns transaction*/
			{
				HTTP_TRANSACTION http_tr;
				http_tr.req_len = p_self->info.p_dns_system->p_dns_update_req_func(
                        (struct _DYN_DNS_CLIENT*) p_self,i,
						(struct DYNDNS_SYSTEM*) p_self->info.p_dns_system);
				http_tr.p_req = (char*) p_self->p_req_buffer;
				http_tr.p_rsp = (char*) p_self->p_work_buffer;
				http_tr.max_rsp_len = p_self->work_buffer_size - 1;/*save place for a \0 at the end*/
				http_tr.rsp_len = 0;
				p_self->p_work_buffer[http_tr.rsp_len+1] = 0;
				
				/*send it*/
				rc = http_client_transaction(&p_self->http_to_dyndns, &http_tr);					

				if (p_self->dbg.level > 2)
				{
					p_self->p_req_buffer[http_tr.req_len] = 0;
					DBG_PRINTF((LOG_DEBUG,"DYNDNS my Request:\n%s\n", p_self->p_req_buffer));
				}

				if (rc == RC_OK)
				{
					BOOL update_ok = 
                        p_self->info.p_dns_system->p_rsp_ok_func((struct _DYN_DNS_CLIENT*)p_self, 
                            http_tr.p_rsp, 
							p_self->info.p_dns_system->p_success_string);
					if (update_ok)
					{
			                        p_self->alias_info.update_required[i] = FALSE;

						DBG_PRINTF((LOG_WARNING,"I:" MODULE_TAG "Alias '%s' to IP '%s' updated successful.\n", 
							p_self->alias_info.names[i].name,
							p_self->info.my_ip_address.name));                        
						p_self->times_since_last_update = 0;
							
					}
					else
					{
						DBG_PRINTF((LOG_WARNING,"W:" MODULE_TAG "Error validating DYNDNS svr answer. Check usr,pass,hostname,abuse...!\n", http_tr.p_rsp));
                        rc = RC_DYNDNS_RSP_NOTOK;						
					}
					if (p_self->dbg.level > 2 || !update_ok)
					{							
						http_tr.p_rsp[http_tr.rsp_len] = 0;
						DBG_PRINTF((LOG_WARNING,"W:" MODULE_TAG "DYNDNS Server response:\n%s\n", http_tr.p_rsp));
					}
				}
			}
			
			{
				RC_TYPE rc2 = http_client_shutdown(&p_self->http_to_dyndns);
				if (rc == RC_OK)
				{
					rc = rc2;
				}			
			}
			if (rc != RC_OK)
			{
				break;
			}
			os_sleep_ms(1000);
		}
		if (rc != RC_OK)
		{
			break;
		}
	}
	while(0);
	return rc;
}


RC_TYPE get_default_config_data(DYN_DNS_CLIENT *p_self)
{
	RC_TYPE rc = RC_OK;

	do
	{
        p_self->info.p_dns_system = get_dns_system_by_id(DYNDNS_MY_DNS_SYSTEM);	
        if (p_self->info.p_dns_system == NULL)
        {
            rc = RC_DYNDNS_INVALID_DNS_SYSTEM_DEFAULT;
            break;
        }
						
		/*forced update period*/
		p_self->forced_update_period_sec = DYNDNS_MY_FORCED_UPDATE_PERIOD_S;
		/*update period*/
		p_self->sleep_sec = DYNDNS_DEFAULT_SLEEP;					
	}
	while(0);
	
	return rc;
}


static RC_TYPE get_encoded_user_passwd(DYN_DNS_CLIENT *p_self)
{
	RC_TYPE rc = RC_OK;
	const char* format = "%s:%s";
	char *p_tmp_buff = NULL;
	int size = strlen(p_self->info.credentials.my_password) + 
			   strlen(p_self->info.credentials.my_username) + 
			   strlen(format) + 1;
	int actual_len;

	do
	{
		p_tmp_buff = (char *) malloc(size);
		if (p_tmp_buff == NULL)
		{
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		actual_len = sprintf(p_tmp_buff, format, 
				p_self->info.credentials.my_username, 
				p_self->info.credentials.my_password);
		if (actual_len >= size)
		{
			rc = RC_OUT_BUFFER_OVERFLOW;
			break;
		}

		/*encode*/
		p_self->info.credentials.p_enc_usr_passwd_buffer = b64encode(p_tmp_buff);	
		p_self->info.credentials.encoded = 
			(p_self->info.credentials.p_enc_usr_passwd_buffer != NULL);
		p_self->info.credentials.size = strlen(p_self->info.credentials.p_enc_usr_passwd_buffer);
	}
	while(0);

	if (p_tmp_buff != NULL)
	{
		free(p_tmp_buff);
	}
	return rc;	
}

/*************PUBLIC FUNCTIONS ******************/

/*
	printout
*/
void dyn_dns_print_hello(void*p)
{
	(void) p;

    DBG_PRINTF((LOG_INFO, MODULE_TAG "Started 'INADYN version %s' - dynamic DNS updater.\n", DYNDNS_VERSION_STRING));
}
/**
	 basic resource allocations for the dyn_dns object
*/
RC_TYPE dyn_dns_construct(DYN_DNS_CLIENT **pp_self)
{
	RC_TYPE rc;
	DYN_DNS_CLIENT *p_self;
    BOOL http_to_dyndns_constructed = FALSE;
    BOOL http_to_ip_constructed = FALSE;

	if (pp_self == NULL)
	{
		return RC_INVALID_POINTER;
	}
	/*alloc space for me*/
	*pp_self = (DYN_DNS_CLIENT *) malloc(sizeof(DYN_DNS_CLIENT));
	if (*pp_self == NULL)
	{
		return RC_OUT_OF_MEMORY;
	}

	do
	{
		p_self = *pp_self;
		memset(p_self, 0, sizeof(DYN_DNS_CLIENT));
	
		/*alloc space for http_to_ip_server data*/
		p_self->work_buffer_size = DYNDNS_HTTP_RESPONSE_BUFFER_SIZE;
		p_self->p_work_buffer = (char*) malloc(p_self->work_buffer_size);
		if (p_self->p_work_buffer == NULL)
		{
			rc = RC_OUT_OF_MEMORY;
			break;		
   		}
	
		/*alloc space for request data*/
		p_self->req_buffer_size = DYNDNS_HTTP_REQUEST_BUFFER_SIZE;
		p_self->p_req_buffer = (char*) malloc(p_self->req_buffer_size);
		if (p_self->p_req_buffer == NULL)
		{
			rc = RC_OUT_OF_MEMORY;
			break;
		}
		
        
		rc = http_client_construct(&p_self->http_to_ip_server);
		if (rc != RC_OK)
		{	
			rc = RC_OUT_OF_MEMORY;
			break;
		}
        http_to_ip_constructed = TRUE;
	
		rc = http_client_construct(&p_self->http_to_dyndns);
		if (rc != RC_OK)
		{		
			rc = RC_OUT_OF_MEMORY;
			break;
		}
        http_to_dyndns_constructed = TRUE;
	
		(p_self)->cmd = NO_CMD;
		(p_self)->sleep_sec = DYNDNS_DEFAULT_SLEEP;
		(p_self)->total_iterations = DYNDNS_DEFAULT_ITERATIONS;	
		(p_self)->initialized = FALSE;
	
		p_self->info.credentials.p_enc_usr_passwd_buffer = NULL;

	}
	while(0);

    if (rc != RC_OK)
    {
        if (*pp_self)
        {
            free(*pp_self);
        }
        if (p_self->p_work_buffer != NULL)
        {
            free(p_self->p_work_buffer);
        }
        if (p_self->p_req_buffer != NULL)
        {
            free (p_self->p_work_buffer);
        }
        if (http_to_dyndns_constructed) 
        {
            http_client_destruct(&p_self->http_to_dyndns);
        }
        if (http_to_ip_constructed) 
        {
            http_client_destruct(&p_self->http_to_ip_server);
        }            
    }

	return RC_OK;
}


/**
	Resource free.
*/	
RC_TYPE dyn_dns_destruct(DYN_DNS_CLIENT *p_self)
{
	RC_TYPE rc;
	if (p_self == NULL)
	{
		return RC_OK;
	}

	if (p_self->initialized == TRUE)
	{
		dyn_dns_shutdown(p_self);
	}

	rc = http_client_destruct(&p_self->http_to_ip_server);
	if (rc != RC_OK)
	{		
		
	}

	rc = http_client_destruct(&p_self->http_to_dyndns);
	if (rc != RC_OK)
	{	
		
	}

	if (p_self->p_work_buffer != NULL)
	{
		free(p_self->p_work_buffer);
		p_self->p_work_buffer = NULL;
	}

	if (p_self->p_req_buffer != NULL)
	{
		free(p_self->p_req_buffer);
		p_self->p_req_buffer = NULL;
	}

	if (p_self->info.credentials.p_enc_usr_passwd_buffer != NULL)
	{
		free(p_self->info.credentials.p_enc_usr_passwd_buffer);
		p_self->info.credentials.p_enc_usr_passwd_buffer = NULL;
	}


	free(p_self);
	p_self = NULL;

	return RC_OK;
}

/** 
	Sets up the object.
	- sets the IPs of the DYN DNS server
    - if proxy server is set use it! 
	- ...
*/
RC_TYPE dyn_dns_init(DYN_DNS_CLIENT *p_self)
{
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (p_self->initialized == TRUE)
	{
		return RC_OK;
	}

	p_self->abort_on_network_errors = FALSE;
	p_self->force_addr_update = FALSE;

    if (strlen(p_self->info.proxy_server_name.name) > 0)
    {
        http_client_set_port(&p_self->http_to_ip_server, p_self->info.proxy_server_name.port);
        http_client_set_remote_name(&p_self->http_to_ip_server, p_self->info.proxy_server_name.name);

        http_client_set_port(&p_self->http_to_dyndns, p_self->info.proxy_server_name.port);
        http_client_set_remote_name(&p_self->http_to_dyndns, p_self->info.proxy_server_name.name);
    }
    else
    {
        http_client_set_port(&p_self->http_to_ip_server, p_self->info.ip_server_name.port);
        http_client_set_remote_name(&p_self->http_to_ip_server, p_self->info.ip_server_name.name);

        http_client_set_port(&p_self->http_to_dyndns, p_self->info.dyndns_server_name.port);
        http_client_set_remote_name(&p_self->http_to_dyndns, p_self->info.dyndns_server_name.name);    
    }

	p_self->cmd = NO_CMD;
    if (p_self->cmd_check_period == 0)
    {
	    p_self->cmd_check_period = DYNDNS_DEFAULT_CMD_CHECK_PERIOD;
    }


	p_self->initialized = TRUE;
	return RC_OK;
}

/** 
	Disconnect and some other clean up.
*/
RC_TYPE dyn_dns_shutdown(DYN_DNS_CLIENT *p_self)
{
	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	if (p_self->initialized == FALSE)
	{
		return RC_OK;
	}

	return RC_OK;
}

/** the real action:
	- increment the forced update times counter
	- detect current IP
		- connect to an HTTP server 
		- parse the response for IP addr

	- for all the names that have to be maintained
		- get the current DYN DNS address from DYN DNS server
		- compare and update if neccessary
*/
RC_TYPE dyn_dns_update_ip(DYN_DNS_CLIENT *p_self)
{
	RC_TYPE rc;

	if (p_self == NULL)
	{
		return RC_INVALID_POINTER;
	}

	do
	{
		/*ask IP server something so he will respond and give me my IP */
		rc = do_ip_server_transaction(p_self);
		if (rc != RC_OK)
		{
			DBG_PRINTF((LOG_WARNING,"W: DYNDNS: Error '%s' (0x%x) when talking to IP server\n",
				errorcode_get_name(rc), rc));
			break;
		}
		if (p_self->dbg.level > 1)
		{
			DBG_PRINTF((LOG_DEBUG,"DYNDNS: IP server response: %s\n", p_self->p_work_buffer));		
		}

		/*extract my IP, check if different than previous one*/
		rc = do_parse_my_ip_address(p_self);
		if (rc != RC_OK)
		{	
			break;
		}
		
		if (p_self->dbg.level > 1)
		{
			DBG_PRINTF((LOG_WARNING,"W: DYNDNS: My IP address: %s\n", p_self->info.my_ip_address.name));		
		}

		/*step through aliases list, resolve them and check if they point to my IP*/
		rc = do_check_alias_update_table(p_self);
		if (rc != RC_OK)
		{	
			break;
		}

		/*update IPs marked as not identical with my IP*/
		rc = do_update_alias_table(p_self);
		if (rc != RC_OK)
		{	
			break;
		}
	}
	while(0);

	return rc;
}


/** MAIN - Dyn DNS update entry point 
	Actions:
		- read the configuration options
		- perform various init actions as specified in the options
		- create and init dyn_dns object.
		- launch the IP update action loop
*/		
int dyn_dns_main(DYN_DNS_CLIENT *p_dyndns, int argc, char* argv[])
{
	RC_TYPE rc = RC_OK;
	int iterations = 0;
	BOOL os_handler_installed = FALSE;

	if (p_dyndns == NULL)
	{
		return RC_INVALID_POINTER;
	}

	/* read cmd line options and set object properties*/
	rc = get_config_data(p_dyndns, argc, argv);
	if (rc != RC_OK || p_dyndns->abort)
	{
		return rc;
	}

    /*if logfile provided, redirect output to log file*/
    if (strlen(p_dyndns->dbg.p_logfilename) != 0)
    {
        rc = os_open_dbg_output(DBG_FILE_LOG, "", p_dyndns->dbg.p_logfilename);
        if (rc != RC_OK)
        {
            return rc;
        }
    }

	if (p_dyndns->debug_to_syslog == TRUE ||
       (p_dyndns->run_in_background == TRUE))
	{
		if (get_dbg_dest() == DBG_STD_LOG) /*avoid file and syslog output */
        {
            rc = os_open_dbg_output(DBG_SYS_LOG, "INADYN", NULL);
            if (rc != RC_OK)
            {
                return rc;
            }
        }
	}

	if (p_dyndns->change_persona)
	{
		OS_USER_INFO os_usr_info;
		memset(&os_usr_info, 0, sizeof(os_usr_info));
		os_usr_info.gid = p_dyndns->sys_usr_info.gid;
		os_usr_info.uid = p_dyndns->sys_usr_info.uid;
		rc = os_change_persona(&os_usr_info);
		if (rc != RC_OK)
		{
			return rc;
		}
	}

    /*if silent required, close console window*/
    if (p_dyndns->run_in_background == TRUE)
    {
        rc = close_console_window();
        if (rc != RC_OK)
        {
            return rc;
        }       
        if (get_dbg_dest() == DBG_SYS_LOG)
        {
            fclose(stdout);
        }
    }

    dyn_dns_print_hello(NULL);

	/* the real work here */
	do
	{
		/* init object */			
		rc = dyn_dns_init(p_dyndns);
		if (rc != RC_OK)
		{
			break;
		}		

		rc = get_encoded_user_passwd(p_dyndns);
		if (rc != RC_OK)
		{
			break;
		}

		if (!os_handler_installed)
		{
			rc = os_install_signal_handler(p_dyndns);
			if (rc != RC_OK)
			{
				DBG_PRINTF((LOG_WARNING,"DYNDNS: Error '%s' (0x%x) installing OS signal handler\n",
					errorcode_get_name(rc), rc));
				break;
			}
			os_handler_installed = TRUE;
		}
		
		/*update IP address in a loop*/
		while(1)
		{
			rc = dyn_dns_update_ip(p_dyndns);
			if (rc != RC_OK)
			{
				DBG_PRINTF((LOG_WARNING,"W:'%s' (0x%x) updating the IPs. (it %d)\n",
					errorcode_get_name(rc), rc, iterations)); 
                if (rc == RC_DYNDNS_RSP_NOTOK)
                { 
                    DBG_PRINTF((LOG_ERR,"E: The response of DYNDNS svr was an error! Aborting.\n"));
                    break;              			
                }
			}
			else /*count only the successful iterations */
			{
			   ++iterations;
			}
			
			/* check if the user wants us to stop */			
			if (iterations >= p_dyndns->total_iterations &&
				p_dyndns->total_iterations != 0)
			{
				break;
			}

			/* also sleep the time set in the ->sleep_sec data memeber*/
			dyn_dns_wait_for_cmd(p_dyndns);
			if (p_dyndns->cmd == CMD_STOP)
			{
				DBG_PRINTF((LOG_DEBUG,"STOP command received. Exiting.\n"));
				rc = RC_OK;
				break;
			}
			
			if (rc == RC_OK)
			{
				if (p_dyndns->dbg.level > 0)
				{
					DBG_PRINTF((LOG_DEBUG,"."));
				}
				p_dyndns->times_since_last_update ++;
			}									
		}	 	
	}
	while(FALSE);	
	
	return rc;
}
