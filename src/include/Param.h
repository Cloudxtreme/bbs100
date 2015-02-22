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
	Param.h	WJ99
*/

#ifndef PARAM_H_WJ99
#define PARAM_H_WJ99 1

#include "KVPair.h"

#define PARAM_TRUE		KV_TRUE
#define PARAM_FALSE		KV_FALSE

/*
	Note: I have numbered the list in blocks, so it is easier for me to insert
	      a new define in between without having to renumber every single entry

	It would be easier to put the params in a Hash, but this code is much faster
*/
#define KVPARAM_BBS_NAME			param[0]
#define KVPARAM_BIND_ADDRESS		param[1]
#define KVPARAM_PORT_NUMBER			param[2]

#define KVPARAM_DIR_N				3
#define KVPARAM_SEP1				param[KVPARAM_DIR_N]

#define KVPARAM_BASEDIR				param[KVPARAM_DIR_N+1]
#define KVPARAM_BINDIR				param[KVPARAM_DIR_N+2]
#define KVPARAM_CONFDIR				param[KVPARAM_DIR_N+3]
#define KVPARAM_HELPDIR				param[KVPARAM_DIR_N+4]
#define KVPARAM_FEELINGSDIR			param[KVPARAM_DIR_N+5]
#define KVPARAM_ZONEINFODIR			param[KVPARAM_DIR_N+6]
#define KVPARAM_USERDIR				param[KVPARAM_DIR_N+7]
#define KVPARAM_ROOMDIR				param[KVPARAM_DIR_N+8]
#define KVPARAM_TRASHDIR			param[KVPARAM_DIR_N+9]
#define KVPARAM_UMASK				param[KVPARAM_DIR_N+10]

#define KVPARAM_PROGRAM_N			(KVPARAM_DIR_N+11)
#define KVPARAM_SEP2				param[KVPARAM_PROGRAM_N]

#define KVPARAM_PROGRAM_MAIN		param[KVPARAM_PROGRAM_N+1]
#define KVPARAM_PROGRAM_RESOLVER	param[KVPARAM_PROGRAM_N+2]

#define KVPARAM_SCREEN_N			(KVPARAM_PROGRAM_N+3)
#define KVPARAM_SEP3				param[KVPARAM_SCREEN_N]

#define KVPARAM_GPL_SCREEN			param[KVPARAM_SCREEN_N+1]
#define KVPARAM_MODS_SCREEN			param[KVPARAM_SCREEN_N+2]
#define KVPARAM_CREDITS_SCREEN		param[KVPARAM_SCREEN_N+3]
#define KVPARAM_LOGIN_SCREEN		param[KVPARAM_SCREEN_N+4]
#define KVPARAM_LOGOUT_SCREEN		param[KVPARAM_SCREEN_N+5]
#define KVPARAM_NOLOGIN_SCREEN		param[KVPARAM_SCREEN_N+6]
#define KVPARAM_MOTD_SCREEN			param[KVPARAM_SCREEN_N+7]
#define KVPARAM_REBOOT_SCREEN		param[KVPARAM_SCREEN_N+8]
#define KVPARAM_SHUTDOWN_SCREEN		param[KVPARAM_SCREEN_N+9]
#define KVPARAM_CRASH_SCREEN		param[KVPARAM_SCREEN_N+10]
#define KVPARAM_BOSS_SCREEN			param[KVPARAM_SCREEN_N+11]
#define KVPARAM_FIRST_LOGIN			param[KVPARAM_SCREEN_N+12]

#define KVPARAM_FILE_N				(KVPARAM_SCREEN_N+13)
#define KVPARAM_SEP4				param[KVPARAM_FILE_N]

#define KVPARAM_HOSTMAP_FILE		param[KVPARAM_FILE_N+1]
#define KVPARAM_HOSTS_ACCESS_FILE	param[KVPARAM_FILE_N+2]
#define KVPARAM_BANISHED_FILE		param[KVPARAM_FILE_N+3]
#define KVPARAM_STAT_FILE			param[KVPARAM_FILE_N+4]
#define KVPARAM_CATEGORIES_FILE		param[KVPARAM_FILE_N+5]
#define KVPARAM_SU_PASSWD_FILE		param[KVPARAM_FILE_N+6]
#define KVPARAM_PID_FILE			param[KVPARAM_FILE_N+7]
#define KVPARAM_SYMTAB_FILE			param[KVPARAM_FILE_N+8]
#define KVPARAM_DEFAULT_TIMEZONE	param[KVPARAM_FILE_N+9]

