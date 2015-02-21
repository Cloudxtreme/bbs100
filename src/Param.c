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
	Param.c	WJ99
*/

#include "config.h"
#include "Param.h"
#include "defines.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"

#include <stdio.h>
#include <stdlib.h>


KVPair **param = NULL;


int init_Param(void) {
int i;

	if ((param = (KVPair **)Malloc(NUM_PARAM * sizeof(KVPair *), TYPE_POINTER)) == NULL)
		return -1;

	for(i = 0; i < NUM_PARAM; i++)
		if ((param[i] = new_KVPair()) == NULL)
			return -1;

	KVPair_setstring(KVPARAM_BBS_NAME,			"bbs_name",			"bbs100");
	KVPair_setstring(KVPARAM_BIND_ADDRESS,		"bind_address",		"0.0.0.0");
	KVPair_setstring(KVPARAM_PORT_NUMBER,		"port_number",		DEFAULT_PORT_1234);

	KVPair_setstring(KVPARAM_SEP1, "", "");

	KVPair_setstring(KVPARAM_BASEDIR,			"basedir",			".");
	KVPair_setstring(KVPARAM_BINDIR,			"bindir",			"bin/");
	KVPair_setstring(KVPARAM_CONFDIR,			"confdir",			"etc/");
	KVPair_setstring(KVPARAM_HELPDIR,			"helpdir",			"etc/help/");
	KVPair_setstring(KVPARAM_FEELINGSDIR,		"feelingsdir",		"etc/feelings/");
	KVPair_setstring(KVPARAM_ZONEINFODIR,		"zoneinfodir",		"etc/zoneinfo/");
	KVPair_setstring(KVPARAM_USERDIR,			"userdir",			"users/");
	KVPair_setstring(KVPARAM_ROOMDIR,			"roomdir",			"rooms/");
	KVPair_setstring(KVPARAM_TRASHDIR,			"trashdir",			"trash/");
	KVPair_setoctal(KVPARAM_UMASK,				"umask",			DEFAULT_UMASK);

	KVPair_setstring(KVPARAM_SEP2, "", "");

	KVPair_setstring(KVPARAM_PROGRAM_MAIN,		"program_main",		"bin/main");
	KVPair_setstring(KVPARAM_PROGRAM_RESOLVER,	"program_resolver",	"bin/resolver");

	KVPair_setstring(KVPARAM_SEP3, "", "");

	KVPair_setstring(KVPARAM_GPL_SCREEN,		"gpl_screen",		"etc/GPL");
	KVPair_setstring(KVPARAM_MODS_SCREEN,		"mods_screen",		"etc/local_mods");
	KVPair_setstring(KVPARAM_CREDITS_SCREEN,	"credits_screen",	"etc/credits");
	KVPair_setstring(KVPARAM_LOGIN_SCREEN,		"login_screen",		"etc/login");
	KVPair_setstring(KVPARAM_LOGOUT_SCREEN,		"logout_screen",	"etc/logout");
	KVPair_setstring(KVPARAM_NOLOGIN_SCREEN,	"nologin_screen",	"etc/nologin");
	KVPair_setstring(KVPARAM_MOTD_SCREEN,		"motd_screen",		"etc/motd");
	KVPair_setstring(KVPARAM_REBOOT_SCREEN,		"reboot_screen",	"etc/reboot");
	KVPair_setstring(KVPARAM_SHUTDOWN_SCREEN,	"shutdown_screen",	"etc/shutdown");
	KVPair_setstring(KVPARAM_CRASH_SCREEN,		"crash_screen",		"etc/crash");
	KVPair_setstring(KVPARAM_BOSS_SCREEN,		"boss_screen",		"etc/boss");
	KVPair_setstring(KVPARAM_FIRST_LOGIN,		"first_login",		"etc/first_login");

	KVPair_setstring(KVPARAM_SEP4, "", "");

	KVPair_setstring(KVPARAM_HOSTMAP_FILE,		"hostmap_file",		"etc/hostmap");
	KVPair_setstring(KVPARAM_HOSTS_ACCESS_FILE,	"hosts_access_file","etc/hosts_access");
	KVPair_setstring(KVPARAM_BANISHED_FILE,		"banished_file",	"etc/banished");
	KVPair_setstring(KVPARAM_STAT_FILE,			"stat_file",		"etc/stats");
	KVPair_setstring(KVPARAM_CATEGORIES_FILE,	"categories_file",	"etc/categories");
	KVPair_setstring(KVPARAM_SU_PASSWD_FILE,	"su_passwd_file",	"etc/su_passwd");
	KVPair_setstring(KVPARAM_PID_FILE,			"pid_file",			"etc/pid");
	KVPair_setstring(KVPARAM_SYMTAB_FILE,		"symtab_file",		"etc/symtab");
	KVPair_setstring(KVPARAM_DEFAULT_TIMEZONE,	"default_timezone",	"Europe/Amsterdam");

	KVPair_setstring(KVPARAM_SEP5, "", "");

	KVPair_setstring(KVPARAM_SYSLOG,			"syslog",			"log/bbslog");
	KVPair_setstring(KVPARAM_AUTHLOG,			"authlog",			"log/authlog");
	KVPair_setstring(KVPARAM_NEWUSERLOG,		"newusers",			"log/newusers");
	KVPair_setstring(KVPARAM_LOGROTATE,			"logrotate",		"daily");
	KVPair_setstring(KVPARAM_ARCHIVEDIR,		"archivedir",		"log/archive/");
	KVPair_setstring(KVPARAM_ONCRASH,			"oncrash",			"dumpcore");
	KVPair_setstring(KVPARAM_CRASHDIR,			"crashdir",			"log/crash/");

	KVPair_setstring(KVPARAM_SEP6, "", "");

	KVPair_setint(KVPARAM_MAX_CACHED,			"max_cached",		DEFAULT_MAX_CACHED);
	KVPair_setint(KVPARAM_MAX_MESSAGES,			"max_messages",		DEFAULT_MAX_MESSAGES);
	KVPair_setint(KVPARAM_MAX_MAIL_MSGS,		"max_mail_msgs",	DEFAULT_MAX_MAIL_MSGS);
	KVPair_setint(KVPARAM_MAX_MSG_LINES,		"max_msg_lines",	DEFAULT_MAX_MSG_LINES);
	KVPair_setint(KVPARAM_MAX_XMSG_LINES,		"max_xmsg_lines",	DEFAULT_MAX_XMSG_LINES);
	KVPair_setint(KVPARAM_MAX_HISTORY,			"max_history",		DEFAULT_MAX_HISTORY);
	KVPair_setint(KVPARAM_MAX_CHAT_HISTORY,		"max_chat_history",	DEFAULT_MAX_CHAT_HISTORY);
	KVPair_setint(KVPARAM_MAX_FRIEND,			"max_friend",		DEFAULT_MAX_FRIEND);
	KVPair_setint(KVPARAM_MAX_ENEMY,			"max_enemy",		DEFAULT_MAX_ENEMY);
	KVPair_setint(KVPARAM_MAX_NEWUSERLOG,		"max_newuserlog",	DEFAULT_MAX_NEWUSERLOG);
	KVPair_setint(KVPARAM_IDLE_TIMEOUT,			"idle_timeout",		DEFAULT_IDLE_TIMEOUT);
	KVPair_setint(KVPARAM_LOCK_TIMEOUT,			"lock_timeout",		DEFAULT_LOCK_TIMEOUT);
	KVPair_setint(KVPARAM_SAVE_TIMEOUT,			"periodic_saving",	DEFAULT_SAVE_TIMEOUT);
	KVPair_setint(KVPARAM_CACHE_TIMEOUT,		"cache_expire",		DEFAULT_CACHE_TIMEOUT);
	KVPair_setint(KVPARAM_HELPER_AGE,			"helper_age",		DEFAULT_HELPER_AGE);

	KVPair_setstring(KVPARAM_SEP7, "", "");

	KVPair_setstring(KVPARAM_NAME_SYSOP,		"name_sysop",		"Sysop");
	KVPair_setstring(KVPARAM_NAME_ROOMAIDE,		"name_room_aide",	"Room Aide");
	KVPair_setstring(KVPARAM_NAME_HELPER,		"name_helper",		"Helping Hand");
	KVPair_setstring(KVPARAM_NAME_GUEST,		"name_guest",		"Guest");

	KVPair_setstring(KVPARAM_SEP8, "", "");

	KVPair_setstring(KVPARAM_NOTIFY_LOGIN,		"notify_login",		"is formed from some <yellow>golden<magenta> stardust");
	KVPair_setstring(KVPARAM_NOTIFY_LOGOUT,		"notify_logout",	"explodes into <yellow>golden<magenta> stardust");
	KVPair_setstring(KVPARAM_NOTIFY_LINKDEAD,	"notify_linkdead",	"freezes up and crumbles to dust");
	KVPair_setstring(KVPARAM_NOTIFY_IDLE,		"notify_idle",		"has been logged off due to inactivity");
	KVPair_setstring(KVPARAM_NOTIFY_LOCKED,		"notify_locked",	"is away from the terminal for a while");
	KVPair_setstring(KVPARAM_NOTIFY_UNLOCKED,	"notify_unlocked",	"has returned to the terminal");
	KVPair_setstring(KVPARAM_NOTIFY_HOLD,		"notify_hold",		"has put messages on hold");
	KVPair_setstring(KVPARAM_NOTIFY_UNHOLD,		"notify_unhold",	"is accepting messages directly again");
	KVPair_setstring(KVPARAM_NOTIFY_ENTER_CHAT,	"notify_enter_chat","enters");
	KVPair_setstring(KVPARAM_NOTIFY_LEAVE_CHAT,	"notify_leave_chat","leaves");

	KVPair_setstring(KVPARAM_SEP9, "", "");

	KVPair_setbool(KVPARAM_HAVE_XMSGS,			"have_xmsgs",		PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_EMOTES,			"have_emotes",		PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_FEELINGS,		"have_feelings",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_QUESTIONS,		"have_questions",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_QUICK_X,		"have_quick_x",		PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_XMSG_HDR,		"have_xmsg_hdr",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_VANITY,			"have_vanity",		PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_TALKEDTO,		"have_talkedto",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_HOLD,			"have_hold_msgs",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_FOLLOWUP,		"have_followup",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_X_REPLY,		"have_x_reply",		PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_CALENDAR,		"have_calendar",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_WORLDCLOCK,		"have_worldclock",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_CHATROOMS,		"have_chatrooms",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_GUESSNAME,		"have_guessname_rooms", PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_HOMEROOM,		"have_homeroom",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_MAILROOM,		"have_mailroom",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_CATEGORY,		"have_category",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_WRAPPER_ALL,	"have_wrapper_all",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_FILECACHE,		"have_filecache",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_RESIDENT_INFO,	"have_resident_info",	PARAM_TRUE);
	KVPair_setbool(KVPARAM_HAVE_DISABLED_MSG,	"have_disabled_msg",PARAM_TRUE);

	return 0;
}

