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
	edit.c	WJ99
*/

#include "config.h"
#include "debug.h"
#include "edit.h"
#include "access.h"
#include "util.h"
#include "log.h"
#include "cstring.h"
#include "Param.h"
#include "OnlineUser.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*
	Returns:
		-1 on Ctrl-C or Ctrl-D
		 0 on 'continue'
		 1 on Ok, edit name is done
		   list of names is in usr->recipients
*/
int edit_recipients(User *usr, char c, int (*access_func)(User *)) {
StringList *sl;
int i;
char many_buf[MAX_LINE*3];

	if (usr == NULL)
		return 0;

	reset_tablist(usr, c);		/* reset the 'tab' name extension list */

	switch(c) {
		case EDIT_INIT:
			usr->runtime_flags &= ~RTF_MULTI;
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_buf[0] = 0;
			usr->edit_pos = 0;
			break;

		case KEY_TAB:
			tab_list(usr, make_users_tablist);
			break;

		case KEY_BACKTAB:
			backtab_list(usr, make_users_tablist);
			break;

		case KEY_BS:
			if (usr->edit_pos > 0) {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b \b");
			} else {
				if (usr->recipients != NULL) {
					erase_many(usr);

					for(sl = usr->recipients; sl != NULL && sl->next != NULL; sl = sl->next);
					strcpy(usr->edit_buf, sl->str);
					usr->edit_pos = strlen(usr->edit_buf);

					remove_StringList(&usr->recipients, sl);
					destroy_StringList(sl);
					if (usr->recipients == NULL)
						usr->runtime_flags &= ~RTF_MULTI;

					Print(usr, "\b \b\b \b%s", print_many(usr, many_buf));
				}
			}
			break;

		case KEY_CTRL('L'):
			if (usr->edit_pos > 0)
				Put(usr, "\n");

			if (usr->recipients == NULL)
				Put(usr, "<magenta>The recipient list is empty");
			else {
				Put(usr, "<magenta>List recipients: ");
				for(sl = usr->recipients; sl != NULL; sl = sl->next) {
					Print(usr, "<yellow>%s", sl->str);
					if (sl->next != NULL)
						Put(usr, "<green>, ");
				}
			}
			Print(usr, "\n<green>Enter recipient%s", print_many(usr, many_buf));
			break;

		case KEY_CTRL('F'):
			if (usr->friends == NULL)
				break;

			erase_name(usr);
			erase_many(usr);
			Put(usr, "\b\b");

			if (!(usr->runtime_flags & RTF_MULTI)) {
				listdestroy_StringList(usr->recipients);
				usr->recipients = NULL;
				usr->runtime_flags |= RTF_MULTI;
			}
			for(sl = usr->friends; sl != NULL; sl = sl->next)
				if (in_StringList(usr->recipients, sl->str) == NULL) {
/* this is a kind of hack; you may multi-mail to offline friends */
					if (access_func != multi_mail_access && is_online(sl->str) == NULL)
						continue;

					add_StringList(&usr->recipients, new_StringList(sl->str));
			}
			Print(usr, "%s", print_many(usr, many_buf));
			break;

/*
	talked-to list: donated by Richard of MatrixBBS
	(same as for the friendlist, but now with usr->talked_to)
*/
		case KEY_CTRL('T'):
			if (!PARAM_HAVE_TALKEDTO || usr->talked_to == NULL)
				break;

			erase_name(usr);
			erase_many(usr);
			Put(usr, "\b\b");

			if (!(usr->runtime_flags & RTF_MULTI)) {
				listdestroy_StringList(usr->recipients);
				usr->recipients = NULL;
				usr->runtime_flags |= RTF_MULTI;
			}
			for(sl = usr->talked_to; sl != NULL; sl = sl->next)
				if (in_StringList(usr->recipients, sl->str) == NULL) {
/* this is a kind of hack; you may multi-mail to ppl you've talked with */
					if (access_func != multi_mail_access && is_online(sl->str) == NULL)
						continue;

					add_StringList(&usr->recipients, new_StringList(sl->str));
			}
			Print(usr, "%s", print_many(usr, many_buf));
			break;

		case KEY_CTRL('W'):
			if (usr->runtime_flags & RTF_SYSOP) {
				User *u;

				erase_name(usr);
				erase_many(usr);
				Put(usr, "\b\b");

				if (!(usr->runtime_flags & RTF_MULTI)) {
					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;
					usr->runtime_flags |= RTF_MULTI;
				}
				for(u = AllUsers; u != NULL; u = u->next) {
					if (u->name[0])
						usr->recipients = add_StringList(&usr->recipients, new_StringList(u->name));
				}
				usr->recipients = rewind_StringList(usr->recipients);
				Print(usr, "%s", print_many(usr, many_buf));
			}
			break;

		case KEY_CTRL('A'):
			if (usr->flags & USR_FOLLOWUP) {
				usr->flags &= ~USR_FOLLOWUP;
				Put(usr, "\n<magenta>Follow up mode aborted\n");
			}
			c = KEY_CTRL('C');

		case KEY_CTRL('R'):				/* remove from list */
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('U'):
		case KEY_CTRL('X'):
		case KEY_ESC:
			erase_name(usr);
			switch(c) {
				case KEY_CTRL('R'):
					if ((sl = in_StringList(usr->recipients, usr->edit_buf)) != NULL) {
						usr->edit_pos = 0;
						usr->edit_buf[0] = 0;

						erase_many(usr);
						remove_StringList(&usr->recipients, sl);
						destroy_StringList(sl);

						Print(usr, "\b \b\b \b%s", print_many(usr, many_buf));
					}
					usr->edit_pos = 0;
					usr->edit_buf[0] = 0;
					break;

				case KEY_CTRL('C'):
				case KEY_CTRL('D'):
					usr->edit_pos = 0;
					usr->edit_buf[0] = 0;
					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;
					Put(usr, "\n");
					return EDIT_BREAK;

				case KEY_ESC:
				case KEY_CTRL('X'):
					break;
			}
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		case KEY_RETURN:
			if (usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == ' ') {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
			}
			if (!usr->edit_pos) {
				if (usr->runtime_flags & RTF_MULTI) {
					usr->runtime_flags &= ~RTF_MULTI;
					Put(usr, "\b\b: \n");
				} else
					Put(usr, "\n");
				return EDIT_RETURN;
			}
			if (!(usr->runtime_flags & RTF_MULTI)) {
				listdestroy_StringList(usr->recipients);
				usr->recipients = NULL;
			}
/*
	if it's already in the list, then erase it
*/
			if (in_StringList(usr->recipients, usr->edit_buf) != NULL) {
				erase_name(usr);
				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;
				break;
			}
			if (access_func == NULL || !access_func(usr))
				break;

			if ((sl = new_StringList(usr->edit_buf)) == NULL) {
				Perror(usr, "Out of memory");
			} else {
				if (!(usr->runtime_flags & RTF_MULTI)) {
					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;
				}
				add_StringList(&usr->recipients, sl);
			}
			usr->edit_buf[0] = 0;
			usr->edit_pos = 0;
			Put(usr, "\n");
			return EDIT_RETURN;

		case ',':
			if (usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == ' ') {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b");
			}
			if (usr->edit_pos > 0 && !(usr->runtime_flags & RTF_MULTI)) {
				erase_many(usr);
				listdestroy_StringList(usr->recipients);
				usr->recipients = NULL;
			}
			if (!usr->edit_pos) {
				if (usr->recipients != NULL) {
					Put(usr, "\b\b, ");
					usr->runtime_flags |= RTF_MULTI;
				}
				break;
			}
			if (in_StringList(usr->recipients, usr->edit_buf) != NULL) {
				erase_name(usr);
				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;
				break;
			}
			if (access_func == NULL || !access_func(usr))
				break;

			erase_name(usr);
			if ((sl = new_StringList(usr->edit_buf)) == NULL) {
				Put(usr, "\n");
				Perror(usr, "Out of memory");

				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;
				return EDIT_BREAK;
			} else {
				usr->runtime_flags |= RTF_MULTI;

				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;

				if (usr->recipients == NULL) {
					add_StringList(&usr->recipients, sl);
					Print(usr, "\b \b\b \b%s", print_many(usr, many_buf));
				} else {
					if (usr->recipients->next == NULL) {
						erase_many(usr);
						add_StringList(&usr->recipients, sl);
						Print(usr, "\b \b\b \b%s", print_many(usr, many_buf));
					} else
						add_StringList(&usr->recipients, sl);
				}
			}
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		case ' ':
			if (!usr->edit_pos)
				break;

			if (usr->edit_buf[usr->edit_pos-1] == ' ')
				break;

			usr->edit_buf[usr->edit_pos++] = ' ';
			usr->edit_buf[usr->edit_pos] = 0;
			Put(usr, " ");
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0':
			if (!usr->edit_pos) {
				if (!PARAM_HAVE_QUICK_X)
					break;

				if (c == '0')
					i = 9;
				else
					i = c - '1';

				if (usr->quick[i] != NULL) {
					erase_name(usr);
					strcpy(usr->edit_buf, usr->quick[i]);
					Put(usr, usr->edit_buf);
					usr->edit_pos = strlen(usr->edit_buf);
				}
				break;
			}
/* else fall through and enter a number */

		default:
			if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
				|| (c >= '0' && c <= '9')))
				break;

			if (usr->edit_pos < MAX_NAME-1) {
				if ((!usr->edit_pos || usr->edit_buf[usr->edit_pos-1] == ' ')
					&& c >= 'a' && c <= 'z')
					c -= ' ';				/* auto uppercase */

				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
				Print(usr, "%c", c);
			}
	}
	return 0;
}

