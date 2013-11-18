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
  Inplementation functions for CMD options parsing system
  Author: Narcis Ilisei
  Date: May 2003
  History:
  - may 15 2003 : first version.
  Basic, table driven option line parsing.
*/

#include <string.h>
#include <stdlib.h>
#include "get_cmd.h"
#include "debug_if.h"

static cmd_desc_t *opt_search(cmd_desc_t *p_table, char *p_opt)
{
	cmd_desc_t *it = p_table;

	while (it->option != NULL) {
		if (strcmp(p_opt, it->option) == 0)
			return it;

		it++;
	}

	return NULL;
}

/**
   Init the cmd_data_t
*/
int cmd_init(cmd_data_t *cmd)
{
	if (!cmd)
		return RC_INVALID_POINTER;

	memset(cmd, 0, sizeof(*cmd));

	return 0;
}

int cmd_destruct(cmd_data_t *cmd)
{
	if (!cmd)
		return RC_INVALID_POINTER;

	if (cmd->argv) {
		int i;

		for (i = 0; i < cmd->argc; ++i) {
			if (cmd->argv[i])
				free(cmd->argv[i]);
		}
		free(cmd->argv);
	}

	return 0;
}

/** Adds a new option (string) to the command line
 */
int cmd_add_val(cmd_data_t *cmd, char *p_val)
{
	char *p, **pp;

	if (!cmd || !p_val)
		return RC_INVALID_POINTER;

	pp = realloc(cmd->argv, (cmd->argc + 1) * sizeof(char *));
	if (!pp)
		return RC_OUT_OF_MEMORY;

	cmd->argv = pp;

	p = malloc(strlen(p_val) + 1);
	if (!p)
		return RC_OUT_OF_MEMORY;

	strcpy(p, p_val);
	cmd->argv[cmd->argc] = p;
	cmd->argc++;

	return 0;
}

/** Creates a struct of argvals from the given command line.
    Action:
    copy the argv from the command line to the given cmd_data_t struct
    set the data val of the list element to the current argv
*/
int cmd_add_vals_from_argv(cmd_data_t *cmd, char **argv, int argc)
{
	int i;
	int rc = 0;

	if (!cmd || !argv || !argc)
		return RC_INVALID_POINTER;

	for (i = 0; i < argc; ++i) {
		rc = cmd_add_val(cmd, argv[i]);
		if (rc != 0)
			break;
	}

	return rc;
}

/*
  Parses the incoming argv list.
  Arguments:
  argv, argc,
  cmd description

  Action:
  performs a match for every option string in the CMD description.
  checks the number of arguments left
  calls the user handler with the pointer to the correct arguments

  Implementation:
  - for each option in the table
  - find it in the argv list
  - check the required number of arguments
  - call the handler
*/
int get_cmd_parse_data(char **argv, int argc, cmd_desc_t *desc)
{
	int rc = 0;
	cmd_data_t cmd;
	int current = 1;	/* Skip daemon name */

	if (argv == NULL || desc == NULL)
		return RC_INVALID_POINTER;

	do {
		rc = cmd_init(&cmd);
		if (rc != 0)
			break;

		rc = cmd_add_vals_from_argv(&cmd, argv, argc);
		if (rc != 0)
			break;

		while (current < cmd.argc) {
			cmd_desc_t *ptr = opt_search(desc, cmd.argv[current]);

			if (ptr == NULL) {
				rc = RC_CMD_PARSER_INVALID_OPTION;
				logit(LOG_WARNING,
				      "Invalid option name at position %d: %s", current + 1, cmd.argv[current]);
				break;
			}
//                      logit(LOG_NOTICE, "Found opt %d: %s", current, cmd.argv[current]);

			++current;

			/*check arg nr required by the current option */
			if (current + ptr->argno > cmd.argc) {
				rc = RC_CMD_PARSER_INVALID_OPTION_ARGUMENT;
				logit(LOG_WARNING,
				      "Missing option value at position %d: %s", current + 1, ptr->option);
				break;
			}

			rc = ptr->handler.func(&cmd, current, ptr->handler.context);
			if (rc != 0) {
				logit(LOG_WARNING, "Error parsing option %s", cmd.argv[current - 1]);
				break;
			}

			current += ptr->argno;
		}
	}
	while (0);

	cmd_destruct(&cmd);

	return rc;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
