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
	Chatter18	WJ97
	user.h
*/

#ifndef USER_H_WJ97
#define USER_H_WJ97 1

#include "defines.h"
#include "PList.h"
#include "StringList.h"
#include "Joined.h"
#include "Room.h"
#include "Message.h"
#include "BufferedMsg.h"
#include "Timer.h"
#include "Timezone.h"
#include "Conn.h"
#include "Telnet.h"
#include "StringIO.h"
#include "Display.h"
#include "sys_time.h"

#define add_User(x,y)			(User *)add_List((x), (y))
#define concat_User(x,y)		(User *)concat_List((x), (y))
#define remove_User(x,y)		(User *)remove_List((x), (y))
#define rewind_User(x)			(User *)rewind_List((x))
#define unwind_User(x)			(User *)unwind_List((x))
#define listdestroy_User(x)		listdestroy_List((x), destroy_User)

#define notify_login(x)			notify_friends((x), PARAM_NOTIFY_LOGIN, 1)
#define notify_logout(x)		notify_friends((x), PARAM_NOTIFY_LOGOUT, 1)
#define notify_linkdead(x)		notify_friends((x), PARAM_NOTIFY_LINKDEAD, 1)
#define notify_idle(x)			notify_friends((x), PARAM_NOTIFY_IDLE, 0)
#define notify_locked(x)		notify_friends((x), PARAM_NOTIFY_LOCKED, 0)
#define notify_unlocked(x)		notify_friends((x), PARAM_NOTIFY_UNLOCKED, 0)
#define notify_hold(x)			notify_friends((x), PARAM_NOTIFY_HOLD, 0)
#define notify_unhold(x)		notify_friends((x), PARAM_NOTIFY_UNHOLD, 0)

/* usr->flags */
#define USR_ANSI				1			/* ANSI terminal */
#define USR_BOLD				2			/* bright/bold enabled */
#define USR_BEEP				4			/* beeping enabled */
#define USR_X_DISABLED			8			/* Xs disabled */
#define USR_FOLLOWUP			0x10		/* followup mode enabled */
#define USR_SHORT_WHO			0x20		/* default who list is short */
#define USR_HACKERZ				0x40		/* hackerz mode enabled */
#define USR_ROOMNUMBERS			0x80		/* show room number in prompt */
#define USR_HELPING_HAND		0x100		/* '%' status enabled */
#define USR_SORT_BYNAME			0x200		/* default who list is sorted by name */
#define USR_SORT_DESCENDING		0x400		/* default who list is sorted descending */
#define USR_SHOW_ALL			0x800		/* show all users when inside a chat room */
#define USR_SHOW_ENEMIES		0x1000		/* show enemies in who list */
#define USR_12HRCLOCK			0x2000		/* show time in 12 hr clock format with AM/PM */
#define USR_HOLD_BUSY			0x4000		/* hold message mode when busy */
#define USR_ROOMBEEP			0x8000		/* rooms beep when new postings are made */
#define USR_HIDE_ADDRESS		0x10000		/* hide address info from non-friends */
#define USR_HIDE_INFO			0x20000		/* hide profile info from enemies */
#define USR_FORCE_TERM			0x40000		/* force terminal width and height */
#define USR_UPPERCASE_HOTKEYS	0x80000		/* hotkeys always displayed in uppercase */
#define USR_DENY_MULTI			0x100000	/* don't receive multi X's */
#define USR_FRIEND_NOTIFY		0x200000	/* display many friend notifications */
#define USR_DONT_AUTO_COLOR		0x400000	/* don't do automatic coloring of symbols */
#define USR_HOTKEY_BRACKETS		0x800000	/* always display brackets around hotkeys */
#define USR_DONT_ASK_REASON		0x1000000	/* don't ask for an Away reason, just go */
#define USR_BOLD_HOTKEYS		0x2000000	/* print hotkeys bold; Warning: complex meaning for this flag (see util.c, function print_hotkey()) */
#define USR_XMSG_NUM			0x4000000	/* put sequence numbers in received messages */
#define USR_SHORT_PROFILE		0x8000000	/* default profile info listing is short */
#define USR_SHORT_DL_COLORS		0x10000000	/* show short color codes in downloaded text */
#define USR_NOPAGE_DOWNLOADS	0x20000000	/* don't show pager for downloaded text */
#define USR_BLOCK_FRIENDS		0x40000000	/* when X disabled, also block friends */
#define USR_CYCLE_ROOMS			0x80000000	/* cycle through rooms when there are no new messages */
#define USR_ALL					0xffffffff	/* USR_ANSI | USR_BOLD | ... | USR_xxx */

/* usr->flags2 */
#define USR2_ENTER_UPLOAD		1			/* enter uploads by default */
#define USR2_ALL				1