int edit_name(User *usr, char c) {
char many_buf[MAX_LINE*3];

	if (usr == NULL)
		return 0;

	switch(c) {
		case EDIT_INIT:
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;
			break;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			erase_name(usr);
			Put(usr, "\n");
			return EDIT_BREAK;

		case KEY_RETURN:
			if (usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == ' ') {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
			}
			Put(usr, "\n");
			return EDIT_RETURN;

		case KEY_BS:
			if (usr->edit_pos > 0) {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b \b");
			} else {
				if (usr->recipients != NULL) {
					StringList *sl;

					erase_many(usr);

					for(sl = usr->recipients; sl != NULL && sl->next != NULL; sl = sl->next);
					strcpy(usr->edit_buf, sl->str);
					usr->edit_pos = strlen(usr->edit_buf);

					remove_StringList(&usr->recipients, sl);
					destroy_StringList(sl);
					if (usr->recipients == NULL)
						usr->runtime_flags &= ~RTF_MULTI;

					Print(usr, "\b \b\b \b%s", print_many(usr, many_buf));
				}
			}
			break;

		case KEY_ESC:
		case KEY_CTRL('U'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('X'):
			erase_name(usr);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		case ' ':
			if (!usr->edit_pos)
				break;

			if (usr->edit_buf[usr->edit_pos-1] == ' ')
				break;

			usr->edit_buf[usr->edit_pos++] = ' ';
			usr->edit_buf[usr->edit_pos] = 0;
			Put(usr, " ");
			break;

		default:
			if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
				|| (c >= '0' && c <= '9')))
				break;

			if (usr->edit_pos < MAX_NAME-1) {
				if (!usr->edit_pos && (c >= '0' && c <= '9'))
					break;

				if ((!usr->edit_pos || usr->edit_buf[usr->edit_pos-1] == ' ')
					&& c >= 'a' && c <= 'z')
					c -= ' ';						/* auto uppercase */

				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
				Print(usr, "%c", c);
			}
	}
	return 0;
}

