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
#include "util.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

char *Wrap_Charset1 = WRAP_CHARSET1;
char *Wrap_Charset2 = WRAP_CHARSET2;

static void edit_putchar(User *, char);
static int wrap_chatline(User *, char, char *);

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
char many_buf[MAX_LINE];

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
				if (count_Queue(usr->recipients) > 0) {
					erase_many(usr);

					sl = (StringList *)usr->recipients->head;
					cstrcpy(usr->edit_buf, sl->str, MAX_LINE);
					usr->edit_pos = strlen(usr->edit_buf);

					(void)remove_StringQueue(usr->recipients, sl);
					destroy_StringList(sl);
					if (count_Queue(usr->recipients) <= 0)
						usr->runtime_flags &= ~RTF_MULTI;

					Print(usr, "\b \b\b \b%s", print_many(usr, many_buf, MAX_LINE));
				}
			}
			break;

		case KEY_CTRL('L'):
			if (usr->edit_pos > 0)
				Put(usr, "\n");

			if (count_Queue(usr->recipients) <= 0)
				Put(usr, "<magenta>The recipient list is empty");
			else {
				Put(usr, "<magenta>List recipients: ");
				for(sl = (StringList *)usr->recipients->tail; sl != NULL; sl = sl->next) {
					Print(usr, "<yellow>%s", sl->str);
					if (sl->next != NULL)
						Put(usr, "<green>, ");
				}
			}
			Print(usr, "\n<green>Enter recipient%s", print_many(usr, many_buf, MAX_LINE));
			break;

		case KEY_CTRL('F'):
			if (usr->friends == NULL)
				break;

			erase_name(usr);
			erase_many(usr);
			Put(usr, "\b\b");

			if (!(usr->runtime_flags & RTF_MULTI)) {
				deinit_StringQueue(usr->recipients);
				usr->runtime_flags |= RTF_MULTI;
			}
			for(sl = usr->friends; sl != NULL; sl = sl->next)
				if (in_StringQueue(usr->recipients, sl->str) == NULL) {
/* this is a kind of hack; you may multi-mail to offline friends */
					if (access_func != multi_mail_access && is_online(sl->str) == NULL)
						continue;

					(void)add_StringQueue(usr->recipients, new_StringList(sl->str));
			}
			Put(usr, print_many(usr, many_buf, MAX_LINE));
			break;

		case KEY_CTRL('T'):						/* Multi to talked-to list by Richard */
			if (PARAM_HAVE_TALKEDTO) {
				StringList *talked_to;

				if ((talked_to = make_talked_to(usr)) == NULL)
					break;

				erase_name(usr);
				erase_many(usr);
				Put(usr, "\b\b");

				if (!(usr->runtime_flags & RTF_MULTI)) {
					deinit_StringQueue(usr->recipients);
					usr->runtime_flags |= RTF_MULTI;
				}
				for(sl = talked_to; sl != NULL; sl = sl->next)
					if (in_StringQueue(usr->recipients, sl->str) == NULL) {
/* this is a kind of hack; you may multi-mail to ppl you've talked with */
						if (access_func != multi_mail_access && is_online(sl->str) == NULL)
							continue;

						(void)add_StringQueue(usr->recipients, new_StringList(sl->str));
				}
				listdestroy_StringList(talked_to);
				Put(usr, print_many(usr, many_buf, MAX_LINE));
			}
			break;

		case KEY_CTRL('W'):
			if (usr->runtime_flags & RTF_SYSOP) {
				User *u;

				erase_name(usr);
				erase_many(usr);
				Put(usr, "\b\b");

				if (!(usr->runtime_flags & RTF_MULTI)) {
					deinit_StringQueue(usr->recipients);
					usr->runtime_flags |= RTF_MULTI;
				}
				for(u = AllUsers; u != NULL; u = u->next) {
					if (u->name[0])
						(void)add_StringQueue(usr->recipients, new_StringList(u->name));
				}
				Put(usr, print_many(usr, many_buf, MAX_LINE));
			}
			break;

		case KEY_CTRL('A'):
			if (usr->flags & USR_FOLLOWUP) {
				usr->flags &= ~USR_FOLLOWUP;
				Put(usr, "\n<magenta>Follow up mode aborted\n");
			}
			c = KEY_CTRL('C');

		case '-':
		case KEY_CTRL('R'):				/* remove from list */
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('U'):
		case KEY_CTRL('X'):
		case KEY_ESC:
			erase_name(usr);
			switch(c) {
				case '-':
				case KEY_CTRL('R'):
					if ((sl = in_StringQueue(usr->recipients, usr->edit_buf)) != NULL) {
						usr->edit_pos = 0;
						usr->edit_buf[0] = 0;

						erase_many(usr);
						(void)remove_StringQueue(usr->recipients, sl);
						destroy_StringList(sl);

						Print(usr, "\b \b\b \b%s", print_many(usr, many_buf, MAX_LINE));
					}
					usr->edit_pos = 0;
					usr->edit_buf[0] = 0;
					break;

				case KEY_CTRL('C'):
				case KEY_CTRL('D'):
					usr->edit_pos = 0;
					usr->edit_buf[0] = 0;
					deinit_StringQueue(usr->recipients);
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
			if (!(usr->runtime_flags & RTF_MULTI))
				deinit_StringQueue(usr->recipients);
/*
	if it's already in the list, then erase it
*/
			if (in_StringQueue(usr->recipients, usr->edit_buf) != NULL) {
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
				if (!(usr->runtime_flags & RTF_MULTI))
					deinit_StringQueue(usr->recipients);

				(void)add_StringQueue(usr->recipients, sl);
			}
			usr->edit_buf[0] = 0;
			usr->edit_pos = 0;
			Put(usr, "\n");
			return EDIT_RETURN;

		case '+':
		case ',':
			if (usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == ' ') {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, "\b");
			}
			if (usr->edit_pos > 0 && !(usr->runtime_flags & RTF_MULTI)) {
				erase_many(usr);
				deinit_StringQueue(usr->recipients);
			}
			if (!usr->edit_pos) {
				if (count_Queue(usr->recipients) > 0) {
					Put(usr, "\b\b, ");
					usr->runtime_flags |= RTF_MULTI;
				}
				break;
			}
			if (in_StringQueue(usr->recipients, usr->edit_buf) != NULL) {
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

				if (count_Queue(usr->recipients) <= 0) {
					(void)add_StringQueue(usr->recipients, sl);
					Print(usr, "\b \b\b \b%s", print_many(usr, many_buf, MAX_LINE));
				} else {
					if (count_Queue(usr->recipients) == 1) {
						erase_many(usr);
						(void)add_StringQueue(usr->recipients, sl);
						Print(usr, "\b \b\b \b%s", print_many(usr, many_buf, MAX_LINE));
					} else
						(void)add_StringQueue(usr->recipients, sl);
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
					cstrcpy(usr->edit_buf, usr->quick[i], MAX_LINE);
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
				Put(usr, usr->edit_buf + usr->edit_pos - 1);
			}
	}
	return 0;
}

