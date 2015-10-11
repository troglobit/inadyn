/* libConfuse interface to parse inadyn.conf v2 format
 *
 * Copyright (C) 2014-2015  Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <string.h>
#include <confuse.h>
#include "ddns.h"

/*
 * syslog        = true
 * period        = 600
 * startup-delay = 30
 * forced-update = 604800
 * bind          = eth0
 *
 * provider default@freedns.afraid.org
 * {
 *   wildcard = false
 *   username = example
 *   password = secret
 *   alias    = { "example.homenet.org", "example.afraid.org" }
 * }
 *
 * provider default@dyndns.org
 * {
 *   ssl      = true
 *   username = admin
 *   password = supersecret
 *   alias    = example.dyndns.org
 * }
 */


static int validate_period(cfg_t *cfg, cfg_opt_t *opt)
{
	int val = cfg_getint(cfg, opt->name);

	if (val < DDNS_MIN_PERIOD)
		val = DDNS_MIN_PERIOD;
	if (val > DDNS_MAX_PERIOD)
		val = DDNS_MAX_PERIOD;

	return 0;
}

static int validate_alias(cfg_t *cfg, const char *provider, cfg_opt_t *alias)
{
	size_t i;

	if (!alias) {
		cfg_error(cfg, "Missing DDNS alias setting in provider %s", provider);
		return -1;
	}

	if (!cfg_opt_size(alias)) {
		cfg_error(cfg, "No aliases listed in DDNS provider %s", provider);
		return -1;
	}

	for (i = 0; i < cfg_opt_size(alias); i++) {
		char *name = cfg_opt_getnstr(alias, i);
		ddns_info_t info;

		if (sizeof(info.alias[0].name) < strlen(name)) {
			cfg_error(cfg, "Too long DDNS alias (%s) in provider %s", name, provider);
			return -1;
		}
	}

	return 0;
}

static int validate_provider(cfg_t *cfg, cfg_opt_t *opt)
{
	char *str;
	const char *provider = cfg_title(cfg);

	fprintf(stderr, "Parsing provider %s ...\n", provider);
	if (!provider) {
		cfg_error(cfg, "Missing DDNS provider name");
		return -1;
	}

	if (!plugin_find(provider)) {
		cfg_error(cfg, "Invalid DDNS provider %s", provider);
		return -1;
	}

	if (!cfg_getstr(cfg, "username")) {
		cfg_error(cfg, "Missing username setting for DDNS provider %s", provider);
		return -1;
	}

	if (!cfg_getstr(cfg, "password")) {
		cfg_error(cfg, "Missing password setting for DDNS provider %s", provider);
		return -1;
	}

	return validate_alias(cfg, provider, cfg_getopt(cfg, "alias"));
}

cfg_t *conf_parse_file(char *file, ddns_t *ctx)
{
	size_t i, num = 0;
	char *str;
	cfg_opt_t popts[] = {   /* Provider options */
		CFG_STR     ("username",  NULL, CFGF_NONE),
		CFG_STR     ("password",  NULL, CFGF_NONE),
		CFG_STR_LIST("alias",     NULL, CFGF_NONE),
		CFG_BOOL    ("ssl",       cfg_false, CFGF_NONE),
		CFG_BOOL    ("wildcard",  cfg_false, CFGF_NONE),
		CFG_END()
	};
	cfg_opt_t opts[] = {	/* Global options */
		CFG_BOOL("fake-address",  cfg_false, CFGF_NONE),
		CFG_STR ("bind",	  NULL, CFGF_NONE),
		CFG_INT ("period",	  DDNS_DEFAULT_PERIOD, CFGF_NONE),
		CFG_INT ("iterations",    DDNS_DEFAULT_ITERATIONS, CFGF_NONE),
		CFG_INT ("forced-update", DDNS_FORCED_UPDATE_PERIOD, CFGF_NONE),
		CFG_SEC ("provider",      popts, CFGF_MULTI | CFGF_TITLE),
		CFG_END()
	};
	cfg_t *cfg = cfg_init(opts, CFGF_NONE);

	/* Validators */
	cfg_set_validate_func(cfg, "period", validate_period);
	cfg_set_validate_func(cfg, "provider", validate_provider);

	switch (cfg_parse(cfg, file)) {
	case CFG_FILE_ERROR:
		fprintf(stderr, "Cannot read configuration file %s: %s\n", file, strerror(errno));
		return NULL;

	case CFG_PARSE_ERROR:
		return NULL;

	case CFG_SUCCESS:
		break;
	}

	/* Set global options */
	ctx->normal_update_period_sec = cfg_getint(cfg, "period");
	ctx->error_update_period_sec  = DDNS_ERROR_UPDATE_PERIOD;
	ctx->forced_update_period_sec = cfg_getint(cfg, "forced-update");
	if (once) {
		debug = 5;
		ctx->total_iterations = 1;
	} else {
		ctx->total_iterations = cfg_getint(cfg, "iterations");
	}
	str                           = cfg_getstr(cfg, "bind");
	ctx->bind_interface           = str ? strdup(str) : NULL;
	ctx->forced_update_fake_addr  = cfg_getbool(cfg, "fake-address");

	/* Set provider options */
	for (i = 0; i < cfg_size(cfg, "provider"); i++) {
		size_t j;
		cfg_t *provider = cfg_getnsec(cfg, "provider", i);
		ddns_info_t *info;
		ddns_system_t *system = plugin_find(cfg_title(provider));

		if (i >= DDNS_MAX_SERVER_NUMBER) {
			logit(LOG_WARNING, "Max number of DDNS providers (%d) reached, skipping %s ...",
			      DDNS_MAX_SERVER_NUMBER, cfg_title(provider));
			continue;
		}

		info = &ctx->info[i];
		info->id     = i;
		info->system = system;

		/* Provider specific options */
		info->wildcard = cfg_getbool(provider, "wildcard");
		info->ssl_enabled = cfg_getbool(provider, "ssl");
		strlcpy(info->creds.username, cfg_getstr(provider, "username"), sizeof(info->creds.username));
		strlcpy(info->creds.password, cfg_getstr(provider, "password"), sizeof(info->creds.password));

		for (j = 0; j < cfg_size(provider, "alias"); j++) {
			str = cfg_getnstr(provider, "alias", j);

			strlcpy(info->alias[j].name, str, sizeof(info->alias[j].name));
			info->alias_count++;
		}

		ctx->info_count++;
	}

	return cfg;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