/* runtime flags (not saved in userfile) */
#define RTF_BUSY				1			/* user is currently busy */
#define RTF_LOCKED				2			/* user is locked */
#define RTF_ROOMAIDE			4			/* user is in room aide mode */
#define RTF_SYSOP				8			/* user is in super user mode */
#define RTF_UPLOAD				0x10		/* upload message */
#define RTF_MULTI				0x20		/* user is entering multiple recipients */
#define RTF_COLOR_EDITING		0x40		/* edit color code with Ctrl-V */
#define RTF_NUMERIC_ROOMNAME	0x80		/* edit room number instead of room name */
#define RTF_CONFIG_EDITED		0x100		/* user config changed */
#define RTF_ROOM_EDITED			0x200		/* room changed in room config menu */
#define RTF_PARAM_EDITED		0x400		/* param changed in sysop menu */
#define RTF_WRAPPER_EDITED		0x800		/* wrapper changed in sysop menu */
#define RTF_ANON				0x1000		/* posting as anonymous */
#define RTF_DEFAULT_ANON		0x2000		/* posting as default-anonymous */
#define RTF_BUSY_SENDING		0x4000		/* busy sending a message */
#define RTF_BUSY_MAILING		0x8000		/* busy mailing a message */
#define RTF_CHAT_ESCAPE			0x10000		/* user pressed '/' in a chat room */
#define RTF_HOLD				0x20000		/* user has messages put on hold */
#define RTF_WAS_HH				0x40000		/* X-disabled, user was Helping Hand */
#define RTF_CATEGORY_EDITED		0x80000		/* categories changed in sysop menu */
#define RTF_ROOM_RESIZED		0x100000	/* max amount of messages changed in roomconfig menu */
#define RTF_BUFFER_TEXT			0x200000	/* output is buffered to usr->text */

/* load_User() flags (for faster loading) */
#define LOAD_USER_ADDRESS		1
#define LOAD_USER_DATA			2
#define LOAD_USER_ROOMS			4
#define LOAD_USER_QUICKLIST		8
#define LOAD_USER_FRIENDLIST	0x10
#define LOAD_USER_ENEMYLIST		0x20
#define LOAD_USER_INFO			0x40
#define LOAD_USER_PASSWORD		0x80
#define LOAD_USER_ALL			0xff

/* temp buffers */
#define TMP_NAME				0
#define TMP_PASSWD				1
#define TMP_FROM_HOST			2
#define TMP_FROM_IP				3
#define NUM_TMP					4

#define NUM_QUICK				10			/* number of quicklist entries */

#ifndef USER_DEFINED
#define USER_DEFINED 1
typedef struct User_tag User;
#endif

struct User_tag {
	List(User);

	Conn *conn;

	int edit_pos, read_lines, total_lines, crashed;
	char edit_buf[MAX_LINE];

	char name[MAX_NAME];
	char passwd[MAX_CRYPTED_PASSWD];

	char *real_name, *street, *zipcode, *city, *state, *country;
	char *phone, *email, *www, *doing, *reminder, *default_anon;
	char *timezone, *vanity, *xmsg_header, *away, *symbols;

	time_t birth, login_time, last_logout, online_timer, idle_time;
	unsigned long logins, total_time, last_online_time;
	unsigned long xsent, xrecv, esent, erecv, fsent, frecv, qsent, qansw, posted, read;
	long curr_msg;
	unsigned int flags, flags2, runtime_flags, default_room;
	int colors[9], symbol_colors[8], color, msg_seq_recv, msg_seq_sent;

	char *quick[NUM_QUICK];

	char *tmpbuf[NUM_TMP];
	char *question_asked;

	Joined *rooms;
	StringList *friends, *enemies, *override;
	StringQueue *recipients, *tablist, *chat_history;

	Message *message, *new_message;
	Room *mail, *curr_room;

	PList *history, *history_p, *held_msgs, *held_msgp;
	BufferedMsg *send_msg;

	Timer *timerq, *idle_timer;
	Timezone *tz;

	StringIO *text, *info;
	PQueue *scroll;
	PList *scrollp;

	Telnet *telnet;
	Display *display;
};

User *new_User(void);
void destroy_User(User *);

int load_User(User *, char *, int);
int load_User_version0(File *, User *, char *, int);
int site_load_User_version0(User *, char *, int);
int load_User_version1(File *, User *, char *, int);
int load_profile_info(User *);

int save_User(User *);
int save_User_version1(File *, User *);

void Write(User *, char *);
void Writechar(User *, char);
void Flush(User *);
void Put(User *, char *);
void Print(User *, char *, ...);
void Tell(User *, char *, ...);
void notify_friends(User *, char *, int);
void logout_user(User *);
void close_connection(User *, char *, ...);
void close_logout(void *);

extern User *AllUsers;
extern User *this_user;

#endif	/* USER_H_WJ97 */

/* EOB */