int edit_name(User *usr, char c) {
char many_buf[MAX_LINE];

	if (usr == NULL)
		return 0;

	switch(c) {
		case EDIT_INIT:
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;

			deinit_StringQueue(usr->recipients);
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
				if (count_Queue(usr->recipients) > 0) {
					StringList *sl;

					erase_many(usr);

					sl = (StringList *)usr->recipients->head;
					cstrcpy(usr->edit_buf, sl->str, MAX_LINE);
					usr->edit_pos = strlen(usr->edit_buf);

					(void)remove_StringQueue(usr->recipients, sl);
					destroy_StringList(sl);
					if (count_Queue(usr->recipients) <= 0)
						usr->runtime_flags &= ~RTF_MULTI;

					Print(usr, "\b \b\b \b%s", print_many(usr, many_buf, MAX_LINE));
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
				Put(usr, usr->edit_buf + usr->edit_pos - 1);
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
				if (count_Queue(usr->recipients) > 0) {
					cstrcpy(usr->edit_buf, ((StringList *)usr->recipients->tail)->str, MAX_LINE);
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
					cstrcpy(usr->edit_buf, usr->quick[i], MAX_LINE);
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
				Put(usr, usr->edit_buf + usr->edit_pos - 1);
			}
	}
	Return 0;
}

int edit_caps_line(User *usr, char c) {
	if (usr == NULL)
		return 0;

	Enter(edit_caps_line);

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
			Return EDIT_BREAK;

		case KEY_RETURN:
			if (usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == ' ') {
				usr->edit_pos--;
				usr->edit_buf[usr->edit_pos] = 0;
			}
			Put(usr, "\n");
			Return EDIT_RETURN;

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
			break;

		case ' ':
			if (!usr->edit_pos)
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
			if (c < ' ' || c > '~')
				break;

			if (usr->edit_pos < MAX_LINE-1) {
				if ((!usr->edit_pos || usr->edit_buf[usr->edit_pos-1] == ' ')
					&& c >= 'a' && c <= 'z')
					c -= ' ';						/* auto uppercase */

				edit_putchar(usr, c);
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
				int flags;

				usr->edit_buf[usr->edit_pos++] = c;
				usr->edit_buf[usr->edit_pos] = 0;
/*
	the juggling with the flags is to disable the auto-coloring habit here
	it's not pretty, but the result looks better this way
*/
				flags = usr->flags;
				usr->flags |= USR_DONT_AUTO_COLOR;
				Put(usr, "*");
				usr->flags = flags;
			}
	}
	return 0;
}

