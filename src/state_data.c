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
	state_data.c	WJ105
*/

#include "config.h"
#include "state_data.h"
#include "edit.h"
#include "DataCmd.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>


void state_data_conn(User *usr, char c) {
int r;

	Enter(state_data_conn);

	if (c == INIT_STATE) {
		edit_data_cmd(usr, EDIT_INIT);
		Return;
	}
	r = edit_data_cmd(usr, c);

	if (r == EDIT_BREAK) {
		edit_data_cmd(usr, EDIT_INIT);
		Return;
	}
	if (r == EDIT_RETURN) {
		exec_cmd(usr, usr->edit_buf);
		edit_data_cmd(usr, EDIT_INIT);
		Return;
	}
	Return;
}

/* EOB */
