/*
    bbs100 1.2.0 WJ102
    Copyright (C) 2002  Walter de Jong <walter@heiho.net>

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

#define PARAM_BBS_NAME				param[0].val.s
#define PARAM_PORT_NUMBER			param[1].val.u

#define PARAM_BASEDIR				param[2].val.s
#define PARAM_BINDIR				param[3].val.s
#define PARAM_CONFDIR				param[4].val.s
#define PARAM_FEELINGSDIR			param[5].val.s
#define PARAM_USERDIR				param[6].val.s
#define PARAM_ROOMDIR				param[7].val.s
#define PARAM_TRASHDIR				param[8].val.s

#define PARAM_PROGRAM_MAIN			param[9].val.s
#define PARAM_PROGRAM_RESOLVER		param[10].val.s
#define PARAM_PROGRAM_LOGD			param[11].val.s

#define PARAM_GPL_SCREEN			param[12].val.s
#define PARAM_MODS_SCREEN			param[13].val.s
#define PARAM_LOGIN_SCREEN			param[14].val.s
#define PARAM_LOGOUT_SCREEN			param[15].val.s
#define PARAM_NOLOGIN_SCREEN		param[16].val.s
#define PARAM_MOTD_SCREEN			param[17].val.s
#define PARAM_REBOOT_SCREEN			param[18].val.s
#define PARAM_SHUTDOWN_SCREEN		param[19].val.s
#define PARAM_CRASH_SCREEN			param[20].val.s

#define PARAM_FIRST_LOGIN			param[21].val.s
#define PARAM_HELP_STD				param[22].val.s
#define PARAM_HELP_CONFIG			param[23].val.s
#define PARAM_HELP_ROOMCONFIG		param[24].val.s
#define PARAM_HELP_SYSOP			param[25].val.s

#define PARAM_HOSTMAP_FILE			param[26].val.s
#define PARAM_HOSTS_ACCESS_FILE		param[27].val.s
#define PARAM_BANISHED_FILE			param[28].val.s
#define PARAM_STAT_FILE				param[29].val.s
#define PARAM_SU_PASSWD_FILE		param[30].val.s
#define PARAM_PID_FILE				param[31].val.s
#define PARAM_SYMTAB_FILE			param[32].val.s

#define PARAM_MAX_CACHED			param[33].val.u
#define PARAM_MAX_MESSAGES			param[34].val.u
#define PARAM_MAX_MAIL_MSGS			param[35].val.u
#define PARAM_MAX_MSG_LINES			param[36].val.u
#define PARAM_MAX_XMSG_LINES		param[37].val.u
#define PARAM_MAX_HISTORY			param[38].val.u
#define PARAM_MAX_CHAT_HISTORY		param[39].val.u
#define PARAM_MAX_FRIEND			param[40].val.u
#define PARAM_MAX_ENEMY				param[41].val.u
#define PARAM_IDLE_TIMEOUT			param[42].val.u
#define PARAM_LOCK_TIMEOUT			param[43].val.u
#define PARAM_SAVE_TIMEOUT			param[44].val.u
#define PARAM_USERHASH_SIZE			param[45].val.u

#define PARAM_NAME_SYSOP			param[46].val.s
#define PARAM_NAME_ROOMAIDE			param[47].val.s
#define PARAM_NAME_HELPER			param[48].val.s
#define PARAM_NAME_GUEST			param[49].val.s

#define PARAM_NOTIFY_LOGIN			param[50].val.s
#define PARAM_NOTIFY_LOGOUT			param[51].val.s
#define PARAM_NOTIFY_LINKDEAD		param[52].val.s
#define PARAM_NOTIFY_IDLE			param[53].val.s
#define PARAM_NOTIFY_LOCKED			param[54].val.s
#define PARAM_NOTIFY_UNLOCKED		param[55].val.s
#define PARAM_NOTIFY_ENTER_CHAT		param[56].val.s
#define PARAM_NOTIFY_LEAVE_CHAT		param[57].val.s

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
	int type;				/* PARAM_INT or PARAM_STRING */
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
