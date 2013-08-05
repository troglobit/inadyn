/*
 * Copyright (C) 2003-2004  Narcis Ilisei
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "os.h"
#include "debug_if.h"

#define MAXSTRING 1024

/**
    The dbg destination.
    DBG_SYS_LOG for SysLog
    DBG_STD_LOG for standard console
*/
static DBG_DEST global_mod_dbg_dest = DBG_STD_LOG;
/**
    Returns the dbg destination.
    DBG_SYS_LOG for SysLog
    DBG_STD_LOG for standard console
*/
DBG_DEST get_dbg_dest(void)
{
    return global_mod_dbg_dest;
}

void set_dbg_dest(DBG_DEST dest)
{
    global_mod_dbg_dest = dest;
}

static char *current_time(void)
{
    time_t now;
    struct tm *timeptr;
    static const char wday_name[7][3] = {
	"Sun", "Mon", "Tue", "Wed",
	"Thu", "Fri", "Sat"
    };
    static const char mon_name[12][3] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static char result[26];

    time(&now);
    timeptr = localtime(&now);

    sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %d:",
	wday_name[timeptr->tm_wday], mon_name[timeptr->tm_mon],
	timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min,
	timeptr->tm_sec, 1900 + timeptr->tm_year);

    return result;
}

void os_printf(int prio, char *fmt, ... )
{
    va_list args;
    static char message[MAXSTRING + 1];

    message[MAXSTRING] = 0;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

#ifdef HAVE_OS_SYSLOG
    if (get_dbg_dest() == DBG_SYS_LOG)
    {
	syslog(prio, "%s",message);
    }
    else
#endif
    {
	printf("%s %s\n", current_time(), message);
        fflush(stdout);
    }
}


/**
 * Opens the dbg output for the required destination.
 *
 * WARNING : Open and Close bg output are quite error prone!
 * They should be called din pairs!
 * TODO:
 *  some simple solution that involves storing the dbg output device name (and filename)
 */
RC_TYPE os_open_dbg_output(DBG_DEST dest, const char *p_prg_name, const char *p_logfile_name)
{
    RC_TYPE rc = RC_OK;
    set_dbg_dest(dest);

    switch (get_dbg_dest())
    {
    case DBG_SYS_LOG:
	if (p_prg_name == NULL)
	{
	    rc = RC_INVALID_POINTER;
	    break;
	}
	rc = os_syslog_open(p_prg_name);
	break;

    case DBG_FILE_LOG:
	if (p_logfile_name == NULL)
	{
	    rc = RC_INVALID_POINTER;
	    break;
	}
	{
	    FILE *pF = freopen(p_logfile_name, "ab", stdout);
	    if (pF == NULL)
	    {
		rc = RC_FILE_IO_OPEN_ERROR;
	    }
	    break;
	}
    case DBG_STD_LOG:
    default:
	rc = RC_OK;
    }
    return rc;
}

/**
 * Closes the dbg output device.
 */
RC_TYPE os_close_dbg_output(void)
{
    RC_TYPE rc = RC_OK;
    switch (get_dbg_dest())
    {
    case DBG_SYS_LOG:
	rc = os_syslog_close();
	break;

    case DBG_FILE_LOG:
	fclose(stdout);
	rc = RC_OK;
	break;

    case DBG_STD_LOG:
    default:
	rc = RC_OK;
    }
    return rc;
}
