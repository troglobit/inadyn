/* Custom error logging system
 *
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
 * Copyright (C) 2006       Steve Horbachuk
 * Copyright (C) 2010-2016  Joachim Nilsson <troglobit@gmail.com>
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

#ifndef INADYN_DEBUG_H_
#define INADYN_DEBUG_H_

#include <stdarg.h>
#include "os.h"
#include <stdarg.h>

extern char *ident;

int  loglvl (char *level);
void logit  (int prio, const char *fmt, ...);
void vlogit (int prio, const char *fmt, va_list args);

#endif /* INADYN_DEBUG_H_ */

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