int load_Param(char *filename) {
AtomicFile *f;
char buf[PRINT_BUF], *p;
int i, line_no, errors;

	if (param == NULL)
		return -1;

	if ((f = openfile(filename, "r")) == NULL)
		return 1;

	line_no = errors = 0;
	while(fgets(buf, PRINT_BUF, f->f) != NULL) {
		line_no++;

		cstrip_line(buf);
		cstrip_spaces(buf);
		if (!*buf || *buf == '#')
			continue;

		if ((p = cstrchr(buf, ' ')) != NULL) {
			*p = 0;
			p++;
			if (!*p)
				continue;

			for(i = 0; i < NUM_PARAM; i++) {
				if (param[i]->key == NULL || !param[i]->key[0])
					continue;

				if (!cstricmp(buf, param[i]->key)) {
					switch(param[i]->type) {
						case KV_STRING:
							Free(param[i]->value.s);
							param[i]->value.s = cstrdup(p);
							break;

						case KV_INT:
							if (p[0] == '-') {
								fprintf(stderr, "%s:%d: %s cannot have a negative value\n", filename, line_no, param[i]->key);
								errors++;
								break;
							}
							if (!is_numeric(p)) {
								fprintf(stderr, "%s:%d: error in integer format for param %s\n", filename, line_no, param[i]->key);
								errors++;
								break;
							}
							param[i]->value.i = (int)cstrtoul(p, 10);
							break;

						case KV_OCTAL:
							if (p[0] == '-') {
								fprintf(stderr, "%s:%d: %s cannot have a negative value\n", filename, line_no, param[i]->key);
								errors++;
								break;
							}
							if (!is_octal(p)) {
								fprintf(stderr, "%s:%d: error in octal integer format for param %s\n", filename, line_no, param[i]->key);
								errors++;
								break;
							}
							param[i]->value.o = (int)cstrtoul(p, 8);
							break;

						case KV_BOOL:
							if (!cstricmp(p, "yes") || !cstricmp(p, "on") || !cstricmp(p, "true") || !strcmp(p, "1"))
								param[i]->value.bool = KV_TRUE;
							else
								if (!cstricmp(p, "no") || !cstricmp(p, "off") || !cstricmp(p, "false") || !strcmp(p, "0"))
									param[i]->value.bool = KV_FALSE;
								else {
									fprintf(stderr, "%s:%d: unknown value '%s' for param %s\n", filename, line_no, p, param[i]->key);
									errors++;
								}
							break;

						default:
							closefile(f);
							fprintf(stderr, "%s:%d BUG! wrong type '%d' for param[%d] (%s)\n", filename, line_no, param[i]->type, i, param[i]->key);
							errors++;
					}
					break;
				}
			}
			if (i >= NUM_PARAM)
				fprintf(stderr, "%s:%d: unknown keyword '%s', ignored\n", filename, line_no, buf);
		}
	}
	closefile(f);
	return -errors;
}

