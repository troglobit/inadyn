/* DDNS client updater main implementation file
 *
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

#include <resolv.h>
#include <stdlib.h>
#include <string.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include "dyndns.h"
#include "debug_if.h"
#include "base64.h"
#include "md5.h"
#include "sha1.h"
#include "get_cmd.h"

#define MD5_DIGEST_BYTES  16
#define SHA1_DIGEST_BYTES 20

/* Used to preserve values during reset at SIGHUP.  Time also initialized from cache file at startup. */
static int cached_time_since_last_update = 0;
static int cached_num_iterations = 0;

static int get_req_for_dyndns_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_freedns_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_generic_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_zoneedit_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_easydns_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_tzo_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_sitelutions_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_dnsexit_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_he_ipv6tb_server(ddns_t *ctx, int infcnt, int alcnt);
static int get_req_for_changeip_server(ddns_t *ctx, int infcnt, int alcnt);

static int is_dyndns_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_freedns_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_generic_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_zoneedit_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_easydns_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_tzo_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_sitelutions_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_dnsexit_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);
static int is_he_ipv6_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infcnt);

ddns_sysinfo_t dns_system_table[] = {
	{DYNDNS_DEFAULT,
	 {"default@dyndns.org",
	  (ddns_response_ok_func_t) is_dyndns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_dyndns_server,
	  DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
	  "members.dyndns.org", "/nic/update"}},

	{FREEDNS_AFRAID_ORG_DEFAULT,
	 {"default@freedns.afraid.org",
	  (ddns_response_ok_func_t) is_freedns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_freedns_server,
	  DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
	  "freedns.afraid.org", "/dynamic/update.php"}},

	{ZONE_EDIT_DEFAULT,
	 {"default@zoneedit.com",
	  (ddns_response_ok_func_t) is_zoneedit_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_zoneedit_server,
	  "dynamic.zoneedit.com", "/checkip.html",
	  "dynamic.zoneedit.com", "/auth/dynamic.html"}},

	{NOIP_DEFAULT,
	 {"default@no-ip.com",
	  (ddns_response_ok_func_t) is_dyndns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_dyndns_server,
	  "ip1.dynupdate.no-ip.com", "/",
	  "dynupdate.no-ip.com", "/nic/update"}},

	{EASYDNS_DEFAULT,
	 {"default@easydns.com",
	  (ddns_response_ok_func_t) is_easydns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_easydns_server,
	  DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
	  "members.easydns.com", "/dyn/dyndns.php"}},

	{TZO_DEFAULT,
	 {"default@tzo.com",
	  (ddns_response_ok_func_t) is_tzo_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_tzo_server,
	  "echo.tzo.com", "/",
	  "rh.tzo.com", "/webclient/tzoperl.html"}},

	{DYNDNS_3322_DYNAMIC,
	 {"dyndns@3322.org",
	  (ddns_response_ok_func_t) is_dyndns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_dyndns_server,
	  "bliao.com", "/ip.phtml",
	  "members.3322.org", "/dyndns/update"}},

	{SITELUTIONS_DOMAIN,
	 {"default@sitelutions.com",
	  (ddns_response_ok_func_t) is_sitelutions_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_sitelutions_server,
	  DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
	  "www.sitelutions.com", "/dnsup"}},

	{DNSOMATIC_DEFAULT,
	 {"default@dnsomatic.com",
	  (ddns_response_ok_func_t) is_dyndns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_dyndns_server,
	  "myip.dnsomatic.com", "/",
	  "updates.dnsomatic.com", "/nic/update"}},

	{DNSEXIT_DEFAULT,
	 {"default@dnsexit.com",
	  (ddns_response_ok_func_t) is_dnsexit_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_dnsexit_server,
	  "ip.dnsexit.com", "/",
	  "update.dnsexit.com", "/RemoteUpdate.sv"}},

	{HE_IPV6TB,
	 {"ipv6tb@he.net",
	  (ddns_response_ok_func_t) is_he_ipv6_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_he_ipv6tb_server,
	  "checkip.dns.he.net", "/",
	  "ipv4.tunnelbroker.net", "/ipv4_end.php"}},

	{HE_DYNDNS,
	 {"dyndns@he.net",
	  (ddns_response_ok_func_t) is_dyndns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_dyndns_server,
	  "checkip.dns.he.net", "/",
	  "dyn.dns.he.net", "/nic/update"}},

	{CHANGEIP_DEFAULT,
	 {"default@changeip.com",
	  (ddns_response_ok_func_t) is_dyndns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_changeip_server,
	  "ip.changeip.com", "/",
	  "nic.changeip.com", "/nic/update"}},

	/* Support for dynsip.org by milkfish, from DD-WRT */
	{DYNSIP_DEFAULT,
	 {"default@dynsip.org",
	  (ddns_response_ok_func_t) is_dyndns_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_dyndns_server,
	  DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
	  "dynsip.org", "/nic/update"}},

	{CUSTOM_HTTP_BASIC_AUTH,
	 {"custom@http_srv_basic_auth",
	  (ddns_response_ok_func_t) is_generic_server_rsp_ok,
	  (ddns_request_func_t) get_req_for_generic_server,
	  DYNDNS_MY_IP_SERVER, DYNDNS_MY_IP_SERVER_URL,
	  "", ""}},

	{LAST_DNS_SYSTEM, {NULL, NULL, NULL, NULL, NULL, NULL, NULL}}
};