#define KVPARAM_LOG_N				(KVPARAM_FILE_N+10)
#define KVPARAM_SEP5				param[KVPARAM_LOG_N]

#define KVPARAM_SYSLOG				param[KVPARAM_LOG_N+1]
#define KVPARAM_AUTHLOG				param[KVPARAM_LOG_N+2]
#define KVPARAM_NEWUSERLOG			param[KVPARAM_LOG_N+3]
#define KVPARAM_LOGROTATE			param[KVPARAM_LOG_N+4]
#define KVPARAM_ARCHIVEDIR			param[KVPARAM_LOG_N+5]
#define KVPARAM_ONCRASH				param[KVPARAM_LOG_N+6]
#define KVPARAM_CRASHDIR			param[KVPARAM_LOG_N+7]

#define KVPARAM_MAX_N				(KVPARAM_LOG_N+8)
#define KVPARAM_SEP6				param[KVPARAM_MAX_N]

#define KVPARAM_MAX_CACHED			param[KVPARAM_MAX_N+1]
#define KVPARAM_MAX_MESSAGES		param[KVPARAM_MAX_N+2]
#define KVPARAM_MAX_MAIL_MSGS		param[KVPARAM_MAX_N+3]
#define KVPARAM_MAX_MSG_LINES		param[KVPARAM_MAX_N+4]
#define KVPARAM_MAX_XMSG_LINES		param[KVPARAM_MAX_N+5]
#define KVPARAM_MAX_HISTORY			param[KVPARAM_MAX_N+6]
#define KVPARAM_MAX_CHAT_HISTORY	param[KVPARAM_MAX_N+7]
#define KVPARAM_MAX_FRIEND			param[KVPARAM_MAX_N+8]
#define KVPARAM_MAX_ENEMY			param[KVPARAM_MAX_N+9]
#define KVPARAM_MAX_NEWUSERLOG		param[KVPARAM_MAX_N+10]
#define KVPARAM_IDLE_TIMEOUT		param[KVPARAM_MAX_N+11]
#define KVPARAM_LOCK_TIMEOUT		param[KVPARAM_MAX_N+12]
#define KVPARAM_SAVE_TIMEOUT		param[KVPARAM_MAX_N+13]
#define KVPARAM_CACHE_TIMEOUT		param[KVPARAM_MAX_N+14]
#define KVPARAM_HELPER_AGE			param[KVPARAM_MAX_N+15]

#define KVPARAM_NAME_N				(KVPARAM_MAX_N+16)
#define KVPARAM_SEP7				param[KVPARAM_NAME_N]

#define KVPARAM_NAME_SYSOP			param[KVPARAM_NAME_N+1]
#define KVPARAM_NAME_ROOMAIDE		param[KVPARAM_NAME_N+2]
#define KVPARAM_NAME_HELPER			param[KVPARAM_NAME_N+3]
#define KVPARAM_NAME_GUEST			param[KVPARAM_NAME_N+4]

#define KVPARAM_NOTIFY_N			(KVPARAM_NAME_N+5)
#define KVPARAM_SEP8				param[KVPARAM_NOTIFY_N]

#define KVPARAM_NOTIFY_LOGIN		param[KVPARAM_NOTIFY_N+1]
#define KVPARAM_NOTIFY_LOGOUT		param[KVPARAM_NOTIFY_N+2]
#define KVPARAM_NOTIFY_LINKDEAD		param[KVPARAM_NOTIFY_N+3]
#define KVPARAM_NOTIFY_IDLE			param[KVPARAM_NOTIFY_N+4]
#define KVPARAM_NOTIFY_LOCKED		param[KVPARAM_NOTIFY_N+5]
#define KVPARAM_NOTIFY_UNLOCKED		param[KVPARAM_NOTIFY_N+6]
#define KVPARAM_NOTIFY_HOLD			param[KVPARAM_NOTIFY_N+7]
#define KVPARAM_NOTIFY_UNHOLD		param[KVPARAM_NOTIFY_N+8]
#define KVPARAM_NOTIFY_ENTER_CHAT	param[KVPARAM_NOTIFY_N+9]
#define KVPARAM_NOTIFY_LEAVE_CHAT	param[KVPARAM_NOTIFY_N+10]

#define KVPARAM_HAVE_N				(KVPARAM_NOTIFY_N+11)
#define KVPARAM_SEP9				param[KVPARAM_HAVE_N]

