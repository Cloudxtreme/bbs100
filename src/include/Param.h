/*
    bbs100 1.2.1 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	Param.h	WJ99
*/

#ifndef PARAM_H_WJ99
#define PARAM_H_WJ99 1

#define PARAM_STRING				0
#define PARAM_INT					1
#define PARAM_UINT					2
#define PARAM_LONG					3
#define PARAM_ULONG					4
#define PARAM_SEPARATOR				0x100
#define PARAM_MASK					0xff

/*
	Note: I have number the list in blocks, so it is easier for me to insert
	      a new define in between without having to manually renumber every
	      single entry
*/
#define PARAM_BBS_NAME				param[0].val.s
#define PARAM_PORT_NUMBER			param[1].val.u

#define PARAM_DIR_N					2
#define PARAM_BASEDIR				param[PARAM_DIR_N].val.s
#define PARAM_BINDIR				param[PARAM_DIR_N+1].val.s
#define PARAM_CONFDIR				param[PARAM_DIR_N+2].val.s
#define PARAM_FEELINGSDIR			param[PARAM_DIR_N+3].val.s
#define PARAM_ZONEINFODIR			param[PARAM_DIR_N+4].val.s
#define PARAM_USERDIR				param[PARAM_DIR_N+5].val.s
#define PARAM_ROOMDIR				param[PARAM_DIR_N+6].val.s
#define PARAM_TRASHDIR				param[PARAM_DIR_N+7].val.s

#define PARAM_PROGRAM_N				10
#define PARAM_PROGRAM_MAIN			param[PARAM_PROGRAM_N].val.s
#define PARAM_PROGRAM_RESOLVER		param[PARAM_PROGRAM_N+1].val.s

#define PARAM_SCREEN_N				12
#define PARAM_GPL_SCREEN			param[PARAM_SCREEN_N].val.s
#define PARAM_MODS_SCREEN			param[PARAM_SCREEN_N+1].val.s
#define PARAM_LOGIN_SCREEN			param[PARAM_SCREEN_N+2].val.s
#define PARAM_LOGOUT_SCREEN			param[PARAM_SCREEN_N+3].val.s
#define PARAM_NOLOGIN_SCREEN		param[PARAM_SCREEN_N+4].val.s
#define PARAM_MOTD_SCREEN			param[PARAM_SCREEN_N+5].val.s
#define PARAM_REBOOT_SCREEN			param[PARAM_SCREEN_N+6].val.s
#define PARAM_SHUTDOWN_SCREEN		param[PARAM_SCREEN_N+7].val.s
#define PARAM_CRASH_SCREEN			param[PARAM_SCREEN_N+8].val.s

#define PARAM_HELP_N				21
#define PARAM_FIRST_LOGIN			param[PARAM_HELP_N].val.s
#define PARAM_HELP_STD				param[PARAM_HELP_N+1].val.s
#define PARAM_HELP_CONFIG			param[PARAM_HELP_N+2].val.s
#define PARAM_HELP_ROOMCONFIG		param[PARAM_HELP_N+3].val.s
#define PARAM_HELP_SYSOP			param[PARAM_HELP_N+4].val.s

#define PARAM_FILE_N				26
#define PARAM_HOSTMAP_FILE			param[PARAM_FILE_N].val.s
#define PARAM_HOSTS_ACCESS_FILE		param[PARAM_FILE_N+1].val.s
#define PARAM_BANISHED_FILE			param[PARAM_FILE_N+2].val.s
#define PARAM_STAT_FILE				param[PARAM_FILE_N+3].val.s
#define PARAM_SU_PASSWD_FILE		param[PARAM_FILE_N+4].val.s
#define PARAM_PID_FILE				param[PARAM_FILE_N+5].val.s
#define PARAM_SYMTAB_FILE			param[PARAM_FILE_N+6].val.s

#define PARAM_LOG_N					33
#define PARAM_SYSLOG				param[PARAM_LOG_N].val.s
#define PARAM_AUTHLOG				param[PARAM_LOG_N+1].val.s
#define PARAM_LOGROTATE				param[PARAM_LOG_N+2].val.s
#define PARAM_ARCHIVEDIR			param[PARAM_LOG_N+3].val.s
#define PARAM_ONCRASH				param[PARAM_LOG_N+4].val.s
#define PARAM_CRASHDIR				param[PARAM_LOG_N+5].val.s