ddns_sysinfo_t *ddns_system_table(void)
{
	return dns_system_table;
}

/*************PRIVATE FUNCTIONS ******************/
static int wait_for_cmd(ddns_t *ctx)
{
	int counter;
	ddns_cmd_t old_cmd;

	if (!ctx)
		return RC_INVALID_POINTER;

	old_cmd = ctx->cmd;
	if (old_cmd != NO_CMD)
		return 0;

	counter = ctx->sleep_sec / ctx->cmd_check_period;
	while (counter--) {
		if (ctx->cmd != old_cmd)
			break;

		os_sleep_ms(ctx->cmd_check_period * 1000);
	}

	return 0;
}

static int is_http_status_code_ok(int status)
{
	if (status == 200)
		return 0;
	else if (status >= 500 && status < 600)
		return RC_DYNDNS_RSP_RETRY_LATER;

	return RC_DYNDNS_RSP_NOTOK;
}

static int get_req_for_dyndns_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, DYNDNS_UPDATE_IP_HTTP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].my_ip_address.name,
		       ctx->info[infcnt].dyndns_server_name.name, ctx->info[infcnt].creds.encoded_password);
}

static int get_req_for_freedns_server(ddns_t *ctx, int infcnt, int alcnt)
{
	int rc = 0, rc2;
	http_client_t client;
	http_trans_t trans;

	char buffer[256];

	unsigned char digestbuf[SHA1_DIGEST_BYTES];
	char digeststr[SHA1_DIGEST_BYTES * 2 + 1];
	int i;

	char *buf, *tmp, *line;
	char host[256], updateurl[256];
	char *hash = NULL;

	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}
	// I know it's ugly, http client needs redesign.

	do {
		if ((rc = http_client_construct(&client)) != 0)
			break;

		http_client_set_port(&client, ctx->info[infcnt].dyndns_server_name.port);
		http_client_set_remote_name(&client, ctx->info[infcnt].dyndns_server_name.name);
		http_client_set_bind_iface(&client, ctx->bind_interface);

		if ((rc = http_client_init(&client, "Sending update URL query")) != 0)
			break;

		snprintf(buffer, sizeof(buffer), "%s|%s",
			 ctx->info[infcnt].creds.username, ctx->info[infcnt].creds.password);
		sha1((unsigned char *)buffer, strlen(buffer), digestbuf);
		for (i = 0; i < SHA1_DIGEST_BYTES; i++)
			sprintf(&digeststr[i * 2], "%02x", digestbuf[i]);

		snprintf(buffer, sizeof(buffer), "/api/?action=getdyndns&sha=%s", digeststr);

		trans.req_len = sprintf(ctx->request_buf, GENERIC_HTTP_REQUEST,
					buffer, ctx->info[infcnt].dyndns_server_name.name);
		trans.p_req = (char *)ctx->request_buf;
		trans.p_rsp = (char *)ctx->work_buf;
		trans.max_rsp_len = ctx->work_buflen - 1;	/* Save place for a \0 at the end */
		trans.rsp_len = 0;

		rc = http_client_transaction(&client, &trans);
		trans.p_rsp[trans.rsp_len] = 0;

		rc2 = http_client_shutdown(&client);

		http_client_destruct(&client, 1);

		if (rc != 0 || rc2 != 0)
			break;

		if ((rc = is_http_status_code_ok(trans.status)) != 0)
			break;

		tmp = buf = strdup(trans.p_rsp_body);

		for (line = strsep(&tmp, "\n"); line; line = strsep(&tmp, "\n")) {
			if (*line &&
			    sscanf(line, "%255[^|\r\n]|%*[^|\r\n]|%255[^|\r\n]",
				   host, updateurl) == 2
			    && !strcmp(host, ctx->info[infcnt].alias[alcnt].names.name)) {
				hash = strstr(updateurl, "?");
				break;
			}
		}

		free(buf);

		if (!hash)
			rc = RC_DYNDNS_RSP_NOTOK;
	}
	while (0);

	if (rc != 0) {
		logit(LOG_INFO, "Update URL query failed");
		return 0;
	}

	return sprintf(ctx->request_buf, FREEDNS_UPDATE_IP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       hash, ctx->info[infcnt].my_ip_address.name, ctx->info[infcnt].dyndns_server_name.name);
}

static int get_req_for_generic_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, GENERIC_BASIC_AUTH_UPDATE_IP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].dyndns_server_name.name, ctx->info[infcnt].creds.encoded_password);
}

static int get_req_for_zoneedit_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, ZONEEDIT_UPDATE_IP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].my_ip_address.name,
		       ctx->info[infcnt].dyndns_server_name.name, ctx->info[infcnt].creds.encoded_password);
}

