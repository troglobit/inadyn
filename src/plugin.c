/* Simplistic plugin support for DDNS service providers
 *
 * Copyright (c) 2012-2014  Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <errno.h>
#include <dlfcn.h>		/* dlopen() et al */
#include <dirent.h>		/* readdir() et al */
#include <string.h>
#ifdef __CYGWIN__
#include <windows.h>
#include <sys/cygwin.h>
#endif

#include "ddns.h"

#define SEARCH_PLUGIN(n)						\
	PLUGIN_ITERATOR(p, tmp) {					\
		if (!strcmp(p->name, n))				\
			return p;					\
	}

static char *plugpath = NULL;   /* Set by first load. */
static TAILQ_HEAD(, ddns_system) plugins = TAILQ_HEAD_INITIALIZER(plugins);

int plugin_register(ddns_system_t *plugin)
{
	if (!plugin) {
		errno = EINVAL;
		return 1;
	}

	if (!plugin->name) {
		char *dli_fname;
#ifdef __CYGWIN__
		char posix_path[MAX_PATH];
		char path[MAX_PATH];
			
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery((void*)&plugin, &mbi, sizeof(mbi));
		GetModuleFileNameA((HINSTANCE)mbi.AllocationBase, path, MAX_PATH);
		cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_RELATIVE, path, posix_path, MAX_PATH);
		dli_fname = posix_path;
#else
		Dl_info info;

		dladdr(plugin, &info);
		dli_fname=(char *)info.dli_fname;
#endif

		if (!dli_fname)
			plugin->name = "unknown";
		else
			plugin->name = (char *)dli_fname;
	}

	/* Already registered? */
	if (plugin_find(plugin->name)) {
		logit(LOG_DEBUG, "... %s already loaded.", plugin->name);
		return 0;
	}

	TAILQ_INSERT_TAIL(&plugins, plugin, link);

	return 0;
}

int plugin_unregister(ddns_system_t *plugin)
{
	TAILQ_REMOVE(&plugins, plugin, link);

	/* XXX: Unfinished, add cleanup code here! */

	return 0;
}


/**
 * plugin_find - Find a plugin by name
 * @name: With or without path, or .so extension
 *
 * This function uses an opporunistic search for a suitable plugin and
 * returns the first match.  Albeit with at least some measure of
 * heuristics.
 *
 * First it checks for an exact match.  If no match is found and @name
 * starts with a slash the search ends.  Otherwise a new search with the
 * plugin path prepended to @name is made.  Also, if @name does not end
 * with .so it too is added to @name before searching.
 *
 * Returns:
 * On success the pointer to the matching &ddns_system_t is returned,
 * otherwise %NULL is returned.
 */
ddns_system_t *plugin_find(const char *name)
{
	ddns_system_t *p, *tmp;

	if (!name) {
		errno = EINVAL;
		return NULL;
	}

	SEARCH_PLUGIN(name);

	if (plugpath && name[0] != '/') {
		int noext;
		char path[256];

		noext = strcmp(name + strlen(name) - 3, ".so");
		snprintf (path, sizeof(path), "%s%s%s%s", plugpath,
			  plugpath[strlen(plugpath) - 1] == '/' ? "" : "/",
			  name, noext ? ".so" : "");

		SEARCH_PLUGIN(path);
	}

	errno = ENOENT;
	return NULL;
}

/* Private daemon API *******************************************************/
#if o
/**
 * load_one - Load one plugin
 * @path: Path to finit plugins, usually %PLUGIN_PATH
 * @name: Name of plugin, optionally ending in ".so"
 *
 * Loads a plugin from @path/@name[.so].  Note, if ".so" is missing from
 * the plugin @name it is added before attempting to load.
 *
 * It is up to the plugin itself ot register itself as a "ctor" with the
 * %PLUGIN_INIT macro so that plugin_register() is called automatically.
 *
 * Returns:
 * POSIX OK(0) on success, non-zero otherwise.
 */
static int load_one(char *path, char *name)
{
	int noext;
	char plugin[256];
	void *handle;

	if (!path || !name) {
		errno = EINVAL;
		return 1;
	}

	/* Compose full path, with optional .so extension, to plugin */
	noext = strcmp(name + strlen(name) - 3, ".so");
	snprintf(plugin, sizeof(plugin), "%s/%s%s", path, name, noext ? ".so" : "");

	logit(LOG_DEBUG, "Loading plugin %s ...", basename(plugin));
	handle = dlopen(plugin, RTLD_LAZY | RTLD_GLOBAL);
	if (!handle) {
		logit(LOG_ERR, "Failed loading plugin %s: %s", plugin, dlerror());
		return 1;
	}

	return 0;
}

int plugin_load_all(char *path)
{
	int fail = 0;
	DIR *dp = opendir(path);
	struct dirent *entry;

	if (!dp) {
		logit(LOG_ERR, "Failed, cannot open plugin directory %s: %s", path, strerror(errno));
		return 1;
	}
	plugpath = path;

	while ((entry = readdir(dp))) {
		if (entry->d_name[0] == '.')
			continue; /* Skip . and .. directories */

		if (load_one(path, entry->d_name))
			fail++;
	}

	closedir(dp);

	return fail;
}
#endif

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