int save_Param(char *filename) {
AtomicFile *f;
char buf[PRINT_BUF*3];
int i;

	if ((f = openfile(filename, "w")) == NULL)
		return -1;

	fprintf(f->f, "#\n"
		"# %s param file %s\n"
		"#\n"
		"\n", PARAM_BBS_NAME, filename);

	for(i = 0; i < NUM_PARAM; i++) {
		if (param[i]->key != NULL && param[i]->key[0]) {
			print_KVPair(param[i], buf, PRINT_BUF*3);
			fprintf(f->f, "%-22s %s\n", param[i]->key, buf);
		} else
			fprintf(f->f, "\n");
	}
	fprintf(f->f, "\n# EOB\n");
	return closefile(f);
}


/*
	check for decent integer values
*/
void check_Param(void) {
/*
	if max_cached < 0, the user probably meant to turn it off
*/	
	if (PARAM_MAX_CACHED < 0) {
		PARAM_MAX_CACHED = 0;
		printf("invalid value for max_cached, cache disabled\n");
	}

#define PARAM_CHECK(x,y,z)	\
	if ((y) < 1) {			\
		(y) = (z);			\
		printf("invalid value for %s, reset to default %d\n", (x), (z));	\
	}

	PARAM_CHECK("max_messages",		PARAM_MAX_MESSAGES,		DEFAULT_MAX_MESSAGES);
	PARAM_CHECK("max_mail_msgs",	PARAM_MAX_MAIL_MSGS,	DEFAULT_MAX_MAIL_MSGS);
	PARAM_CHECK("max_history",		PARAM_MAX_HISTORY,		DEFAULT_MAX_HISTORY);
	PARAM_CHECK("max_chat_history",	PARAM_MAX_CHAT_HISTORY,	DEFAULT_MAX_CHAT_HISTORY);
	PARAM_CHECK("max_msg_lines",	PARAM_MAX_MSG_LINES,	DEFAULT_MAX_MSG_LINES);
	PARAM_CHECK("max_xmsg_lines",	PARAM_MAX_XMSG_LINES,	DEFAULT_MAX_XMSG_LINES);
	PARAM_CHECK("max_friend",		PARAM_MAX_FRIEND,		DEFAULT_MAX_FRIEND);
	PARAM_CHECK("max_enemy",		PARAM_MAX_ENEMY,		DEFAULT_MAX_ENEMY);
	PARAM_CHECK("idle_timeout",		PARAM_IDLE_TIMEOUT,		DEFAULT_IDLE_TIMEOUT);
	PARAM_CHECK("lock_timeout",		PARAM_LOCK_TIMEOUT,		DEFAULT_LOCK_TIMEOUT);
	PARAM_CHECK("periodic_saving",	PARAM_SAVE_TIMEOUT,		DEFAULT_SAVE_TIMEOUT);
	PARAM_CHECK("cache_expire",		PARAM_CACHE_TIMEOUT,	DEFAULT_CACHE_TIMEOUT);
}

/*
	used at startup for a pretty boot screen
*/
void print_Param(void) {
int i;
char buf[PRINT_BUF*3];

	for(i = 0; i < NUM_PARAM; i++) {
		if (param[i]->key != NULL && param[i]->key[0]) {
			print_KVPair(param[i], buf, PRINT_BUF*3);
			printf("%-22s %s\n", param[i]->key, buf);
		} else
			printf("\n");
	}
	printf("\n");
}

/* EOB */