void clear_password_buffer(User *usr) {
	memset(usr->edit_buf, 0, MAX_LINE);
	usr->edit_pos = 0;
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

		case '^':
		case KEY_CTRL('V'):
			usr->runtime_flags |= RTF_COLOR_EDITING;
			break;

		case KEY_TAB:
			if (edit_tab_color(usr, edit_line))
				break;

			edit_tab_spaces(usr, edit_line);
			break;

		case '>':
			edit_long_color(usr);
			break;

		case KEY_BACKTAB:
			if (edit_backtab_color(usr, edit_line))
				break;

/* fall through */
		default:
			if (c < ' ' || c > '~')
				break;

			if (usr->edit_pos < MAX_LINE-1)
				edit_putchar(usr, c);
	}
	return 0;
}

/*
	same as edit_line(), but it handles KEY_RETURN and word-wrapping
	in a different way
	the third argument to this function is a buffer to store word-wrapped text in
*/
int edit_chatline(User *usr, char c, char *wrapped) {
	if (usr == NULL)
		return 0;

	if (c == EDIT_INIT) {
		usr->runtime_flags |= RTF_BUSY;
		usr->runtime_flags &= ~RTF_COLOR_EDITING;

		if (wrapped == NULL) {
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
		} else {
			cstrcpy(usr->edit_buf, wrapped, MAX_LINE);
			usr->edit_pos = strlen(usr->edit_buf);
			*wrapped = 0;
		}
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
/*			Put(usr, "\n");			*/
			ctrim_line(usr->edit_buf);
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

		case '^':
		case KEY_CTRL('V'):
			usr->runtime_flags |= RTF_COLOR_EDITING;
			break;

		case KEY_TAB:
			if (edit_tab_color(usr, edit_line))
				break;

			edit_tab_spaces(usr, edit_line);
			break;

		case '>':
			edit_long_color(usr);
			break;

		case KEY_BACKTAB:
			if (edit_backtab_color(usr, edit_line))
				break;

/* fall through */
		default:
			if (c < ' ' || c > '~')
				break;

			if (wrap_chatline(usr, c, wrapped)) {
				ctrim_line(usr->edit_buf);
				return EDIT_RETURN;
			}
	}
	return 0;
}

/*
	put a character into edit_buf and display it
*/
static void edit_putchar(User *usr, char c) {
char *p;

	Enter(edit_putchar);
/*
	this strange construction is faster than using Print()
	(not that anyone cares, but hey...)
*/
	p = usr->edit_buf + usr->edit_pos;
	*p = c;
	p[1] = 0;

	if (!(usr->flags & USR_DONT_AUTO_COLOR)) {
/*
	this fixes (most) auto-coloring problems when editing
*/
		if (usr->edit_pos > 0 && (usr->edit_buf[usr->edit_pos-1] < ' ' || usr->edit_buf[usr->edit_pos-1] > '~'))
			p = usr->edit_buf + usr->edit_pos - 1;
/*
	hack for annoying wrong auto-coloring of dots; do a color correction
	and do it only the second time a dot is entered
*/
		if (c == '.' && usr->edit_pos > 0 && usr->edit_buf[usr->edit_pos-1] == '.'
			&& !(usr->edit_pos > 1 && usr->edit_buf[usr->edit_pos-2] == '.')) {
			usr->edit_pos++;
			Put(usr, "\b..");
			Return;
		}
	}
	usr->edit_pos++;
	Put(usr, p);
	Return;
}