int edit_tabname(User *usr, char c) {
int i;

	if (usr == NULL)
		return 0;

	reset_tablist(usr, c);		/* reset the 'tab' name extension list */

	switch(c) {
		case KEY_TAB:
			tab_list(usr, make_users_tablist);
			break;

		case KEY_BACKTAB:
			backtab_list(usr, make_users_tablist);
			break;

		case KEY_RETURN:
			if (usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == ' ') {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
			}
			if (!usr->edit_pos) {
				if (usr->recipients != NULL && usr->recipients->str != NULL) {
					strcpy(usr->edit_buf, usr->recipients->str);
					usr->edit_pos = strlen(usr->edit_buf);
				}
			}
			Put(usr, "\n");
			return EDIT_RETURN;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0':
			if (!usr->edit_pos) {
				if (c == '0')
					i = 9;
				else
					i = c - '1';

				if (usr->quick[i] != NULL) {
					erase_name(usr);
					strcpy(usr->edit_buf, usr->quick[i]);
					Put(usr, usr->edit_buf);
					usr->edit_pos = strlen(usr->edit_buf);
				}
				break;
			}
/* else fall through and enter a number */
		default:
			return edit_name(usr, c);
	}
	return 0;
}

int edit_roomname(User *usr, char c) {
	if (usr == NULL)
		return 0;

	Enter(edit_roomname);

	reset_tablist(usr, c);		/* reset the 'tab' name extension list */

	switch(c) {
		case EDIT_INIT:
			usr->runtime_flags |= RTF_BUSY;
			usr->runtime_flags &= ~RTF_NUMERIC_ROOMNAME;
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			erase_name(usr);
			usr->runtime_flags &= ~RTF_NUMERIC_ROOMNAME;
			Put(usr, "\n");
			Return EDIT_BREAK;

		case KEY_RETURN:
			if (usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == ' ') {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
			}
			usr->runtime_flags &= ~RTF_NUMERIC_ROOMNAME;
			Put(usr, "\n");
			Return EDIT_RETURN;

		case KEY_TAB:
			if (!(usr->runtime_flags & RTF_NUMERIC_ROOMNAME))
				tab_list(usr, make_rooms_tablist);
			break;

		case KEY_BACKTAB:
			if (!(usr->runtime_flags & RTF_NUMERIC_ROOMNAME))
				backtab_list(usr, make_rooms_tablist);
			break;

/* the rest of this func is the same as edit_name(), except for the MAX_LINE */
		case KEY_BS:
			if (usr->edit_pos) {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b \b");

				if (!usr->edit_pos)
					usr->runtime_flags &= ~RTF_NUMERIC_ROOMNAME;
			}
			break;

		case KEY_ESC:
		case KEY_CTRL('U'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('X'):
			erase_name(usr);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			usr->runtime_flags &= ~RTF_NUMERIC_ROOMNAME;
			break;

		case ' ':
			if (!usr->edit_pos || (usr->runtime_flags & RTF_NUMERIC_ROOMNAME))
				break;

			if (usr->edit_pos >= MAX_LINE-1)
				break;

			if (usr->edit_buf[usr->edit_pos-1] == ' ')
				break;

			usr->edit_buf[usr->edit_pos++] = ' ';
			usr->edit_buf[usr->edit_pos] = 0;
			Put(usr, " ");
			break;

		default:
			if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
				|| (c >= '0' && c <= '9') || c == '\''))
				break;

			if (usr->edit_pos < MAX_LINE-1) {
				if (!usr->edit_pos && c >= '0' && c <= '9')
					usr->runtime_flags |= RTF_NUMERIC_ROOMNAME;

				if ((usr->runtime_flags & RTF_NUMERIC_ROOMNAME)
					&& !(c >= '0' && c <= '9'))
					break;

				if ((!usr->edit_pos || usr->edit_buf[usr->edit_pos-1] == ' ')
					&& c >= 'a' && c <= 'z')
					c -= ' ';						/* auto uppercase */

				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
				Print(usr, "%c", c);
			}
	}
	Return 0;
}