#define KVPARAM_HAVE_XMSGS			param[KVPARAM_HAVE_N+1]
#define KVPARAM_HAVE_EMOTES			param[KVPARAM_HAVE_N+2]
#define KVPARAM_HAVE_FEELINGS		param[KVPARAM_HAVE_N+3]
#define KVPARAM_HAVE_QUESTIONS		param[KVPARAM_HAVE_N+4]
#define KVPARAM_HAVE_QUICK_X		param[KVPARAM_HAVE_N+5]
#define KVPARAM_HAVE_XMSG_HDR		param[KVPARAM_HAVE_N+6]
#define KVPARAM_HAVE_VANITY			param[KVPARAM_HAVE_N+7]
#define KVPARAM_HAVE_TALKEDTO		param[KVPARAM_HAVE_N+8]
#define KVPARAM_HAVE_HOLD			param[KVPARAM_HAVE_N+9]
#define KVPARAM_HAVE_FOLLOWUP		param[KVPARAM_HAVE_N+10]
#define KVPARAM_HAVE_X_REPLY		param[KVPARAM_HAVE_N+11]
#define KVPARAM_HAVE_CALENDAR		param[KVPARAM_HAVE_N+12]
#define KVPARAM_HAVE_WORLDCLOCK		param[KVPARAM_HAVE_N+13]
#define KVPARAM_HAVE_CHATROOMS		param[KVPARAM_HAVE_N+14]
#define KVPARAM_HAVE_GUESSNAME		param[KVPARAM_HAVE_N+15]
#define KVPARAM_HAVE_HOMEROOM		param[KVPARAM_HAVE_N+16]
#define KVPARAM_HAVE_MAILROOM		param[KVPARAM_HAVE_N+17]
#define KVPARAM_HAVE_CATEGORY		param[KVPARAM_HAVE_N+18]
#define KVPARAM_HAVE_WRAPPER_ALL	param[KVPARAM_HAVE_N+19]
#define KVPARAM_HAVE_FILECACHE		param[KVPARAM_HAVE_N+20]
#define KVPARAM_HAVE_RESIDENT_INFO	param[KVPARAM_HAVE_N+21]
#define KVPARAM_HAVE_DISABLED_MSG	param[KVPARAM_HAVE_N+22]
#define NUM_PARAM					(KVPARAM_HAVE_N+23)

#define SPARAM(x)					(KVPARAM_##x)->value.s
#define IPARAM(x)					(KVPARAM_##x)->value.i
#define OPARAM(x)					(KVPARAM_##x)->value.o
#define LPARAM(x)					(KVPARAM_##x)->value.l
#define BPARAM(x)					(KVPARAM_##x)->value.bool
#define VPARAM(x)					(KVPARAM_##x)->value.v

#define PARAM_BBS_NAME				SPARAM(BBS_NAME)
#define PARAM_BIND_ADDRESS			SPARAM(BIND_ADDRESS)
#define PARAM_PORT_NUMBER			SPARAM(PORT_NUMBER)

#define PARAM_BASEDIR				SPARAM(BASEDIR)
#define PARAM_BINDIR				SPARAM(BINDIR)
#define PARAM_CONFDIR				SPARAM(CONFDIR)
#define PARAM_HELPDIR				SPARAM(HELPDIR)
#define PARAM_FEELINGSDIR			SPARAM(FEELINGSDIR)
#define PARAM_ZONEINFODIR			SPARAM(ZONEINFODIR)
#define PARAM_LANGUAGEDIR			SPARAM(LANGUAGEDIR)
#define PARAM_USERDIR				SPARAM(USERDIR)
#define PARAM_ROOMDIR				SPARAM(ROOMDIR)
#define PARAM_TRASHDIR				SPARAM(TRASHDIR)
#define PARAM_UMASK                 OPARAM(UMASK)

#define PARAM_PROGRAM_MAIN			SPARAM(PROGRAM_MAIN)
#define PARAM_PROGRAM_RESOLVER		SPARAM(PROGRAM_RESOLVER)

#define PARAM_GPL_SCREEN			SPARAM(GPL_SCREEN)
#define PARAM_MODS_SCREEN			SPARAM(MODS_SCREEN)
#define PARAM_CREDITS_SCREEN		SPARAM(CREDITS_SCREEN)
#define PARAM_LOGIN_SCREEN			SPARAM(LOGIN_SCREEN)
#define PARAM_LOGOUT_SCREEN			SPARAM(LOGOUT_SCREEN)
#define PARAM_NOLOGIN_SCREEN		SPARAM(NOLOGIN_SCREEN)
#define PARAM_MOTD_SCREEN			SPARAM(MOTD_SCREEN)
#define PARAM_REBOOT_SCREEN			SPARAM(REBOOT_SCREEN)
#define PARAM_SHUTDOWN_SCREEN		SPARAM(SHUTDOWN_SCREEN)
#define PARAM_CRASH_SCREEN			SPARAM(CRASH_SCREEN)
#define PARAM_BOSS_SCREEN			SPARAM(BOSS_SCREEN)

