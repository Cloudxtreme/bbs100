/*
    bbs100 2.2 WJ105
    Copyright (C) 2005  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
	DataCmd.c	WJ105
*/

#include "config.h"
#include "DataCmd.h"
#include "cstring.h"
#include "debug.h"
#include "version.h"
#include "util.h"
#include "inet.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

DataCmd default_cmds[] = {
	{ "logout",		cmd_logout		},
	{ "exit",		cmd_logout		},
	{ "quit",		cmd_logout		},
	{ "close",		cmd_close		},
	{ "commands",	cmd_commands	},
	{ "version",	cmd_version		},
};

DataCmd login_cmds[] = {
	{ "login",		cmd_login		},
};


int exec_cmd(User *usr, char *cmd) {
int i;
char **argv = NULL, *p;
PList *pl;
DataCmd *cmds;

	if (cmd == NULL || !*cmd)
		return -1;

	if ((p = cstrchr(cmd, ' ')) != NULL) {
		*p = 0;
		p++;
		argv = cstrsplit(p, ' ');
	}
	for(pl = usr->cmd_chain; pl != NULL; pl = pl->next) {
		cmds = (DataCmd *)(pl->p);
		for(i = 0; cmds[i].cmd != NULL; i++) {
			if (!cstricmp(cmds[i].cmd, cmd)) {
				default_cmds[i].func(usr, argv);
				Free(argv);
				return 0;
			}
		}
	}
	Put(usr, "error unknown command\n");
	Free(argv);
	return -1;
}


void cmd_close(User *usr, char **argv) {
char buf[MAX_NAME];

	Enter(cmd_close);

	strcpy(buf, usr->name);
	usr->name[0] = 0;

	if (buf[0])
		close_connection(usr, "%s closes connection from %s", buf, usr->from_ip);
	else
		close_connection(usr, "closing connection from %s", usr->from_ip);

	Return;
}

void cmd_commands(User *usr, char **argv) {
int i;
PList *pl;
DataCmd *cmds;

	Enter(cmd_commands);

	for(pl = usr->cmd_chain; pl != NULL; pl = pl->next) {
		cmds = (DataCmd *)(pl->p);
		for(i = 0; cmds[i].cmd != NULL; i++)
			Print(usr, " %s\n", default_cmds[i].cmd);
	}
	Put(usr, "\n");
	Return;
}

void cmd_version(User *usr, char **argv) {
	Print(usr, " %s\n\n", VERSION);
}

void cmd_logout(User *usr, char **argv) {
	Enter(cmd_logout);

	if (usr->name[0])
		close_connection(usr, "%s has logged out from %s", usr->name, usr->from_ip);
	else
		close_connection(usr, "logout from %s", usr->from_ip);

	Return;
}

void cmd_login(User *usr, char **argv) {
	Enter(cmd_login);

	Return;
}

/* EOB */