int edit_password(User *usr, char c) {
	if (usr == NULL)
		return 0;

	switch(c) {
		case EDIT_INIT:
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			erase_name(usr);
			Put(usr, "\n");
			return EDIT_BREAK;

		case KEY_RETURN:
			Put(usr, "\n");
			return EDIT_RETURN;


		case KEY_BS:
			if (usr->edit_pos) {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b \b");
			}
			break;

		case KEY_ESC:
		case KEY_CTRL('U'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('X'):
			erase_name(usr);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		default:
			if (c < ' ' || c > '~')
				break;

			if (usr->edit_pos < MAX_LINE-1) {
				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "*");
			}
	}
	return 0;
}

int edit_line(User *usr, char c) {
	if (usr == NULL)
		return 0;

	if (c == EDIT_INIT) {
		usr->runtime_flags |= RTF_BUSY;
		usr->runtime_flags &= ~RTF_COLOR_EDITING;
		usr->edit_pos = 0;
		usr->edit_buf[0] = 0;
		return 0;
	}
	if (usr->runtime_flags & RTF_COLOR_EDITING) {
		edit_color(usr, c);
		return 0;
	}
	switch(c) {
/*
		case EDIT_INIT:					already done (see above)
			break;
*/

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			Put(usr, "\n");
			return EDIT_BREAK;

		case KEY_RETURN:
			Put(usr, "\n");
			return EDIT_RETURN;

		case KEY_BS:
			if (usr->edit_pos) {
				usr->edit_pos--;
				if (usr->edit_buf[usr->edit_pos] >= ' ' && usr->edit_buf[usr->edit_pos] <= '~')
					Put(usr, "\b \b");
				else
					Put(usr, "<yellow>");
				usr->edit_buf[usr->edit_pos] = 0;
			}
			break;

		case KEY_CTRL('W'):
			erase_word(usr);
			break;

		case KEY_ESC:
		case KEY_CTRL('U'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('X'):
			erase_line(usr, usr->edit_buf);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			Put(usr, "<yellow>");
			break;

		case KEY_CTRL('V'):
			usr->runtime_flags |= RTF_COLOR_EDITING;
			break;

		default:
			if (c < ' ' || c > '~')
				break;

			if (usr->edit_pos < MAX_LINE-1) {
				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
				Print(usr, "%c", c);
			}
	}
	return 0;
}

int edit_x(User *usr, char c) {
int wrapable = 0;

	if (usr == NULL)
		return 0;

	if (c == EDIT_INIT) {
		usr->runtime_flags |= RTF_BUSY;
		usr->runtime_flags &= ~RTF_COLOR_EDITING;
		usr->edit_pos = usr->total_lines = 0;
		usr->edit_buf[0] = 0;

		listdestroy_StringList(usr->more_text);
		usr->more_text = NULL;
		return 0;
	}
	if (usr->total_lines >= PARAM_MAX_XMSG_LINES) {
		if (c == KEY_CTRL('C')) {
			Put(usr, "<cr>                                                          <cr>");
			usr->more_text = rewind_StringList(usr->more_text);
			return EDIT_BREAK;
		}
		if (c == KEY_CTRL('X')) {
			Put(usr, "<cr>                                                          <cr>");
			usr->more_text = rewind_StringList(usr->more_text);
			return EDIT_RETURN;
		}
		Put(usr, "<cr><red>Too many lines, press <white><<yellow>Ctrl-C<white>><red> to abort, <white><<yellow>Ctrl-X<white>><red> to send");
		return 0;
	}
	if (usr->runtime_flags & RTF_COLOR_EDITING) {
		edit_color(usr, c);
		return 0;
	}
	switch(c) {
/*
		case EDIT_INIT:					already done (see above)
			break;
*/
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			Put(usr, "\n");
			usr->more_text = rewind_StringList(usr->more_text);
			return EDIT_BREAK;

		case KEY_RETURN:
			if (usr->edit_buf[0]) {
/*
	added by Shannon Prickett <spameater@metanav.org>
	'ABORT' aborts an X message (like it does in DOC)
*/
				if (!strcmp(usr->edit_buf, "ABORT")) {
					usr->more_text = rewind_StringList(usr->more_text);
					Put(usr, "\n");
					return EDIT_BREAK;
				}
				usr->more_text = add_StringList(&usr->more_text, new_StringList(usr->edit_buf));

				usr->total_lines++;
				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;

				if (usr->total_lines < PARAM_MAX_XMSG_LINES) {
					Put(usr, "\n>");
					break;
				}
			}
			Put(usr, "\n");
			usr->more_text = rewind_StringList(usr->more_text);
			return EDIT_RETURN;


		case KEY_BS:
			if (usr->edit_pos) {
				usr->edit_pos--;
				if (usr->edit_buf[usr->edit_pos] >= ' ' && usr->edit_buf[usr->edit_pos] <= '~')
					Put(usr, "\b \b");
				else
					Put(usr, "<green>");
				usr->edit_buf[usr->edit_pos] = 0;
			}
			break;

		case KEY_ESC:
		case KEY_CTRL('U'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('X'):
			erase_line(usr, usr->edit_buf);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			Put(usr, "<green>");
			break;

		case KEY_CTRL('V'):
			usr->runtime_flags |= RTF_COLOR_EDITING;
			if (usr->edit_pos >= MAX_LINE-2) {		/* wrap color to next line */
				usr->more_text = add_StringList(&usr->more_text, new_StringList(usr->edit_buf));
				usr->total_lines++;
				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;
			}
			break;
/*
	Eat words in X line, to be more like DOC
	contributed by Shannon Prickett <spameater@metanav.org>
*/
		case KEY_CTRL('W'):
			erase_word(usr);
			break;

		default:
			if (c < ' ' || c > '~')
				break;

			if (usr->edit_pos < MAX_LINE-2) {
				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
				Print(usr, "%c", c);
			} else {
				int i;
				char buf[MAX_LINE*3] = "", buf2[MAX_LINE];

/* word wrap */
/* improved by Fearlezz of MatrixBBS */
				i = usr->edit_pos;
				while(i > (MAX_LINE >> 1)) {
					if (usr->edit_buf[i] == ' ')
						wrapable = 1;
					i--;
				}
				if (!wrapable)
					i = MAX_LINE-1;
				else {
					i = usr->edit_pos;
					if (wrapable)
						while(i > (MAX_LINE >> 1) && usr->edit_buf[i] != ' ') {
							if (usr->edit_buf[i] >= ' ' && usr->edit_buf[i] <= '~')
								strcat(buf, "\b \b");
							i--;
						}
				}
				i++;
				strcpy(buf2, usr->edit_buf+i);
				usr->edit_buf[i] = 0;

/* add new line */
				usr->more_text = add_StringList(&usr->more_text, new_StringList(usr->edit_buf));

/* wrap word to next line */
				usr->edit_pos = strlen(buf2);
				buf2[usr->edit_pos++] = c;
				buf2[usr->edit_pos] = 0;

				Print(usr, "%s\n>%s", buf, buf2);
				strcpy(usr->edit_buf, buf2);
				usr->total_lines++;
			}
	}
	return 0;
}

int edit_msg(User *usr, char c) {
int wrapable = 0;

	if (usr == NULL)
		return 0;

	if (c == EDIT_INIT) {
		usr->runtime_flags |= RTF_BUSY;
		usr->runtime_flags &= ~RTF_COLOR_EDITING;
		usr->edit_pos = usr->total_lines = 0;
		usr->edit_buf[0] = 0;

		listdestroy_StringList(usr->more_text);
		usr->more_text = NULL;
		Put(usr, "<yellow>");
		return 0;
	}
	if (usr->total_lines >= PARAM_MAX_MSG_LINES) {
		if (c == KEY_CTRL('C')) {
			Put(usr, "<cr>                                 \n");
			return EDIT_BREAK;
		}
		Put(usr, "<cr><red>Too many lines, press <white><<yellow>Ctrl-C<white>>");
		return 0;
	}
	if (usr->runtime_flags & RTF_COLOR_EDITING) {
		edit_color(usr, c);
		return 0;
	}
	switch(c) {
/*
		case EDIT_INIT:				already done (see above)
			break;
*/

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			Put(usr, "\n");
			return EDIT_BREAK;

		case KEY_RETURN:
			Put(usr, "\n");
			return EDIT_RETURN;


		case KEY_BS:
			if (usr->edit_pos) {
				usr->edit_pos--;
				if (usr->edit_buf[usr->edit_pos] >= ' ' && usr->edit_buf[usr->edit_pos] <= '~')
					Put(usr, "\b \b");
				else
					Put(usr, "<green>");
				usr->edit_buf[usr->edit_pos] = 0;
			}
			break;

		case KEY_ESC:
		case KEY_CTRL('U'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('X'):
			erase_line(usr, usr->edit_buf);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			Put(usr, "<green>");
			break;

		case KEY_CTRL('V'):
			usr->runtime_flags |= RTF_COLOR_EDITING;
			if (usr->edit_pos >= MAX_LINE-2) {		/* wrap color to next line */
				usr->more_text = add_StringList(&usr->more_text, new_StringList(usr->edit_buf));
				usr->total_lines++;
				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;
			}
			break;

/*
	Eat words in post line, to be more like DOC
	contributed by Shannon Prickett <spameater@metanav.org>
*/
		case KEY_CTRL('W'):
			erase_word(usr);
			break;

		default:
			if (c < ' ' || c > '~')
				break;

			if (usr->edit_pos < MAX_LINE-2) {
				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
				Print(usr, "%c", c);
			} else {
				int i;
				char buf[MAX_LINE*3] = "", buf2[MAX_LINE];

/* word wrap */
/* improved by Fearlezz of MatrixBBS */
				i = usr->edit_pos;
				while(i > (MAX_LINE >> 1)) {
					if (usr->edit_buf[i] == ' ')
						wrapable = 1;
					i--;
				}
				if (!wrapable)
					i = MAX_LINE-1;
				else {
					i = usr->edit_pos;

					if (wrapable)
						while(i > (MAX_LINE >> 1) && usr->edit_buf[i] != ' ') {
							if (usr->edit_buf[i] >= ' ' && usr->edit_buf[i] <= '~')
								strcat(buf, "\b \b");
							i--;
						}
				}
				strcat(buf, "\n");
				Put(usr, buf);
				i++;
				strcpy(buf2, usr->edit_buf+i);
				usr->edit_buf[i] = 0;

/* add new line */
				usr->more_text = add_StringList(&usr->more_text, new_StringList(usr->edit_buf));

/* wrap word to next line */
				usr->edit_pos = strlen(buf2);
				buf2[usr->edit_pos++] = c;
				buf2[usr->edit_pos] = 0;

				Print(usr, "%s", buf2);
				strcpy(usr->edit_buf, buf2);
				usr->total_lines++;
			}
	}
	return 0;
}

int edit_number(User *usr, char c) {
	if (usr == NULL)
		return 0;

	switch(c) {
		case EDIT_INIT:
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			erase_name(usr);
			Put(usr, "\n");
			return EDIT_BREAK;

		case KEY_RETURN:
			Put(usr, "\n");
			return EDIT_RETURN;


		case KEY_BS:
			if (usr->edit_pos) {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b \b");
			}
			break;

		case KEY_ESC:
		case KEY_CTRL('X'):
			erase_name(usr);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		default:
			if (c >= '0' && c <= '9') {
				if (usr->edit_pos < MAX_NAME-1) {
					usr->edit_buf[usr->edit_pos++] = c;
					usr->edit_buf[usr->edit_pos] = 0;
					Print(usr, "%c", c);
				}
			}
	}
	return 0;
}

int edit_octal_number(User *usr, char c) {
	if (usr == NULL)
		return 0;

	switch(c) {
		case EDIT_INIT:
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			erase_name(usr);
			Put(usr, "\n");
			return EDIT_BREAK;

		case KEY_RETURN:
			Put(usr, "\n");
			return EDIT_RETURN;


		case KEY_BS:
			if (usr->edit_pos) {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b \b");
			}
			break;

		case KEY_ESC:
		case KEY_CTRL('X'):
			erase_name(usr);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			break;

		default:
			if (c >= '0' && c <= '7') {
				if (usr->edit_pos < MAX_NAME-1) {
					usr->edit_buf[usr->edit_pos++] = c;
					usr->edit_buf[usr->edit_pos] = 0;
					Print(usr, "%c", c);
				}
			}
	}
	return 0;
}

void edit_color(User *usr, char c) {
char color = 0;

	switch(c) {
		case 'k':
		case 'K':
		case KEY_CTRL('K'):
			color = (char)color_by_name("black");
			break;

		case 'r':
		case 'R':
		case KEY_CTRL('R'):
			color = (char)color_by_name("red");
			break;

		case 'g':
		case 'G':
		case KEY_CTRL('G'):
			color = (char)color_by_name("green");
			break;

		case 'y':
		case 'Y':
		case KEY_CTRL('Y'):
			color = (char)color_by_name("yellow");
			break;

		case 'b':
		case 'B':
		case KEY_CTRL('B'):
			color = (char)color_by_name("blue");
			break;

		case 'm':
		case 'M':
		case KEY_CTRL('M'):
			color = (char)color_by_name("magenta");
			break;

		case 'c':
		case 'C':
		case KEY_CTRL('C'):
			color = (char)color_by_name("cyan");
			break;

		case 'w':
		case 'W':
		case KEY_CTRL('W'):
			color = (char)color_by_name("white");
			break;
	}
	if (color && usr->edit_pos < MAX_LINE-1) {
		if (usr->edit_pos && (usr->edit_buf[usr->edit_pos-1] < ' '
			|| usr->edit_buf[usr->edit_pos-1] > '~'))
			usr->edit_pos--;			/* overwrite multiple colors */

		usr->edit_buf[usr->edit_pos++] = color;
		usr->edit_buf[usr->edit_pos] = 0;
		Print(usr, "%c", color);
	}
	usr->runtime_flags &= ~RTF_COLOR_EDITING;
}


/*
	erase word
	contributed by Shannon Prickett <spameater@metanav.org>
*/
void erase_word(User *usr) {
int where;
char lastcolor = 0;

	if (usr->edit_pos <= 0)
		return;

/* Eat until there's a character left of us */
	for (where = usr->edit_pos; where > 0; where--) {
		if (!isspace((int)usr->edit_buf[(where-1)] & 0xff))
			break;

		usr->edit_pos--;
		if (isprint((int)usr->edit_buf[usr->edit_pos] & 0xff))
			Put(usr, "\b \b");

		usr->edit_buf[usr->edit_pos] = 0;
	}

/* Then eat until we're in the space to the right of a char */
	for (where = usr->edit_pos; where > 0; where--) {
		if (isspace((int)usr->edit_buf[(where-1)] & 0xff))
			break;

		usr->edit_pos--;
		if (isprint((int)usr->edit_buf[usr->edit_pos] & 0xff))
			Put(usr, "\b \b");

		usr->edit_buf[usr->edit_pos] = 0;
	}

/* Restore our last color code */
	for (where=usr->edit_pos; where > 0; where--) {
		if (iscntrl((int)usr->edit_buf[(where-1)] & 0xff)) {
			lastcolor = usr->edit_buf[(where-1)];
			break;
		}
	}
	if (lastcolor)
		Print(usr, "%c", lastcolor);
	else
		Put(usr, "<yellow>");
}

/*
	erase a color-code marked line
*/
void erase_line(User *usr, char *line) {
char buf[MAX_LINE*3];
char *p;

	if (usr == NULL || line == NULL)
		return;

	buf[0] = 0;
	for(p = line; *p != 0; p++)
		if (*p >= ' ' && *p <= '~')
			strcat(buf, "\b \b");
	Put(usr, buf);
}

/*
	Note: erase_name() and erase_many() only erase onscreen
*/
void erase_name(User *usr) {
char buf[MAX_LINE*3];
int i;

	if (usr == NULL)
		return;

	buf[0] = 0;
	for(i = usr->edit_pos; i > 0; i--)
		strcat(buf, "\b \b");
	Put(usr, buf);
}

void erase_many(User *usr) {
	if (usr == NULL)
		return;

	if (usr->recipients != NULL) {
		if (usr->recipients->next == NULL) {
			char buf[MAX_LINE*3];
			int i;

			buf[0] = 0;
			i = strlen(usr->recipients->str) + 5;	/* erase '[User Joe]: ' */
			while(i > 0) {
				strcat(buf, "\b \b");
				i--;
			}
			Print(usr, "%s<yellow>%c ", buf, (usr->runtime_flags & RTF_MULTI) ? ',' : ':');
		} else						/* erase ' [<many>]: ' */
			Print(usr, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b<yellow>%c ",
				(usr->runtime_flags & RTF_MULTI) ? ',' : ':');
	}
}

/*
	Note: buf must be large enough (120 bytes will do)
*/
char *print_many(User *usr, char *buf) {
	if (buf == NULL)
		return NULL;

	buf[0] = 0;
	if (usr == NULL)
		return buf;

	if (usr->recipients != NULL) {
		if (usr->recipients->next == NULL)
			sprintf(buf, " <white>[<yellow>%s<white>]%c <yellow>%s", usr->recipients->str,
				(usr->runtime_flags & RTF_MULTI) ? ',' : ':', usr->edit_buf);
		else
			sprintf(buf, " <white>[<green><many><white>]%c <yellow>%s", (usr->runtime_flags & RTF_MULTI) ? ',' : ':',
				usr->edit_buf);
	} else {
		usr->runtime_flags &= ~RTF_MULTI;
		sprintf(buf, "<yellow>: %s", usr->edit_buf);
	}
	return buf;
}


void make_users_tablist(User *usr) {
User *u;

	if (usr == NULL)
		return;

	for(u = AllUsers; u != NULL; u = u->next) {
		if (!u->name[0] || in_StringList(usr->enemies, u->name) != NULL)
			continue;

		if (!usr->edit_pos || !strncmp(u->name, usr->edit_buf, usr->edit_pos))
			add_StringList(&usr->tablist, new_StringList(u->name));
	}
/* now link end to beginning and beginning to end, forming a cyclic chain */
	if (usr->tablist != NULL) {
		StringList *sl;

		for(sl = usr->tablist; sl->next != NULL; sl = sl->next);
		sl->next = usr->tablist;
		usr->tablist->prev = sl;
	}
}

void make_rooms_tablist(User *usr) {
Room *r, *r_next;
Joined *j;

	if (usr == NULL)
		return;

	Enter(make_rooms_tablist);

	listdestroy_StringList(usr->tablist);
	usr->tablist = NULL;

	for(r = AllRooms; r != NULL; r = r_next) {
		r_next = r->next;

		if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
			if ((r = find_Roombynumber(usr, r->number)) == NULL)
				continue;

		if ((r->flags & ROOM_CHATROOM) && !PARAM_HAVE_CHATROOMS)
			continue;

		j = in_Joined(usr->rooms, r->number);

/* if not welcome, continue */
		if (!(in_StringList(r->kicked, usr->name) != NULL
			|| ((r->flags & ROOM_INVITE_ONLY) && in_StringList(r->invited, usr->name) == NULL)
			|| ((r->flags & ROOM_HIDDEN) && j == NULL)
			|| ((r->flags & ROOM_HIDDEN) && j != NULL && r->generation != j->generation))) {

			if (!usr->edit_pos || !strncmp(r->name, usr->edit_buf, usr->edit_pos) || cstristr(r->name, usr->edit_buf) != NULL)
				add_StringList(&usr->tablist, new_StringList(r->name));
		}
		if (r->number == HOME_ROOM)
			unload_Room(r);
	}
/* now link end to beginning and beginning to end, forming a cyclic chain */
	if (usr->tablist != NULL) {
		StringList *sl;

		for(sl = usr->tablist; sl->next != NULL; sl = sl->next);
		sl->next = usr->tablist;
		usr->tablist->prev = sl;
	}
	Return;
}

void erase_tabname(User *usr) {
char buf[MAX_LINE*3];
int i;

	if (usr == NULL)
		return;

	buf[0] = 0;
	for(i = usr->edit_pos; i > 0; i--)
		strcat(buf, "\b \b");
	Put(usr, buf);
}


void tab_list(User *usr, void (*make_tablist)(User *)) {
	if (usr == NULL)
		return;

	Enter(tab_list);

	if (usr->tablist == NULL)
		make_tablist(usr);
	else
		usr->tablist = usr->tablist->next;

	if (usr->tablist != NULL && usr->tablist->str != NULL) {
		erase_tabname(usr);
		strcpy(usr->edit_buf, usr->tablist->str);
		usr->edit_pos = strlen(usr->tablist->str);
		Put(usr, usr->tablist->str);
	}
	Return;
}

void backtab_list(User *usr, void (*make_tablist)(User *)) {
	if (usr == NULL)
		return;

	if (usr->tablist == NULL)
		make_tablist(usr);
	else
		usr->tablist = usr->tablist->prev;

	if (usr->tablist != NULL && usr->tablist->str != NULL) {
		erase_tabname(usr);
		strcpy(usr->edit_buf, usr->tablist->str);
		usr->edit_pos = strlen(usr->tablist->str);
		Put(usr, usr->tablist->str);
	}
}

void reset_tablist(User *usr, char c) {
	if (usr == NULL)
		return;

	if (c != KEY_TAB && c != KEY_BACKTAB && usr->tablist != NULL) {
		if (usr->tablist->prev != NULL) {		/* break the cyclic chain */
			usr->tablist->prev->next = NULL;
			usr->tablist->prev = NULL;
		}
		listdestroy_StringList(usr->tablist);
		usr->tablist = NULL;
	}
}

int edit_data_cmd(User *usr, char c) {
	if (usr == NULL)
		return 0;

	if (c == EDIT_INIT) {
		usr->runtime_flags |= RTF_BUSY;
		usr->edit_pos = 0;
		usr->edit_buf[0] = 0;
		return 0;
	}
	switch(c) {
		case KEY_RETURN:
			return EDIT_RETURN;

		default:
			if (c < ' ' || c > '~')
				break;

			if (usr->edit_pos < MAX_LINE-1) {
				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
			}
	}
	return 0;
}

/* EOB */