static int get_req_for_easydns_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, EASYDNS_UPDATE_IP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].my_ip_address.name,
		       ctx->info[infcnt].wildcard ? "ON" : "OFF",
		       ctx->info[infcnt].dyndns_server_name.name, ctx->info[infcnt].creds.encoded_password);
}

static int get_req_for_tzo_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, TZO_UPDATE_IP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].creds.username,
		       ctx->info[infcnt].creds.password,
		       ctx->info[infcnt].my_ip_address.name, ctx->info[infcnt].dyndns_server_name.name);
}

static int get_req_for_sitelutions_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, SITELUTIONS_UPDATE_IP_HTTP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].creds.username,
		       ctx->info[infcnt].creds.password,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].my_ip_address.name, ctx->info[infcnt].dyndns_server_name.name);
}

static int get_req_for_dnsexit_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, DNSEXIT_UPDATE_IP_HTTP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].creds.username,
		       ctx->info[infcnt].creds.password,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].my_ip_address.name, ctx->info[infcnt].dyndns_server_name.name);
}

static int get_req_for_he_ipv6tb_server(ddns_t *ctx, int infcnt, int alcnt)
{
	unsigned char digestbuf[MD5_DIGEST_BYTES];
	char digeststr[MD5_DIGEST_BYTES * 2 + 1];
	int i;

	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	md5((unsigned char *)ctx->info[infcnt].creds.password,
	    strlen(ctx->info[infcnt].creds.password), digestbuf);
	for (i = 0; i < MD5_DIGEST_BYTES; i++)
		sprintf(&digeststr[i * 2], "%02x", digestbuf[i]);
	return sprintf(ctx->request_buf, HE_IPV6TB_UPDATE_IP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].my_ip_address.name,
		       ctx->info[infcnt].creds.username,
		       digeststr,
		       ctx->info[infcnt].alias[alcnt].names.name, ctx->info[infcnt].dyndns_server_name.name);
}

static int get_req_for_changeip_server(ddns_t *ctx, int infcnt, int alcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, CHANGEIP_UPDATE_IP_HTTP_REQUEST,
		       ctx->info[infcnt].dyndns_server_url,
		       ctx->info[infcnt].alias[alcnt].names.name,
		       ctx->info[infcnt].my_ip_address.name,
		       ctx->info[infcnt].dyndns_server_name.name, ctx->info[infcnt].creds.encoded_password);
}

/*
	Get the IP address from interface
*/
static int check_interface_address(ddns_t *ctx)
{
	struct ifreq ifr;
	in_addr_t new_ip;
	char *new_ip_str;
	int i, sd, anychange = 0;

	logit(LOG_INFO, "Checking for IP# change, querying interface %s", ctx->check_interface);

	sd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		int code = os_get_socket_error();

		logit(LOG_WARNING, "Failed opening network socket: %s", strerror(code));
		return RC_IP_OS_SOCKET_INIT_FAILED;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	ifr.ifr_addr.sa_family = AF_INET;
	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ctx->check_interface);
	if (ioctl(sd, SIOCGIFADDR, &ifr) != -1) {
		new_ip = ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
		new_ip_str = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	} else {
		int code = os_get_socket_error();

		logit(LOG_ERR, "Failed reading IP address of interface %s: %s",
		      ctx->check_interface, strerror(code));
		return RC_ERROR;
	}
	close(sd);

	if (IN_ZERONET(new_ip) ||
	    IN_LOOPBACK(new_ip) || IN_LINKLOCAL(new_ip) || IN_MULTICAST(new_ip) || IN_EXPERIMENTAL(new_ip)) {
		logit(LOG_WARNING, "Interface %s has invalid IP# %s", ctx->check_interface, new_ip_str);
		return RC_ERROR;
	}

	for (i = 0; i < ctx->info_count; i++) {
		ddns_info_t *info = &ctx->info[i];

		info->ip_has_changed = strcmp(info->my_ip_address.name, new_ip_str) != 0;
		if (info->ip_has_changed) {
			anychange++;
			strcpy(info->my_ip_address.name, new_ip_str);
		}
	}

	if (!anychange)
		logit(LOG_INFO, "No IP# change detected, still at %s", new_ip_str);

	return 0;
}

static int get_req_for_ip_server(ddns_t *ctx, int infcnt)
{
	if (ctx == NULL) {
		/* 0 == "No characters written" */
		return 0;
	}

	return sprintf(ctx->request_buf, DYNDNS_GET_IP_HTTP_REQUEST,
		       ctx->info[infcnt].ip_server_url, ctx->info[infcnt].ip_server_name.name);
}

