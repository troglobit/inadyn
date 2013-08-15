/* Custom error logging system
 *
 * Copyright (C) 2003-2006  Narcis Ilisei
 * Copyright (C)      2006  Steve Horbachuk
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

#ifndef _DEBUG_IF_INCLUDED
#define _DEBUG_IF_INCLUDED

#include <stdio.h>
#include "os.h"

typedef struct
{
	int level;
	int verbose;
	char p_logfilename[1024];
	FILE *p_file;
} ddns_dbg_t;


int  get_dbg_dest(void);
void set_dbg_dest(int dest);
void os_printf(int prio, char *fmt, ... );

#ifndef DBG_PRINT_DISABLED
#  ifndef logit
#    define logit(prio, fmt, args...) os_printf(prio, fmt, ##args)
#  endif
#else
#  define logit(fmt, args...)
#endif

#endif /* _DEBUG_IF_INCLUDED */
