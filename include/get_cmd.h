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
	Interface functions for CMD options parsing system
	Author: Narcis Ilisei
	Date: May 2003
	History:
		- may 15 2003 : first version.
			Basic, table driven option line parsing.
*/

#ifndef _GET_CMD_IF_INCLUDED
#define _GET_CMD_IF_INCLUDED


#include "errorcode.h"
typedef struct
{
        int            argc;
        char         **argv;
} cmd_data_t;

typedef int (*cmd_func_t)(cmd_data_t *cmd, int no, void *context);

typedef struct
{
	cmd_func_t     func;
	void          *context;
} cmd_handler_t;

typedef struct
{
	char          *option;
	int            argno;
	cmd_handler_t  handler;
	char          *description;
} cmd_desc_t;

int get_cmd_parse_data(char **argv, int argc, cmd_desc_t *desc);
int cmd_add_val(cmd_data_t *cmd, char *val);

#endif /*_GET_CMD_IF_INCLUDED*/