/*
	Send req to IP server and get the response
*/
static int server_transaction(ddns_t *ctx, int servernum)
{
	int rc = 0;
	http_client_t *p_http;
	http_trans_t *p_tr;

	if (!ctx)
		return RC_INVALID_POINTER;

	p_http = &ctx->http_to_ip_server[servernum];

	DO(http_client_init(p_http, "Checking for IP# change"));

	/* Prepare request for IP server */
	p_tr = &ctx->http_transaction;
	p_tr->req_len = get_req_for_ip_server(ctx, servernum);
	if (ctx->dbg.level > 2) {
		logit(LOG_DEBUG, "Querying DDNS server for my public IP#:");
		logit(LOG_DEBUG, "%s", ctx->request_buf);
	}
	p_tr->p_req = (char *)ctx->request_buf;
	p_tr->p_rsp = (char *)ctx->work_buf;
	p_tr->max_rsp_len = ctx->work_buflen - 1;	/* Save place for terminating \0 in string. */
	p_tr->rsp_len = 0;

	rc = http_client_transaction(p_http, &ctx->http_transaction);
	p_tr->p_rsp[p_tr->rsp_len] = 0;

	if (p_tr->status != 200)
		rc = RC_DYNDNS_INVALID_RSP_FROM_IP_SERVER;

	http_client_shutdown(p_http);

	return rc;
}

/*
  Read in 4 integers the ip addr numbers.
  construct then the IP address from those numbers.
  Note:
  it updates the flag: info->'ip_has_changed' if the old address was different
*/
static int parse_my_ip_address(ddns_t *ctx, int servernum)
{
	int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
	int count, i;
	char *p_ip, *p_current_str;
	int found;
	char new_ip_str[IP_V4_MAX_LEN];

	(void)servernum;

	if (!ctx || ctx->http_transaction.rsp_len <= 0 || !ctx->http_transaction.p_rsp)
		return RC_INVALID_POINTER;

	p_current_str = ctx->http_transaction.p_rsp_body;

	found = 0;
	do {
		/* Try to find first decimal number (begin of IP) */
		p_ip = strpbrk(p_current_str, "0123456789");
		if (p_ip != NULL) {
			/* Maybe I found it? */
			count = sscanf(p_ip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
			if (count != 4 || ip1 <= 0 || ip1 > 255 || ip2 < 0
			    || ip2 > 255 || ip3 < 0 || ip3 > 255 || ip4 < 0 || ip4 > 255) {
				p_current_str = p_ip + 1;
			} else {
				/* FIRST occurence of a valid IP found */
				found = 1;
				break;
			}
		}
	}
	while (p_ip != NULL);

	if (found) {
		int anychange = 0;

		for (i = 0; i < ctx->info_count; i++) {
			ddns_info_t *info = &ctx->info[i];

			sprintf(new_ip_str, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

			info->ip_has_changed = strcmp(info->my_ip_address.name, new_ip_str) != 0;
			if (info->ip_has_changed) {
				anychange++;
				strcpy(info->my_ip_address.name, new_ip_str);
			}
		}

		if (!anychange) {
			logit(LOG_INFO, "No IP# change detected, still at %s", new_ip_str);
		}

		return 0;
	}

	return RC_DYNDNS_INVALID_RSP_FROM_IP_SERVER;
}

/*
  Updates for every maintained name the property: 'update_required'.
  The property will be checked in another function and updates performed.

  Action:
  Check if my IP address has changed. -> ALL names have to be updated.
  Nothing else.
  Note: In the update function the property will set to false if update was successful.
*/
static int check_alias_update_table(ddns_t *ctx)
{
	int i, j;

	/* Uses fix test if ip of server 0 has changed.
	 * That should be OK even if changes check_address() to
	 * iterate over servernum, but not if it's fix set to =! 0 */
	if (ctx->info[0].ip_has_changed || ctx->force_addr_update ||
	    (ctx->time_since_last_update > ctx->forced_update_period_sec)) {
		for (i = 0; i < ctx->info_count; i++) {
			ddns_info_t *info = &ctx->info[i];

			for (j = 0; j < info->alias_count; j++) {
				info->alias[j].update_required = 1;
				logit(LOG_WARNING, "Update needed for alias %s, new IP# %s",
				      info->alias[j].names.name, info->my_ip_address.name);
			}
		}
	}

	return 0;
}

/* DynDNS org.specific response validator.
   'good' or 'nochg' are the good answers,
*/
static int is_dyndns_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	(void)ctx;
	char *p_rsp = p_tr->p_rsp_body;
	(void)infnr;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	if (strstr(p_rsp, "good") != NULL || strstr(p_rsp, "nochg") != NULL)
		return 0;
	else if (strstr(p_rsp, "dnserr") != NULL || strstr(p_rsp, "911") != NULL)
		return RC_DYNDNS_RSP_RETRY_LATER;
	else
		return RC_DYNDNS_RSP_NOTOK;
}

/* Freedns afraid.org.specific response validator.
   ok blabla and n.n.n.n
    fail blabla and n.n.n.n
    are the good answers. We search our own IP address in response and that's enough.
*/
static int is_freedns_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	char *p_rsp = p_tr->p_rsp_body;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	if (strstr(p_rsp, ctx->info[infnr].my_ip_address.name) != NULL)
		return 0;
	else
		return RC_DYNDNS_RSP_NOTOK;
}

/** generic http dns server ok parser
    parses a given string. If found is ok,
    Example : 'SUCCESS CODE='
*/
static int is_generic_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	(void)ctx;
	char *p_rsp = p_tr->p_rsp_body;
	(void)infnr;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	if (strstr(p_rsp, "OK") != NULL)
		return 0;
	else
		return RC_DYNDNS_RSP_NOTOK;
}