#define PARAM_FIRST_LOGIN			SPARAM(FIRST_LOGIN)
#define PARAM_HELP_DIR				SPARAM(HELP_DIR)
#define PARAM_HELP_CONFIG			SPARAM(HELP_CONFIG)
#define PARAM_HELP_ROOMCONFIG		SPARAM(HELP_ROOMCONFIG)
#define PARAM_HELP_SYSOP			SPARAM(HELP_SYSOP)

#define PARAM_HOSTMAP_FILE			SPARAM(HOSTMAP_FILE)
#define PARAM_HOSTS_ACCESS_FILE		SPARAM(HOSTS_ACCESS_FILE)
#define PARAM_BANISHED_FILE			SPARAM(BANISHED_FILE)
#define PARAM_STAT_FILE				SPARAM(STAT_FILE)
#define PARAM_CATEGORIES_FILE		SPARAM(CATEGORIES_FILE)
#define PARAM_SU_PASSWD_FILE		SPARAM(SU_PASSWD_FILE)
#define PARAM_PID_FILE				SPARAM(PID_FILE)
#define PARAM_SYMTAB_FILE			SPARAM(SYMTAB_FILE)
#define PARAM_DEFAULT_TIMEZONE		SPARAM(DEFAULT_TIMEZONE)

#define PARAM_SYSLOG				SPARAM(SYSLOG)
#define PARAM_AUTHLOG				SPARAM(AUTHLOG)
#define PARAM_NEWUSERLOG			SPARAM(NEWUSERLOG)
#define PARAM_LOGROTATE				SPARAM(LOGROTATE)
#define PARAM_ARCHIVEDIR			SPARAM(ARCHIVEDIR)
#define PARAM_ONCRASH				SPARAM(ONCRASH)
#define PARAM_CRASHDIR				SPARAM(CRASHDIR)

#define PARAM_MAX_CACHED			IPARAM(MAX_CACHED)
#define PARAM_MAX_MESSAGES			IPARAM(MAX_MESSAGES)
#define PARAM_MAX_MAIL_MSGS			IPARAM(MAX_MAIL_MSGS)
#define PARAM_MAX_MSG_LINES			IPARAM(MAX_MSG_LINES)
#define PARAM_MAX_XMSG_LINES		IPARAM(MAX_XMSG_LINES)
#define PARAM_MAX_HISTORY			IPARAM(MAX_HISTORY)
#define PARAM_MAX_CHAT_HISTORY		IPARAM(MAX_CHAT_HISTORY)
#define PARAM_MAX_FRIEND			IPARAM(MAX_FRIEND)
#define PARAM_MAX_ENEMY				IPARAM(MAX_ENEMY)
#define PARAM_MAX_NEWUSERLOG		IPARAM(MAX_NEWUSERLOG)
#define PARAM_IDLE_TIMEOUT			IPARAM(IDLE_TIMEOUT)
#define PARAM_LOCK_TIMEOUT			IPARAM(LOCK_TIMEOUT)
#define PARAM_SAVE_TIMEOUT			IPARAM(SAVE_TIMEOUT)
#define PARAM_CACHE_TIMEOUT			IPARAM(CACHE_TIMEOUT)
#define PARAM_HELPER_AGE			IPARAM(HELPER_AGE)
#define PARAM_CHUNK_SIZE			IPARAM(CHUNK_SIZE)

#define PARAM_NAME_SYSOP			SPARAM(NAME_SYSOP)
#define PARAM_NAME_ROOMAIDE			SPARAM(NAME_ROOMAIDE)
#define PARAM_NAME_HELPER			SPARAM(NAME_HELPER)
#define PARAM_NAME_GUEST			SPARAM(NAME_GUEST)

#define PARAM_NOTIFY_LOGIN			SPARAM(NOTIFY_LOGIN)
#define PARAM_NOTIFY_LOGOUT			SPARAM(NOTIFY_LOGOUT)
#define PARAM_NOTIFY_LINKDEAD		SPARAM(NOTIFY_LINKDEAD)
#define PARAM_NOTIFY_IDLE			SPARAM(NOTIFY_IDLE)
#define PARAM_NOTIFY_LOCKED			SPARAM(NOTIFY_LOCKED)
#define PARAM_NOTIFY_UNLOCKED		SPARAM(NOTIFY_UNLOCKED)
#define PARAM_NOTIFY_HOLD			SPARAM(NOTIFY_HOLD)
#define PARAM_NOTIFY_UNHOLD			SPARAM(NOTIFY_UNHOLD)
#define PARAM_NOTIFY_ENTER_CHAT		SPARAM(NOTIFY_ENTER_CHAT)
#define PARAM_NOTIFY_LEAVE_CHAT		SPARAM(NOTIFY_LEAVE_CHAT)

