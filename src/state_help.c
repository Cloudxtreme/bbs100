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
	state_help.c	WJ107
*/

#include "config.h"
#include "debug.h"
#include "state_help.h"
#include "state_msg.h"
#include "state.h"
#include "edit.h"
#include "util.h"
#include "screens.h"
#include "bufprintf.h"
#include "Param.h"

#include <stdio.h>
#include <stdlib.h>


#define HELP_TEXT(x)												\
	bufprintf(filename, sizeof(filename), "%s/" x, PARAM_HELPDIR);	\
	if (load_screen(usr->text, filename) < 0) {						\
		Put(usr, "<red>No help available\n");						\
		break;														\
	}																\
	PUSH(usr, STATE_PRESS_ANY_KEY);									\
	read_text(usr);													\
	Return


void state_help_menu(User *usr, char c) {
char filename[MAX_PATHLEN];

	if (usr == NULL)
		return;

	Enter(state_help_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Put(usr, "<magenta>\n"
				"<hotkey>Introduction                 Editing recipient <hotkey>lists\n"
				"e<hotkey>Xpress messages             Editing with <hotkey>colors\n"
				"<hotkey>Friends and enemies          <hotkey>Navigating the --More-- prompt\n"
			);
			Put(usr,
				"Reading and posting <hotkey>messages\n"
				"The <hotkey>room system\n"
				"Customizing your <hotkey>profile     <hotkey>Other commands\n"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'i':
		case 'I':
			Put(usr, "Introduction\n");
			HELP_TEXT("intro");

		case 'x':
		case 'X':
			Put(usr, "eXpress Messages\n");
			HELP_TEXT("xmsg");

		case 'f':
		case 'F':
			Put(usr, "Friends and Enemies\n");
			HELP_TEXT("friends");

		case 'm':
		case 'M':
			Put(usr, "Reading and posting messages\n");
			HELP_TEXT("msgs");

		case 'r':
		case 'R':
			Put(usr, "The room system\n");
			HELP_TEXT("rooms");

		case 'p':
		case 'P':
			Put(usr, "Customizing your profile\n");
			HELP_TEXT("profile");

		case 'l':
		case 'L':
			Put(usr, "Editing recipient lists\n");
			HELP_TEXT("recipients");

		case 'c':
		case 'C':
			Put(usr, "Editing with colors\n");
			HELP_TEXT("colors");

		case 'n':
		case 'N':
			Put(usr, "Navigating the --More-- prompt\n");
			HELP_TEXT("more");

		case 'o':
		case 'O':
			Put(usr, "Other commands\n");
			HELP_TEXT("other");
	}
	Print(usr, "<yellow>\n[Help] %c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

/* EOB */