/**
   the OK codes are:
   CODE=200, 201
   CODE=707, for duplicated updates
*/
static int is_zoneedit_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	(void)ctx;
	(void)infnr;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	int code = -1;
	sscanf(p_tr->p_rsp_body, "%*s CODE=\"%d\" ", &code);

	switch (code) {
	case 200:
	case 201:
	case 707:
		// is 707 really ok??
		return 0;
	default:
		return RC_DYNDNS_RSP_NOTOK;
	}
}

/**
   NOERROR is the OK code here
*/
static int is_easydns_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	(void)ctx;
	char *p_rsp = p_tr->p_rsp_body;
	(void)infnr;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	if (strstr(p_rsp, "NOERROR") != NULL)
		return 0;
	else if (strstr(p_rsp, "TOOSOON") != NULL)
		return RC_DYNDNS_RSP_RETRY_LATER;
	else
		return RC_DYNDNS_RSP_NOTOK;
}

/* TZO specific response validator. */
static int is_tzo_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	(void)ctx;
	(void)infnr;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	int code = -1;
	sscanf(p_tr->p_rsp_body, "%d ", &code);

	switch (code) {
	case 200:
	case 304:
		return 0;
	case 414:
	case 500:
		return RC_DYNDNS_RSP_RETRY_LATER;
	default:
		return RC_DYNDNS_RSP_NOTOK;
	}
}

static int is_sitelutions_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	(void)ctx;
	char *p_rsp = p_tr->p_rsp_body;
	(void)infnr;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	if (strstr(p_rsp, "success") != NULL)
		return 0;
	else if (strstr(p_rsp, "dberror") != NULL)
		return RC_DYNDNS_RSP_RETRY_LATER;
	else
		return RC_DYNDNS_RSP_NOTOK;
}

static int is_dnsexit_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	(void)ctx;
	(void)infnr;

	int rc;
	if ((rc = is_http_status_code_ok(p_tr->status)) != 0)
		return rc;

	int code = -1;
	char *tmp;
	if ((tmp = strstr(p_tr->p_rsp_body, "\n")) != NULL)
		sscanf(++tmp, "%d=", &code);

	switch (code) {
	case 0:
	case 1:
		return 0;
	case 4:
	case 11:
		return RC_DYNDNS_RSP_RETRY_LATER;
	default:
		return RC_DYNDNS_RSP_NOTOK;
	}
}

/* HE ipv6 tunnelbroker specific response validator.
   own IP address and 'already in use' are the good answers.
*/
static int is_he_ipv6_server_rsp_ok(ddns_t *ctx, http_trans_t *p_tr, int infnr)
{
	char *p_rsp = p_tr->p_rsp_body;

	DO(is_http_status_code_ok(p_tr->status));

	if (strstr(p_rsp, ctx->info[infnr].my_ip_address.name) != NULL ||
	    strstr(p_rsp, "-ERROR: This tunnel is already associated with this IP address.")
	    != NULL)
		return 0;

	return RC_DYNDNS_RSP_NOTOK;
}


/* DNS Lookup */
static int nslookup(ddns_info_t *entry)
{
	int error;
	char name[DYNDNS_SERVER_NAME_LEN];
	struct addrinfo hints;
	struct addrinfo *result;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;	/* IPv4 */
	hints.ai_socktype = SOCK_DGRAM;	/* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;	        /* Any protocol */

	error = getaddrinfo(entry->alias[0].names.name, NULL, &hints, &result);
	if (!error) {
		/* DNS reply for alias found, convert to IP# */
		if (!getnameinfo(result->ai_addr, result->ai_addrlen, name, sizeof(name), NULL, 0, NI_NUMERICHOST)) {
			/* Update local record for next checkip call. */
			strncpy(entry->my_ip_address.name, name, sizeof(entry->my_ip_address.name));
			logit(LOG_INFO, "Resolving hostname %s => IP# %s", entry->alias[0].names.name, name);
		}

		freeaddrinfo(result);
		return 0;
	}

	logit(LOG_WARNING, "Failed resolving hostname %s: %s", entry->alias[0].names.name, gai_strerror(error));

	return 1;
}

/* At boot, or when restarting inadyn at runtime, the memory struct holding our
 * current IP# is empty.  We want to avoid unnecessary updates of our DDNS server
 * record, since we might get locked out for abuse, so we "seed" each of the DDNS
 * records of our struct with the cached IP# from our cache file, or from a regular
 * DNS query. */