/*
	word wrapping for edit_x() and edit_msg()

	The word wrap wraps on terminal width, but is also delimited by
	the maximum buffer size MAX_LINE
	(this will be forever the case, until all buffers are dynamically sizeable)
*/
static void edit_wrap(User *usr, char c, char *prompt) {
char erase[MAX_LONGLINE], wrap[MAX_LINE];
int i, wrap_len, n;

	if (prompt == NULL)
		n = 0;
	else
		n = strlen(prompt);

	if (usr->edit_pos < MAX_LINE-1-n && usr->edit_pos < usr->display->term_width-1-n) {
		edit_putchar(usr, c);
		return;
	}

/* word wrap */

	erase[0] = wrap[0] = 0;

	if (c != ' ' && cstrchr(Wrap_Charset2, c) == NULL) {
		wrap_len = (usr->display->term_width-n) / 3;
		if (wrap_len >= MAX_LINE)
			wrap_len = MAX_LINE-1;

		for(i = usr->edit_pos - 1; i > wrap_len; i--) {
			if (cstrchr(Wrap_Charset1, usr->edit_buf[i]) != NULL) {
				i++;
				break;
			}
			if (cstrchr(Wrap_Charset2, usr->edit_buf[i]) != NULL) {
				cstrcat(erase, "\b \b", MAX_LONGLINE);
				break;
			}
			cstrcat(erase, "\b \b", MAX_LONGLINE);
		}
		if (i > wrap_len) {
			cstrcpy(wrap, usr->edit_buf+i, MAX_LINE);
			usr->edit_buf[i] = 0;
			Put(usr, erase);
		}
	}
/*
	it is a nice experiment to leave out the newline, but it gives problems
	because then it's suddenly possible to have really long lines
	especially the load_xxx() and save_xxx() functions don't like it ...
*/
	put_StringIO(usr->text, usr->edit_buf);
	write_StringIO(usr->text, "\n", 1);
	usr->total_lines++;

/* wrap word to next line */
	cstrcpy(usr->edit_buf, wrap, MAX_LINE);
	usr->edit_pos = strlen(usr->edit_buf);

/* don't wrap a space at the beginning of the line */
	if (c != ' ' || usr->edit_pos > 0)
		usr->edit_buf[usr->edit_pos++] = c;

	usr->edit_buf[usr->edit_pos] = 0;

	Print(usr, "\n%s", prompt);
	Put(usr, usr->edit_buf);			/* in 2 separate prints; no lame color bug(!) */
}

/*
	do word-wrapping on chat lines
	the wrapped text is put into char *wrap
	wrap should be at least MAX_LINE long
*/
static int wrap_chatline(User *usr, char c, char *wrap) {
int i, wrap_len;

	if (usr == NULL)
		return 0;

	i = usr->edit_pos + strlen(usr->name) + 2;
	if (i < MAX_LINE && i < usr->display->term_width) {
		edit_putchar(usr, c);
		return 0;
	}
	if (wrap == NULL)
		return 0;

/* word wrap */

	*wrap = 0;
	if (c != ' ' && cstrchr(Wrap_Charset2, c) == NULL) {
		wrap_len = usr->display->term_width / 3;
		if (wrap_len >= MAX_LINE)
			wrap_len = MAX_LINE-1;

		for(i = usr->edit_pos - 1; i > wrap_len; i--) {
			if (cstrchr(Wrap_Charset1, usr->edit_buf[i]) != NULL) {
				i++;
				break;
			}
			if (cstrchr(Wrap_Charset2, usr->edit_buf[i]) != NULL)
				break;
		}
		if (i > wrap_len) {
			if (*usr->edit_buf == ' ') {
				*wrap = ' ';
				cstrcpy(wrap+1, usr->edit_buf+i, MAX_LINE-1);
			} else
				cstrcpy(wrap, usr->edit_buf+i, MAX_LINE);

			usr->edit_buf[i] = 0;
			ctrim_line(usr->edit_buf);
		}
	}
	usr->edit_pos = strlen(wrap);
	if (c != ' ') {
		wrap[usr->edit_pos++] = c;
		wrap[usr->edit_pos] = 0;
	}
	return 1;							/* we wrapped */
}