#define PARAM_HAVE_XMSGS			BPARAM(HAVE_XMSGS)
#define PARAM_HAVE_EMOTES			BPARAM(HAVE_EMOTES)
#define PARAM_HAVE_FEELINGS			BPARAM(HAVE_FEELINGS)
#define PARAM_HAVE_QUESTIONS		BPARAM(HAVE_QUESTIONS)
#define PARAM_HAVE_QUICK_X			BPARAM(HAVE_QUICK_X)
#define PARAM_HAVE_XMSG_HDR			BPARAM(HAVE_XMSG_HDR)
#define PARAM_HAVE_VANITY			BPARAM(HAVE_VANITY)
#define PARAM_HAVE_TALKEDTO			BPARAM(HAVE_TALKEDTO)
#define PARAM_HAVE_HOLD				BPARAM(HAVE_HOLD)
#define PARAM_HAVE_FOLLOWUP			BPARAM(HAVE_FOLLOWUP)
#define PARAM_HAVE_X_REPLY			BPARAM(HAVE_X_REPLY)
#define PARAM_HAVE_CALENDAR			BPARAM(HAVE_CALENDAR)
#define PARAM_HAVE_WORLDCLOCK		BPARAM(HAVE_WORLDCLOCK)
#define PARAM_HAVE_CHATROOMS		BPARAM(HAVE_CHATROOMS)
#define PARAM_HAVE_GUESSNAME		BPARAM(HAVE_GUESSNAME)
#define PARAM_HAVE_HOMEROOM			BPARAM(HAVE_HOMEROOM)
#define PARAM_HAVE_MAILROOM			BPARAM(HAVE_MAILROOM)
#define PARAM_HAVE_LANGUAGE			BPARAM(HAVE_LANGUAGE)
#define PARAM_HAVE_CATEGORY			BPARAM(HAVE_CATEGORY)
#define PARAM_HAVE_WRAPPER_ALL		BPARAM(HAVE_WRAPPER_ALL)
#define PARAM_HAVE_BINALLOC			BPARAM(HAVE_BINALLOC)
#define PARAM_HAVE_FILECACHE		BPARAM(HAVE_FILECACHE)
#define PARAM_HAVE_RESIDENT_INFO	BPARAM(HAVE_RESIDENT_INFO)
#define PARAM_HAVE_DISABLED_MSG		BPARAM(HAVE_DISABLED_MSG)

#define DEFAULT_PORT_1234			"1234"	/* default port number (can also be a service name) */

#define DEFAULT_MAX_CACHED			256		/* max # of objects in cache */
#define DEFAULT_MAX_MESSAGES		50		/* max messages kept in a room before expiring */
#define DEFAULT_MAX_MAIL_MSGS		50		/* max mail messages kept before expiring */
#define DEFAULT_MAX_HISTORY			50		/* max # of messages in history buffer */
#define DEFAULT_MAX_CHAT_HISTORY	10		/* max # of messages in a chatroom history */
#define DEFAULT_MAX_MSG_LINES		1000	/* max # of lines in a message */
#define DEFAULT_MAX_XMSG_LINES		7		/* max # of lines in an eXpress Message */
#define DEFAULT_MAX_FRIEND			100		/* max # of friends per user */
#define DEFAULT_MAX_ENEMY			100		/* max # of enemies per user */
#define DEFAULT_MAX_NEWUSERLOG		20		/* number of entries in the newusers log */
#define DEFAULT_IDLE_TIMEOUT		10		/* 10 minute timeout */
#define DEFAULT_LOCK_TIMEOUT		30		/* 30 minute timeout */
#define DEFAULT_SAVE_TIMEOUT		5		/* save user every 5 minutes */
#define DEFAULT_CACHE_TIMEOUT		30		/* expire unused cached files every 30 minutes */
#define DEFAULT_HELPER_AGE			1		/* minimum age in days required for Helper status */
#define DEFAULT_UMASK				007		/* allow user+group, deny others */

extern KVPair **param;

int init_Param(void);
int load_Param(char *);
int save_Param(char *);
void check_Param(void);
void print_Param(void);

#endif	/* PARAM_H_WJ99 */

/* EOB */