static int read_cache_file (ddns_t *ctx)
{
	int i;
	FILE *fp;
	char name[DYNDNS_SERVER_NAME_LEN];

	if (!ctx)
		return RC_INVALID_POINTER;

	fp = fopen(ctx->cache_file, "r");
	if (!fp) {
		/* Clear DNS cache before querying for the IP below. */
		res_init();

		/* Try a DNS lookup of our last known IP#. */
		for (i = 0; i < ctx->info_count; i++) {
			/* exception for tunnelbroker.net - no name to lookup */
			if (ctx->info[i].alias_count && strcmp(ctx->info[i].system->key, "ipv6tb@he.net"))
				nslookup(&ctx->info[i]);
		}
	} else {
		struct stat statbuf;

		/* Read cached IP# from inadyn cache file. */
		if (fgets(name, sizeof(name), fp)) {
			logit(LOG_INFO, "Cached IP# %s from previous invocation.", name);

			/* Update local record for next checkip call. */
			for (i = 0; i < ctx->info_count; i++) {
				strncpy(ctx->info[i].my_ip_address.name, name,
					sizeof(ctx->info[i].my_ip_address.name));
			}
		}

		/* Initialize time since last update from modification time of cache file. */
		if (fstat(fileno(fp), &statbuf) == 0) {
			time_t now = time(NULL);

			if (now != -1 && now > statbuf.st_mtime) {
				cached_time_since_last_update = (int)(now - statbuf.st_mtime);
				logit(LOG_INFO, "Cached time since last update %d (seconds) from previous invocation.",
				      cached_time_since_last_update);
			}
		}

		fclose(fp);
	}

	return 0;
}

/* Update cache with new IP */
static int write_cache_file(ddns_t *ctx)
{
	FILE *fp;

	fp = fopen(ctx->cache_file, "w");
	if (fp) {
		fprintf(fp, "%s", ctx->info[0].my_ip_address.name);
		fclose(fp);

		return 0;
	}

	return 1;
}

static int write_pidfile(ddns_t *ctx)
{
	FILE *fp = fopen(ctx->pidfile, "w");

	if (!fp) {
		logit(LOG_ERR, "Failed creating pidfile %s: %s", ctx->pidfile, strerror(errno));
		return 1;
	}

	fprintf(fp, "%u\n", getpid());
	fclose(fp);

	return 0;
}

static int update_alias_table(ddns_t *ctx)
{
	int i, j;
	int rc = 0, rc2;
	http_trans_t http_tr;
	int anychange = 0;

	for (i = 0; i < ctx->info_count; i++) {
		ddns_info_t *info = &ctx->info[i];

		for (j = 0; j < info->alias_count; j++) {
			if (info->alias[j].update_required != 1)
				continue;

			rc = http_client_init(&ctx->http_to_dyndns[i], "Sending IP# update to DDNS server");
			if (rc != 0) {
				break;
			}

			/* Build dyndns transaction */
			http_tr.req_len = info->system->update_request_func(ctx, i, j);
			http_tr.p_req = (char *)ctx->request_buf;
			http_tr.p_rsp = (char *)ctx->work_buf;
			http_tr.max_rsp_len = ctx->work_buflen - 1;	/* Save place for a \0 at the end */
			http_tr.rsp_len = 0;

			rc = http_client_transaction(&ctx->http_to_dyndns[i], &http_tr);
			http_tr.p_rsp[http_tr.rsp_len] = 0;

			if (ctx->dbg.level > 2) {
				ctx->request_buf[http_tr.req_len] = 0;
				logit(LOG_DEBUG, "Sending alias table update to DDNS server:");
				logit(LOG_DEBUG, "%s", ctx->request_buf);
			}

			if (rc == 0) {
				rc = info->system->response_ok_func(ctx, &http_tr, i);
				if (rc == 0) {
					info->alias[j].update_required = 0;

					logit(LOG_INFO,
					      "Successful alias table update for %s => new IP# %s",
					      info->alias[j].names.name, info->my_ip_address.name);
					ctx->time_since_last_update = 0;
					ctx->force_addr_update = 0;
					anychange++;
				} else {
					logit(LOG_WARNING,
					      "%s error in DDNS server response:",
					      rc == RC_DYNDNS_RSP_RETRY_LATER ? "Temporary" : "Fatal");
					logit(LOG_WARNING, "[%d %s] %s",
					      http_tr.status,
					      http_tr.status_desc,
					      http_tr.p_rsp_body != http_tr.p_rsp ? http_tr.p_rsp_body : "");
				}

				if (ctx->dbg.level > 2) {
					logit(LOG_DEBUG, "DDNS server response:");
					logit(LOG_DEBUG, "%s", http_tr.p_rsp);
				}
			}

			rc2 = http_client_shutdown(&ctx->http_to_dyndns[i]);
			if (rc == 0) {
				/* Only overwrite rc with of http_client_shutdown() rc if previous call, in
				 * e.g., http_client_transaction() or the response_ok_func() callback was OK. */
				rc = rc2;
			}
			if (rc != 0) {
				break;
			}
			os_sleep_ms(1000);
		}
	}

	/* Successful change or when cache file does not yet exist! */
	if (anychange || access(ctx->cache_file, F_OK)) {
		write_cache_file(ctx);

		/* Run external command hook on update. */
		if (anychange && ctx->external_command)
			os_shell_execute(ctx->external_command,
					 ctx->info[0].my_ip_address.name,
					 ctx->info[0].alias[0].names.name, ctx->bind_interface);
	}

	return rc;
}

