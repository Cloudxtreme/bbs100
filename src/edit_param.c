/*
    bbs100 3.3 WJ107
    Copyright (C) 1997-2015  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
/*
	edit_param.c	WJ105
*/

#include "config.h"
#include "debug.h"
#include "edit_param.h"
#include "edit.h"
#include "cstring.h"
#include "util.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>


void change_int_param(User *usr, char c, int *var) {
int r;

	if (usr == NULL || var == NULL)
		return;

	Enter(change_int_param);

	if (c == INIT_STATE)
		Print(usr, "<green>Enter new value<white> [%d]: <yellow>", *var);

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			r = atoi(usr->edit_buf);
			if (r < 1)
				Put(usr, "<red>Invalid value; not changed\n");
			else {
				*var = r;
				usr->runtime_flags |= RTF_PARAM_EDITED;
			}
		} else
			Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}

/*
	exactly the same as change_int_param(), except that this one
	accepts zero as valid value
*/
void change_int0_param(User *usr, char c, int *var) {
int r;

	if (usr == NULL || var == NULL)
		return;

	Enter(change_int_param);

	if (c == INIT_STATE)
		Print(usr, "<green>Enter new value<white> [%d]: <yellow>", *var);

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			if (!strcmp(usr->edit_buf, "0"))
				r = 0;
			else {
				r = atoi(usr->edit_buf);
				if (r < 1) {
					Put(usr, "<red>Invalid value; not changed\n");
					RET(usr);
					Return;
				}
			}
			*var = r;
			usr->runtime_flags |= RTF_PARAM_EDITED;
		} else
			Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}

void change_octal_param(User *usr, char c, int *var) {
int r;

	if (usr == NULL || var == NULL)
		return;

	Enter(change_octal_param);

	if (c == INIT_STATE)
		Print(usr, "<green>Enter new octal value<white> [0%02o]: <yellow>", *var);

	r = edit_octal_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			if (!is_octal(usr->edit_buf)) {
				Put(usr, "<red>Invalid value\n");
			} else {
				*var = (int)cstrtoul(usr->edit_buf, 8);
				usr->runtime_flags |= RTF_PARAM_EDITED;
			}
		} else
			Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}

/*
	practically copied from change_config() (in state_config.c) :P
	except that this routine sets RTF_PARAM_EDITED
*/
void change_string_param(User *usr, char c, char **var, char *prompt) {
int r;

	if (usr == NULL || var == NULL)
		return;

	Enter(change_string_param);

	if (c == INIT_STATE && prompt != NULL)
		Put(usr, prompt);

	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			cstrip_line(usr->edit_buf);

			if (!usr->edit_buf[0]) {
				Free(*var);
				*var = NULL;
			} else {
				char *s;

				if ((s = cstrdup(usr->edit_buf)) == NULL) {
					Perror(usr, "Out of memory");
					RET(usr);
					Return;
				}
				Free(*var);
				*var = s;
			}
			usr->runtime_flags |= RTF_PARAM_EDITED;
		} else
			if (var != NULL && *var != NULL && **var)
				Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}

/* EOB */