#define PARAM_MAX_N					39
#define PARAM_MAX_CACHED			param[PARAM_MAX_N].val.u
#define PARAM_MAX_MESSAGES			param[PARAM_MAX_N+1].val.u
#define PARAM_MAX_MAIL_MSGS			param[PARAM_MAX_N+2].val.u
#define PARAM_MAX_MSG_LINES			param[PARAM_MAX_N+3].val.u
#define PARAM_MAX_XMSG_LINES		param[PARAM_MAX_N+4].val.u
#define PARAM_MAX_HISTORY			param[PARAM_MAX_N+5].val.u
#define PARAM_MAX_CHAT_HISTORY		param[PARAM_MAX_N+6].val.u
#define PARAM_MAX_FRIEND			param[PARAM_MAX_N+7].val.u
#define PARAM_MAX_ENEMY				param[PARAM_MAX_N+8].val.u
#define PARAM_IDLE_TIMEOUT			param[PARAM_MAX_N+9].val.u
#define PARAM_LOCK_TIMEOUT			param[PARAM_MAX_N+10].val.u
#define PARAM_SAVE_TIMEOUT			param[PARAM_MAX_N+11].val.u
#define PARAM_USERHASH_SIZE			param[PARAM_MAX_N+12].val.u

#define PARAM_NAME_N				52
#define PARAM_NAME_SYSOP			param[PARAM_NAME_N].val.s
#define PARAM_NAME_ROOMAIDE			param[PARAM_NAME_N+1].val.s
#define PARAM_NAME_HELPER			param[PARAM_NAME_N+2].val.s
#define PARAM_NAME_GUEST			param[PARAM_NAME_N+3].val.s

#define PARAM_NOTIFY_N				56
#define PARAM_NOTIFY_LOGIN			param[PARAM_NOTIFY_N].val.s
#define PARAM_NOTIFY_LOGOUT			param[PARAM_NOTIFY_N+1].val.s
#define PARAM_NOTIFY_LINKDEAD		param[PARAM_NOTIFY_N+2].val.s
#define PARAM_NOTIFY_IDLE			param[PARAM_NOTIFY_N+3].val.s
#define PARAM_NOTIFY_LOCKED			param[PARAM_NOTIFY_N+4].val.s
#define PARAM_NOTIFY_UNLOCKED		param[PARAM_NOTIFY_N+5].val.s
#define PARAM_NOTIFY_ENTER_CHAT		param[PARAM_NOTIFY_N+6].val.s
#define PARAM_NOTIFY_LEAVE_CHAT		param[PARAM_NOTIFY_N+7].val.s

#define DEFAULT_PORT_1234			1234	/* default port number */

#define DEFAULT_MAX_CACHED			256		/* max # of objects in cache */
#define DEFAULT_MAX_MESSAGES		50		/* max messages kept in a room before expiring */
#define DEFAULT_MAX_MAIL_MSGS		50		/* max mail messages kept before expiring */
#define DEFAULT_MAX_HISTORY			50		/* max # of messages in history buffer */
#define DEFAULT_MAX_CHAT_HISTORY	10		/* max # of messages in a chatroom history */
#define DEFAULT_MAX_MSG_LINES		1000	/* max # of lines in a message */
#define DEFAULT_MAX_XMSG_LINES		7		/* max # of lines in an eXpress Message */
#define DEFAULT_MAX_FRIEND			100		/* max # of friends per user */
#define DEFAULT_MAX_ENEMY			100		/* max # of enemies per user */
#define DEFAULT_IDLE_TIMEOUT		10		/* 10 minute timeout */
#define DEFAULT_LOCK_TIMEOUT		30		/* 30 minute timeout */
#define DEFAULT_SAVE_TIMEOUT		5		/* save user every 5 minutes */
#define DEFAULT_USERHASH_SIZE		16		/* good for small and medium sized BBSes */

typedef union {
	char *str;
	char *s;
	int i;
	int d;
	unsigned int u;
	unsigned int ui;
	long l;
	long ld;
	unsigned long ul;
	unsigned long lu;
} Param_value;

typedef struct {
	int type;				/* PARAM_INT or PARAM_STRING or whatever */
	char *var;
	Param_value val;
	Param_value default_val;
} Param;

extern Param param[];

int init_Param(void);
int load_Param(char *);
int save_Param(char *);
void print_Param(void);

#endif	/* PARAM_H_WJ99 */

/* EOB */