static int get_encoded_user_passwd(ddns_t *ctx)
{
	int rc = 0;
	const char *format = "%s:%s";
	char *buf = NULL;
	int size, actual_len;
	int i = 0;
	char *encode = NULL;
	int rc2;

	do {
		size_t dlen = 0;
		ddns_info_t *info = &ctx->info[i];

		size = strlen(info->creds.password) + strlen(info->creds.username) + strlen(format) + 1;

		buf = (char *)malloc(size);
		if (!buf) {
			info->creds.encoded = 0;
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		actual_len = sprintf(buf, format, info->creds.username, info->creds.password);
		if (actual_len >= size) {
			info->creds.encoded = 0;
			rc = RC_OUT_BUFFER_OVERFLOW;
			break;
		}

		/* query required buffer size for base64 encoded data */
		base64_encode(NULL, &dlen, (unsigned char *)buf, strlen(buf));
		encode = (char *)malloc(dlen);
		if (encode == NULL) {
			info->creds.encoded = 0;
			rc = RC_OUT_OF_MEMORY;
			break;
		}

		/* encode */
		rc2 = base64_encode((unsigned char *)encode, &dlen, (unsigned char *)buf, strlen(buf));
		if (rc2 != 0) {
			info->creds.encoded = 0;
			rc = RC_OUT_BUFFER_OVERFLOW;
			break;
		}

		info->creds.encoded_password = encode;
		info->creds.encoded = 1;
		info->creds.size = strlen(info->creds.encoded_password);

		free(buf);
		buf = NULL;
	}
	while (++i < ctx->info_count);

	if (buf)
		free(buf);

	return rc;
}

/**
   Sets up the object.
   - sets the IPs of the DYN DNS server
   - if proxy server is set use it!
   - ...
*/
static int init_context(ddns_t *ctx)
{
	int i;

	if (!ctx)
		return RC_INVALID_POINTER;

	if (ctx->initialized == 1)
		return 0;

	for (i = 0; i < ctx->info_count; i++) {
		ddns_info_t *info = &ctx->info[i];

		if (strlen(info->proxy_server_name.name)) {
			http_client_set_port(&ctx->http_to_ip_server[i], info->proxy_server_name.port);
			http_client_set_remote_name(&ctx->http_to_ip_server[i], info->proxy_server_name.name);

			http_client_set_port(&ctx->http_to_dyndns[i], info->proxy_server_name.port);
			http_client_set_remote_name(&ctx->http_to_dyndns[i], info->proxy_server_name.name);
		} else {
			http_client_set_port(&ctx->http_to_ip_server[i], info->ip_server_name.port);
			http_client_set_remote_name(&ctx->http_to_ip_server[i], info->ip_server_name.name);

			http_client_set_port(&ctx->http_to_dyndns[i], info->dyndns_server_name.port);
			http_client_set_remote_name(&ctx->http_to_dyndns[i], info->dyndns_server_name.name);
		}

		http_client_set_bind_iface(&ctx->http_to_dyndns[i], ctx->bind_interface);
		http_client_set_bind_iface(&ctx->http_to_ip_server[i], ctx->bind_interface);
	}

	/* Restore values, if reset by SIGHUP.  Initialize time from cache file at startup. */
	ctx->time_since_last_update = cached_time_since_last_update;
	ctx->num_iterations = cached_num_iterations;

	ctx->initialized = 1;

	return 0;
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
static int check_address(ddns_t *ctx)
{
	int servernum = 0;  /* server to use for checking IP, index 0 by default */

	if (!ctx)
		return RC_INVALID_POINTER;

	if (ctx->check_interface) {
		DO(check_interface_address(ctx));
	} else {
		/* Ask IP server something so he will respond and give me my IP */
		DO(server_transaction(ctx, servernum));
		if (ctx->dbg.level > 1) {
			logit(LOG_DEBUG, "IP server response:");
			logit(LOG_DEBUG, "%s", ctx->work_buf);
		}

		/* Extract our IP, check if different than previous one */
		DO(parse_my_ip_address(ctx, servernum));
		if (ctx->dbg.level > 1)
			logit(LOG_INFO, "Current public IP# %s", ctx->info[servernum].my_ip_address.name);
	}

	/* Step through aliases list, resolve them and check if they point to my IP */
	DO(check_alias_update_table(ctx));

	/* Update IPs marked as not identical with my IP */
	DO(update_alias_table(ctx));

	return 0;
}

/**
 * Main DDNS loop
 * Actions:
 *  - read the configuration options
 *  - perform various init actions as specified in the options
 *  - create and init dyn_dns object.
 *  - launch the IP update action loop
 */
int ddns_main_loop(ddns_t *ctx, int argc, char *argv[])
{
	int rc = 0;
	static int first_startup = 1;

	if (!ctx)
		return RC_INVALID_POINTER;

	/* Create pid and cache file repository. */
	mkdir(DYNDNS_RUNTIME_DATA_DIR, 0755);

	/* read cmd line options and set object properties */
	rc = get_config_data(ctx, argc, argv);
	if (rc != 0 || ctx->abort)
		return rc;

	if (ctx->change_persona) {
		ddns_user_t user;

		memset(&user, 0, sizeof(user));
		user.gid = ctx->sys_usr_info.gid;
		user.uid = ctx->sys_usr_info.uid;

		DO(os_change_persona(&user));
	}

	/* if logfile provided, redirect output to log file */
	if (strlen(ctx->dbg.p_logfilename) != 0)
		DO(os_open_dbg_output(DBG_FILE_LOG, "", ctx->dbg.p_logfilename));

	if (ctx->debug_to_syslog == 1 || (ctx->run_in_background == 1)) {
		if (get_dbg_dest() == DBG_STD_LOG)	/* avoid file and syslog output */
			DO(os_open_dbg_output(DBG_SYS_LOG, "inadyn", NULL));
	}

	/* if silent required, close console window */
	if (ctx->run_in_background == 1) {
		DO(close_console_window());

		if (get_dbg_dest() == DBG_SYS_LOG)
			fclose(stdout);
	}

	/* Create files with permissions 0644 */
	umask(S_IWGRP | S_IWOTH);
	write_pidfile(ctx);

	/* "Hello!" Let user know we've started up OK */
	logit(LOG_INFO, "%s", DYNDNS_VERSION_STRING);

	/* On first startup only, optionally wait for network and any NTP daemon
	 * to set system time correctly.  Intended for devices without battery
	 * backed real time clocks as initialization of time since last update
	 * requires the correct time.  Sleep can be interrupted with the usual
	 * signals inadyn responds too. */
	if (first_startup && ctx->startup_delay_sec) {
		logit(LOG_NOTICE, "Startup delay: %d sec ...", ctx->startup_delay_sec);
		first_startup = 0;

		/* Now sleep a while. Using the time set in sleep_sec data member */
		ctx->sleep_sec = ctx->startup_delay_sec;
		wait_for_cmd(ctx);

		if (ctx->cmd == CMD_STOP) {
			logit(LOG_INFO, "STOP command received, exiting.");
			return 0;
		}
		if (ctx->cmd == CMD_RESTART) {
			logit(LOG_INFO, "RESTART command received, restarting.");
			return RC_RESTART;
		}
		if (ctx->cmd == CMD_FORCED_UPDATE) {
			logit(LOG_INFO, "FORCED_UPDATE command received, updating now.");
			ctx->force_addr_update = 1;
			ctx->cmd = NO_CMD;
		} else if (ctx->cmd == CMD_CHECK_NOW) {
			logit(LOG_INFO, "CHECK_NOW command received, leaving startup delay.");
			ctx->cmd = NO_CMD;
		}
	}

	DO(init_context(ctx));
	DO(read_cache_file(ctx));
	DO(get_encoded_user_passwd(ctx));

	if (ctx->update_once == 1)
		ctx->force_addr_update = 1;

	/* DDNS client main loop */
	while (1) {
		rc = check_address(ctx);
		if (rc) {
			if (ctx->cmd == CMD_RESTART) {
				logit(LOG_INFO, "RESTART command received. Restarting.");
				rc = RC_RESTART;
				ctx->cmd = NO_CMD;
				break;
			}

			if (rc == RC_DYNDNS_RSP_NOTOK) {
				logit(LOG_ERR, "Error response from DDNS server, exiting!");
				break;
			}
		} else {
			/* count only the successful iterations */
			ctx->num_iterations++;
		}

		/* check if the user wants us to stop */
		if (ctx->total_iterations != 0 && ctx->num_iterations >= ctx->total_iterations)
			break;

		ctx->sleep_sec = (rc == RC_DYNDNS_RSP_RETRY_LATER) ?
			ctx->error_update_period_sec : ctx->normal_update_period_sec;

		if (rc != 0) {
			/* check_address() failed above, and we've not reached MAX iterations.
			 * Time to inform the user the (network) error is not fatal and that we
			 * will try again in a short while. */
			logit(LOG_WARNING, "Will retry again in %d sec ...", ctx->sleep_sec);
		}

		/* Now sleep a while. Using the time set in sleep_sec data member */
		wait_for_cmd(ctx);

		if (ctx->cmd == CMD_STOP) {
			logit(LOG_INFO, "STOP command received, exiting.");
			rc = 0;
			break;
		}
		if (ctx->cmd == CMD_RESTART) {
			logit(LOG_INFO, "RESTART command received, restarting.");
			rc = RC_RESTART;
			break;
		}
		if (ctx->cmd == CMD_FORCED_UPDATE) {
			logit(LOG_INFO, "FORCED_UPDATE command received, updating now.");
			ctx->force_addr_update = 1;
			ctx->cmd = NO_CMD;
			continue;
		}

		if (ctx->cmd == CMD_CHECK_NOW) {
			logit(LOG_INFO, "CHECK_NOW command received, checking ...");
			ctx->cmd = NO_CMD;
			continue;
		}

		if (ctx->dbg.level > 0) {
			logit(LOG_DEBUG, ".");
//                      logit(LOG_DEBUG, "Time since last update: %d", ctx->time_since_last_update);
		}

		ctx->time_since_last_update += ctx->sleep_sec;
	}

	/* Save old value, if restarted by SIGHUP */
	cached_time_since_last_update = ctx->time_since_last_update;
	cached_num_iterations = ctx->num_iterations;

	return rc;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
