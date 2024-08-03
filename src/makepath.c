/* mkpath() -- Create all components leading up to a given directory
 *
 * Copyright (c) 2013-2024  Joachim Wiberg <troglobit@gmail.com>
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
#include <string.h>		/* strdup(), strrchr() */
#include <stdlib.h>		/* free() */
#include <sys/stat.h>		/* mkdir() */

#include "compat.h"

/* Recursively create directories */
static int _mkpath(char *dir, mode_t mode)
{
	char *slash;

	if (!mkdir(dir, mode) || errno == EEXIST)
		return 0;

	if (errno != ENOENT)
		return -1;

	slash = strrchr(dir, '/');
	if (!slash)
		return -1;

	*slash = 0;
	if (_mkpath(dir, mode) == -1)
		return -1;

	*slash = '/';
	return mkdir(dir, mode);
}

/**
 * mkpath - Like makepath() but takes a mode_t argument
 * @dir:  Directory to created, relative or absolute
 * @mode: A &mode_t mode to create @dir with
 *
 * Returns:
 * POSIX OK(0) on success, otherwise -1 with @errno set.
 */
int mkpath(const char *dir, mode_t mode)
{
	char *_dir;
	int rc;

	if (!dir) {
		errno = EINVAL;
		return 1;
	}

	_dir = strdup(dir);
	if (!_dir)
		return -1;

	rc = _mkpath(_dir, mode);
	free(_dir);

	return rc;
}

/**
 * makepath - Create all components of the specified directory.
 * @dir: Directory to create.
 *
 * Returns:
 * POSIX OK (0) on success, otherwise -1 and errno set appropriately.
 * This function returns EINVAL on bad argument, or ENOMEM when it
 * fails allocating temporary memory.  For other error codes see the
 * mkdir() syscall description.
 */
int makepath(const char *dir)
{
	return mkpath(dir, 0777);
}
