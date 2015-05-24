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
	edit.h	WJ99
*/

#ifndef EDIT_H_WJ99
#define EDIT_H_WJ99 1

#include "User.h"

#define EDIT_INIT		0
#define EDIT_BREAK		-1
#define EDIT_RETURN		1

#define KEY_CTRL(x)		((x) - 'A' + 1)

#define KEY_ESC			0x1b
#define KEY_RETURN		'\r'
#define KEY_BS			'\b'
#define KEY_BEEP		7
#define KEY_TAB			'\t'
#define KEY_BACKTAB		'\\'

/* tabsize must be a power of 2, preferred values are 4 and 8 */
#define TABSIZE			4

/*
	charset1 breaks the line, charset2 wraps along
*/
#define WRAP_CHARSET1	" ,-+=&^%!?>}])/"
#define WRAP_CHARSET2	"<{[($`#~"

int edit_recipients(User *, char, int (*)(User *));
int edit_name(User *, char);
int edit_tabname(User *, char);
int edit_roomname(User *, char);
int edit_caps_line(User *, char);
int edit_password(User *, char);
void clear_password_buffer(User *);
int edit_line(User *, char);
int edit_chatline(User *, char, char *);
int edit_x(User *, char);
int edit_msg(User *, char);
int edit_number(User *, char);
int edit_octal_number(User *, char);
void edit_color(User *, char);
void edit_long_color(User *);
int edit_tab_color(User *, int (*)(User *, char));
int edit_backtab_color(User *, int (*)(User *, char));
void edit_tab_spaces(User *, int (*)(User *, char));
void erase_word(User *);
void erase_line(User *, char *);
void erase_name(User *);
void erase_many(User *);
char *print_many(User *, char *, int);
void erase_tabname(User *);
void make_users_tablist(User *);
void make_rooms_tablist(User *);
void tab_list(User *, void (*)(User *));
void backtab_list(User *, void (*)(User *));
void reset_tablist(User *, char);

int empty_emote(char *);
int empty_xmsg(StringIO *);
int empty_message(Message *);

extern char *Wrap_Charset1;
extern char *Wrap_Charset2;

#endif	/* EDIT_H_WJ99 */

/* EOB */