int edit_x(User *usr, char c) {
int color;
char prompt[4];

	if (usr == NULL)
		return 0;

	if (c == EDIT_INIT) {
		usr->runtime_flags |= RTF_BUSY;
		usr->runtime_flags &= ~RTF_COLOR_EDITING;
		usr->edit_pos = usr->total_lines = 0;
		usr->edit_buf[0] = 0;

		free_StringIO(usr->text);
		return 0;
	}
	if (usr->total_lines >= PARAM_MAX_XMSG_LINES) {
		if (c == KEY_CTRL('C')) {
			wipe_line(usr);
			return EDIT_BREAK;
		}
		if (c == KEY_CTRL('X')) {
			wipe_line(usr);
			return EDIT_RETURN;
		}
		color = usr->color;
		Print(usr, "%c<red>Too many lines, press<yellow> <Ctrl-C><red> to abort,<yellow> <Ctrl-X><red> to send", KEY_CTRL('X'));
		restore_color(usr, color);
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
			if (usr->edit_buf[0]) {
/*
	added by Shannon Prickett <spameater@metanav.org>
	'ABORT' aborts an X message (like it does in DOC)
*/
				if (!strcmp(usr->edit_buf, "ABORT")) {
					Put(usr, "\n");
					return EDIT_BREAK;
				}
				put_StringIO(usr->text, usr->edit_buf);
				write_StringIO(usr->text, "\n", 1);

				usr->total_lines++;
				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;

				if (usr->total_lines < PARAM_MAX_XMSG_LINES) {
					prompt[0] = '\n';
					prompt[1] = KEY_CTRL('Q');
					prompt[2] = '>';
					prompt[3] = 0;
					Put(usr, prompt);
					break;
				}
			}
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

		case KEY_ESC:
		case KEY_CTRL('U'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('X'):
			erase_line(usr, usr->edit_buf);
			usr->edit_pos = 0;
			usr->edit_buf[0] = 0;
			Put(usr, "<yellow>");
			break;

		case '^':
		case KEY_CTRL('V'):
			usr->runtime_flags |= RTF_COLOR_EDITING;
			if (usr->edit_pos >= MAX_LINE-2) {		/* wrap color to next line */
				put_StringIO(usr->text, usr->edit_buf);
				write_StringIO(usr->text, "\n", 1);
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

		case KEY_TAB:
			if (edit_tab_color(usr, edit_x))
				break;

			edit_tab_spaces(usr, edit_x);
			break;

		case '>':
			edit_long_color(usr);
			break;

		default:
			if (c < ' ' || c > '~')
				break;

			prompt[0] = KEY_CTRL('Q');
			prompt[1] = '>';
			prompt[2] = 0;
			edit_wrap(usr, c, prompt);
	}
	return 0;
}

int edit_msg(User *usr, char c) {
int color;

	if (usr == NULL)
		return 0;

	if (c == EDIT_INIT) {
		usr->runtime_flags |= RTF_BUSY;
		usr->runtime_flags &= ~RTF_COLOR_EDITING;
		usr->edit_pos = usr->total_lines = 0;
		usr->edit_buf[0] = 0;

		free_StringIO(usr->text);

		Put(usr, "<yellow>");
		return 0;
	}
	if (usr->total_lines >= PARAM_MAX_MSG_LINES) {
		if (c == KEY_CTRL('C')) {
			wipe_line(usr);
			Put(usr, "\n");
			return EDIT_BREAK;
		}
		color = usr->color;
		Print(usr, "%c<red>Too many lines, press<yellow> <Ctrl-C>", KEY_CTRL('X'));
		restore_color(usr, color);
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
					Put(usr, "<yellow>");
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
			Put(usr, "<yellow>");
			break;
/*
	Eat words in post line, to be more like DOC
	contributed by Shannon Prickett <spameater@metanav.org>
*/
		case KEY_CTRL('W'):
			erase_word(usr);
			break;

		case '^':
		case KEY_CTRL('V'):
			usr->runtime_flags |= RTF_COLOR_EDITING;
			if (usr->edit_pos >= MAX_LINE-2) {		/* wrap color to next line */
				put_StringIO(usr->text, usr->edit_buf);
				write_StringIO(usr->text, "\n", 1);
				usr->total_lines++;
				usr->edit_pos = 0;
				usr->edit_buf[0] = 0;
			}
			break;

		case KEY_TAB:
			if (edit_tab_color(usr, edit_msg))
				break;

			edit_tab_spaces(usr, edit_msg);
			break;

		case '>':
			edit_long_color(usr);
			break;

		case KEY_BACKTAB:
			if (edit_backtab_color(usr, edit_line))
				break;

/* fall through */
		default:
			if (c < ' ' || c > '~')
				break;

			edit_wrap(usr, c, "");
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
			if (c == ' ' && !usr->edit_pos) {
				Put(usr, "\n");
				return EDIT_BREAK;
			}
			if (c >= '0' && c <= '9') {
				if (usr->edit_pos < MAX_NAME-1) {
					usr->edit_buf[usr->edit_pos++] = c;
					usr->edit_buf[usr->edit_pos] = 0;
					Put(usr, usr->edit_buf + usr->edit_pos - 1);
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
			if (c == ' ' && !usr->edit_pos) {
				Put(usr, "\n");
				return EDIT_BREAK;
			}
			if (c >= '0' && c <= '7') {
				if (usr->edit_pos < MAX_NAME-1) {
					usr->edit_buf[usr->edit_pos++] = c;
					usr->edit_buf[usr->edit_pos] = 0;
					Put(usr, usr->edit_buf + usr->edit_pos - 1);
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

		case 'p':				/* purple */
		case 'P':
		case KEY_CTRL('P'):
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

		default:
			color = (c >= ' ' && c <= '~') ? c : 0;
/*
	user entered a different character, put "^char"
	this is rather funny when you used Ctrl-V rather than caret, or not ...
*/
			if (usr->edit_pos < MAX_LINE-1 && c != '^') {
				usr->edit_buf[usr->edit_pos++] = '^';
				usr->edit_buf[usr->edit_pos] = 0;
				Put(usr, usr->edit_buf + usr->edit_pos - 1);
			}
	}
	if (color && usr->edit_pos < MAX_LINE-1) {
/* overwrite multiple colors */
		if (usr->edit_pos > 0 && (color < ' ' || color > '~')
			&& (usr->edit_buf[usr->edit_pos-1] < ' ' || usr->edit_buf[usr->edit_pos-1] > '~'))
			usr->edit_pos--;

		usr->edit_buf[usr->edit_pos++] = color;
		usr->edit_buf[usr->edit_pos] = 0;
		Put(usr, usr->edit_buf + usr->edit_pos - 1);
	}
	usr->runtime_flags &= ~RTF_COLOR_EDITING;
}

void edit_long_color(User *usr) {
int i, l;
char colorbuf[MAX_COLORBUF];

	if (usr == NULL)
		return;

	for(i = 0; i < NUM_COLORS; i++) {
		if (i == HOTKEY)
			continue;

		l = bufprintf(colorbuf, sizeof(colorbuf), "<%s", color_table[i].name);
		if (usr->edit_pos < l)
			continue;

		if (!cstrnicmp(colorbuf, usr->edit_buf + usr->edit_pos - l, l)) {
			usr->edit_pos -= l;

/* overwrite multiple colors */
			if (usr->edit_pos > 0 && (usr->edit_buf[usr->edit_pos-1] < ' ' || usr->edit_buf[usr->edit_pos-1] > '~'))
				usr->edit_pos--;

			usr->edit_buf[usr->edit_pos++] = color_table[i].key;
			usr->edit_buf[usr->edit_pos] = 0;

			while(l > 0) {
				Put(usr, "\b \b");
				l--;
			}
			Put(usr, usr->edit_buf + usr->edit_pos - 1);
			return;
		}
	}

/* no match, it wasn't a color code after all */

	usr->edit_buf[usr->edit_pos++] = '>';
	usr->edit_buf[usr->edit_pos] = 0;
	Put(usr, usr->edit_buf + usr->edit_pos - 1);
}

/*
	use the Tab key to expand color names
	Black is skipped because black text doesn't work well on a black background,
	and 99.99% percent of all cases you were typing for 'blue' anyway
*/
int edit_tab_color(User *usr, int (*edit_func)(User *, char)) {
int pos, i, n;
char color[MAX_COLORBUF];

	if (usr == NULL || edit_func == NULL)
		return -1;

	Enter(edit_tab_color);

	pos = usr->edit_pos;
	if (pos <= 0) {
		Return 0;
	} 
	pos--;
	n = 0;
	while(usr->edit_buf[pos] != '<' && n < 8) {
		pos--;
		n++;
	}
	if (usr->edit_buf[pos] != '<') {
		Return 0;
	}
	n = strlen(usr->edit_buf + pos);

	for(i = 1; i < NUM_COLORS; i++) {			/* skip 0, which is black text */
		if (i == HOTKEY)
			continue;

		bufprintf(color, sizeof(color), "<%s", color_table[i].name);
		cstrlwr(color);

		if (!cstrnicmp(usr->edit_buf+pos, color, n)) {
			if (!color[n]) {
				while(n > 1) {
					Put(usr, "\b \b");

					if (usr->edit_pos > 0)
						usr->edit_pos--;
					usr->edit_buf[usr->edit_pos] = 0;
					n--;
				}
				i++;
				if (i >= HOTKEY)
					i = 1;				/* skip 0, which is black */

				bufprintf(color, sizeof(color), "<%s", color_table[i].name);
				cstrlwr(color);
				n = 1;
			}
			pos = n;
			n = strlen(color);
			while(pos < n)
				edit_func(usr, color[pos++]);
			Return 1;
		}
	}
	Return 0;
}

int edit_backtab_color(User *usr, int (*edit_func)(User *, char)) {
int pos, i, n;
char color[MAX_COLORBUF];

	if (usr == NULL || edit_func == NULL)
		return -1;

	Enter(edit_backtab_color);

	pos = usr->edit_pos;
	if (pos <= 0) {
		Return 0;
	} 
	pos--;
	n = 0;
	while(usr->edit_buf[pos] != '<' && n < 8) {
		pos--;
		n++;
	}
	if (usr->edit_buf[pos] != '<') {
		Return 0;
	}
	n = strlen(usr->edit_buf + pos);

	for(i = NUM_COLORS-1; i >= 1; i--) {			/* skip 0, which is black text */
		if (i == HOTKEY)
			continue;

		bufprintf(color, sizeof(color), "<%s", color_table[i].name);
		cstrlwr(color);

		if (!cstrnicmp(usr->edit_buf+pos, color, n)) {
			if (!color[n]) {
				while(n > 1) {
					Put(usr, "\b \b");

					if (usr->edit_pos > 0)
						usr->edit_pos--;
					usr->edit_buf[usr->edit_pos] = 0;
					n--;
				}
				i--;
				if (i <= 0) {						/* skip 0, which is black text */
					i = NUM_COLORS-1;
					if (i == HOTKEY)
						i--;
				}
				bufprintf(color, sizeof(color), "<%s", color_table[i].name);
				cstrlwr(color);
				n = 1;
			}
			pos = n;
			n = strlen(color);
			while(pos < n)
				edit_func(usr, color[pos++]);
			Return 1;
		}
	}
	Return 0;
}

/*
	insert spaces for tabs
*/
void edit_tab_spaces(User *usr, int (*edit_func)(User *, char)) {
int n, i;

	if (usr == NULL || edit_func == NULL)
		return;

	Enter(edit_tab_spaces);

/*
	this loops goes to TABSIZE*2, but breaks earlier than that
	the 'times 2' is only to be sure that we have a full tab even if the line wraps
*/
	for(i = 0; i < TABSIZE*2; i++) {
		edit_func(usr, ' ');

		n = color_strlen(usr->edit_buf) + 1;
		if (!(n & (TABSIZE-1)))
			break;

		n++;
	}
	Return;
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
	for(where = usr->edit_pos; where > 0; where--) {
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
char buf[MAX_LONGLINE];
char *p;

	if (usr == NULL || line == NULL)
		return;

	buf[0] = 0;
	for(p = line; *p != 0; p++)
		if (*p >= ' ' && *p <= '~')
			cstrcat(buf, "\b \b", MAX_LONGLINE);
	Put(usr, buf);
}

/*
	Note: erase_name() and erase_many() only erase onscreen
*/
void erase_name(User *usr) {
char buf[MAX_LONGLINE];
int i;

	if (usr == NULL)
		return;

	buf[0] = 0;
	for(i = usr->edit_pos; i > 0; i--)
		cstrcat(buf, "\b \b", MAX_LONGLINE);
	Put(usr, buf);
}

void erase_many(User *usr) {
	if (usr == NULL)
		return;

	if (count_Queue(usr->recipients) > 0) {
		if (count_Queue(usr->recipients) == 1) {
			char buf[MAX_LONGLINE];
			int i;

			buf[0] = 0;
			i = strlen(((StringList *)usr->recipients->tail)->str) + 5;	/* erase '[User Joe]: ' */
			while(i > 0) {
				cstrcat(buf, "\b \b", MAX_LONGLINE);
				i--;
			}
			Print(usr, "%s<yellow>%c ", buf, (usr->runtime_flags & RTF_MULTI) ? ',' : ':');
		} else						/* erase ' [<many>]: ' */
			Print(usr, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b<yellow>%c ",
				(usr->runtime_flags & RTF_MULTI) ? ',' : ':');
	}
}

/*
	Note: buf must be large enough (MAX_LINE should do)
*/
char *print_many(User *usr, char *buf, int buflen) {
	if (buf == NULL)
		return NULL;

	buf[0] = 0;
	if (usr == NULL)
		return buf;

	if (count_Queue(usr->recipients) > 0) {
		if (count_Queue(usr->recipients) == 1)
			bufprintf(buf, buflen, " <white>[<yellow>%s<white>]%c <yellow>%s", ((StringList *)usr->recipients->tail)->str,
				(usr->runtime_flags & RTF_MULTI) ? ',' : ':', usr->edit_buf);
		else
			bufprintf(buf, buflen, "<green> <white>[<green><many<green>><white>]%c <yellow>%s", (usr->runtime_flags & RTF_MULTI) ? ',' : ':',
				usr->edit_buf);
	} else {
		usr->runtime_flags &= ~RTF_MULTI;
		bufprintf(buf, buflen, ": <yellow>%s", usr->edit_buf);
	}
	return buf;
}


void make_users_tablist(User *usr) {
User *u;

	if (usr == NULL)
		return;

	deinit_StringQueue(usr->tablist);

	for(u = AllUsers; u != NULL; u = u->next) {
		if (!u->name[0] || (!(usr->flags & USR_SHOW_ENEMIES) && in_StringList(usr->enemies, u->name) != NULL))
			continue;

		if (!usr->edit_pos || !strncmp(u->name, usr->edit_buf, usr->edit_pos))
			(void)add_StringQueue(usr->tablist, new_StringList(u->name));
	}
/* now link end to beginning and beginning to end, forming a cyclic chain */
	if (count_Queue(usr->tablist) > 0) {
		usr->tablist->head->next = usr->tablist->tail;
		usr->tablist->tail->prev = usr->tablist->head;
	}
}

void make_rooms_tablist(User *usr) {
Room *r, *r_next;
Joined *j;
StringList *zapped = NULL, *very_similar = NULL, *similar = NULL;
char room_name[MAX_LINE+2], match[MAX_LINE+2];

	if (usr == NULL)
		return;

	Enter(make_rooms_tablist);

	deinit_StringQueue(usr->tablist);

	bufprintf(match, sizeof(match), " %s ", usr->edit_buf);

	for(r = AllRooms; r != NULL; r = r_next) {
		r_next = r->next;

		if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
			if ((r = find_Roombynumber(usr, r->number)) == NULL)
				continue;

		if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
			continue;

		if (!usr->edit_pos || !strncmp(r->name, usr->edit_buf, usr->edit_pos)) {
			j = in_Joined(usr->rooms, r->number);

			if (!joined_visible(usr, r, j))
				continue;

			if (j != NULL && j->zapped)
				zapped = add_StringList(&zapped, new_StringList(r->name));
			else
				(void)add_StringQueue(usr->tablist, new_StringList(r->name));
		} else {
			bufprintf(room_name, sizeof(room_name), " %s ", r->name);
			if (cstristr(room_name, match) != NULL) {
				if (!room_visible(usr, r))
					continue;

				very_similar = add_StringList(&very_similar, new_StringList(r->name));
				continue;
			}
			if (cstristr(r->name, usr->edit_buf) != NULL) {
				if (!room_visible(usr, r))
					continue;

				similar = add_StringList(&similar, new_StringList(r->name));
			}
		}
		if (r->number == HOME_ROOM)
			unload_Room(r);
	}
	zapped = rewind_StringList(zapped);
	(void)concat_StringQueue(usr->tablist, zapped);
	very_similar = rewind_StringList(very_similar);
	(void)concat_StringQueue(usr->tablist, very_similar);
	similar = rewind_StringList(similar);
	(void)concat_StringQueue(usr->tablist, similar);

/* now link end to beginning and beginning to end, forming a cyclic chain */
	if (count_Queue(usr->tablist) > 0) {
		usr->tablist->head->next = usr->tablist->tail;
		usr->tablist->tail->prev = usr->tablist->head;
	}
	Return;
}

void erase_tabname(User *usr) {
char buf[MAX_LONGLINE];
int i;

	if (usr == NULL)
		return;

	buf[0] = 0;
	for(i = usr->edit_pos; i > 0; i--)
		cstrcat(buf, "\b \b", MAX_LONGLINE);
	Put(usr, buf);
}

void tab_list(User *usr, void (*make_tablist)(User *)) {
	if (usr == NULL)
		return;

	Enter(tab_list);

	if (count_Queue(usr->tablist) <= 0)
		make_tablist(usr);
	else
		usr->tablist->tail = usr->tablist->tail->next;

	if (usr->tablist->tail != NULL) {
		erase_tabname(usr);
		cstrcpy(usr->edit_buf, ((StringList *)usr->tablist->tail)->str, MAX_LINE);
		usr->edit_pos = strlen(((StringList *)usr->tablist->tail)->str);
		Put(usr, ((StringList *)usr->tablist->tail)->str);
	}
	Return;
}

void backtab_list(User *usr, void (*make_tablist)(User *)) {
	if (usr == NULL)
		return;

	if (count_Queue(usr->tablist) <= 0)
		make_tablist(usr);
	else
		usr->tablist->tail = usr->tablist->tail->prev;

	if (usr->tablist->head != NULL) {
		erase_tabname(usr);
		cstrcpy(usr->edit_buf, ((StringList *)usr->tablist->tail)->str, MAX_LINE);
		usr->edit_pos = strlen(((StringList *)usr->tablist->tail)->str);
		Put(usr, ((StringList *)usr->tablist->tail)->str);
	}
}

void reset_tablist(User *usr, char c) {
	if (usr == NULL)
		return;

	if (c != KEY_TAB && c != KEY_BACKTAB && usr->tablist->tail != NULL) {
		usr->tablist->tail = usr->tablist->head->next;	/* break the cyclic chain */
		usr->tablist->tail->prev = NULL;
		usr->tablist->head->next = NULL;
		deinit_StringQueue(usr->tablist);
	}
}

int empty_emote(char *buf) {
char *p;

	for(p = buf; p != NULL && *p; p++) {
		if (*p <= ' ' || *p == '\n')
			continue;

		return 0;
	}
	return 1;
}

int empty_xmsg(StringIO *s) {
char *p;

	if (s == NULL || s->buf == NULL)
		return 1;

	for(p = s->buf; p != NULL && *p; p++) {
		if (*p <= ' ' || *p == '\n')
			continue;

		return 0;
	}
	return 1;
}

int empty_message(Message *msg) {
char *p;

	if (msg == NULL || msg->msg == NULL)
		return 1;

	for(p = msg->msg->buf; p != NULL && *p; p++) {
		if (*p <= ' ' || *p == '\n')
			continue;

		return 0;
	}
	return 1;
}

/* EOB */
