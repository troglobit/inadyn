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

cfg_t *parse_conf(char *file)
{
	cfg_opt_t provider_opts[] = {
		CFG_STR     ("username",  0, CFGF_NONE),
		CFG_STR     ("password",  0, CFGF_NONE),
		CFG_STR_LIST("alias",     0, CFGF_NONE),
		CFG_BOOL    ("ssl",       cfg_false, CFGF_NONE),
		CFG_BOOL    ("wildcard",  cfg_false, CFGF_NONE),
		CFG_END()
	};
	cfg_opt_t opts[] = {
		CFG_BOOL("syslog",	  cfg_false, CFGF_NONE),
		CFG_BOOL("fake-address",  cfg_false, CFGF_NONE),
		CFG_STR ("bind",	  0, CFGF_NONE),
		CFG_INT ("period",	  60, CFGF_NONE),
		CFG_INT ("iterations",    0, CFGF_NONE),
		CFG_INT ("forced-update", 720000, CFGF_NONE),
		CFG_SEC ("provider", provider_opts, CFGF_MULTI | CFGF_TITLE),
		CFG_END()
	};
	cfg_t *cfg = cfg_init(opts, CFGF_NONE);

	switch (cfg_parse(cfg, file)) {
	case CFG_FILE_ERROR:
		fprintf(stderr, "Cannot read configuration file %s: %s\n", conf, strerror(errno));
		/* Fall through. */

	case CFG_PARSE_ERROR:
		return NULL;

	case CFG_SUCCESS:
		break;
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
