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

Param param[] = {
	{ PARAM_STRING,	"bbs_name",			{ NULL },	{ "bbs100" },					},
	{ PARAM_INT | PARAM_SEPARATOR,
					"port_number",		{ NULL },	{ (char *)DEFAULT_PORT_1234 },	},

	{ PARAM_STRING, "basedir",			{ NULL },	{ "." },						},
	{ PARAM_STRING,	"bindir",			{ NULL },	{ "bin/" },						},
	{ PARAM_STRING,	"confdir",			{ NULL },	{ "etc/" },						},
	{ PARAM_STRING, "feelingsdir",		{ NULL },	{ "etc/feelings/" },			},
	{ PARAM_STRING, "zoneinfodir",		{ NULL },	{ "etc/zoneinfo/" },			},
	{ PARAM_STRING, "languagedir",		{ NULL },	{ "etc/language/" },			},
	{ PARAM_STRING,	"userdir",			{ NULL },	{ "users/" }, 					},
	{ PARAM_STRING,	"roomdir",			{ NULL },	{ "rooms/" },					},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"trashdir",			{ NULL },	{ "trash/" },					},

	{ PARAM_STRING, "program_main",		{ NULL },	{ "bin/main" },					},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"program_resolver",	{ NULL },	{ "bin/resolver" },				},

	{ PARAM_STRING, "gpl_screen",		{ NULL },	{ "etc/GPL" },					},
	{ PARAM_STRING, "mods_screen",		{ NULL },	{ "etc/local_mods" },			},
	{ PARAM_STRING,	"login_screen",		{ NULL },	{ "etc/login" },				},
	{ PARAM_STRING,	"logout_screen",	{ NULL },	{ "etc/logout" },				},
	{ PARAM_STRING,	"nologin_screen",	{ NULL },	{ "etc/nologin" },				},
	{ PARAM_STRING,	"motd_screen",		{ NULL },	{ "etc/motd" },					},
	{ PARAM_STRING,	"reboot_screen",	{ NULL },	{ "etc/reboot" },				},
	{ PARAM_STRING,	"shutdown_screen",	{ NULL },	{ "etc/shutdown" },				},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"crash_screen",		{ NULL },	{ "etc/crash" },				},

	{ PARAM_STRING,	"first_login",		{ NULL },	{ "etc/first_login" },			},
	{ PARAM_STRING,	"help_std",			{ NULL },	{ "etc/help.std" },				},
	{ PARAM_STRING,	"help_config",		{ NULL },	{ "etc/help.config" },			},
	{ PARAM_STRING,	"help_roomconfig",	{ NULL },	{ "etc/help.roomconfig" },		},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"help_sysop",		{ NULL },	{ "etc/help.sysop" },			},

	{ PARAM_STRING, "hostmap_file",		{ NULL },	{ "etc/hostmap" },				},
	{ PARAM_STRING, "hosts_access_file",{ NULL },	{ "etc/hosts_access" },			},
	{ PARAM_STRING,	"banished_file",	{ NULL },	{ "etc/banished" },				},
	{ PARAM_STRING,	"stat_file",		{ NULL },	{ "etc/stats" },				},
	{ PARAM_STRING,	"su_passwd_file",	{ NULL },	{ "etc/su_passwd" },			},
	{ PARAM_STRING,	"pid_file",			{ NULL },	{ "etc/pid" },					},
	{ PARAM_STRING,	"symtab_file",		{ NULL },	{ "etc/symtab" },				},
	{ PARAM_STRING, "default_timezone",	{ NULL },	{ "Europe/Amsterdam" },			},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"default_language",	{ NULL },	{ "bbs100" },					},

	{ PARAM_STRING, "syslog",			{ NULL },	{ "log/bbslog" },				},
	{ PARAM_STRING, "authlog",			{ NULL },	{ "log/authlog" },				},
	{ PARAM_STRING, "logrotate",		{ NULL },	{ "daily" },					},
	{ PARAM_STRING, "archivedir",		{ NULL },	{ "log/archive/" },				},
	{ PARAM_STRING, "oncrash",			{ NULL },	{ "dumpcore" },					},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"crashdir",			{ NULL },	{ "log/crash/" },				},

	{ PARAM_INT,	"max_cached",		{ NULL },	{ (char *)DEFAULT_MAX_CACHED },		},
	{ PARAM_INT,	"max_messages",		{ NULL },	{ (char *)DEFAULT_MAX_MESSAGES },	},
	{ PARAM_INT,	"max_mail_msgs",	{ NULL },	{ (char *)DEFAULT_MAX_MAIL_MSGS },	},
	{ PARAM_INT,	"max_msg_lines",	{ NULL },	{ (char *)DEFAULT_MAX_MSG_LINES },	},
	{ PARAM_INT,	"max_xmsg_lines",	{ NULL },	{ (char *)DEFAULT_MAX_XMSG_LINES },	},
	{ PARAM_INT,	"max_history",		{ NULL },	{ (char *)DEFAULT_MAX_HISTORY },	},
	{ PARAM_INT,	"max_chat_history",	{ NULL },	{ (char *)DEFAULT_MAX_CHAT_HISTORY },	},
	{ PARAM_INT,	"max_friend",		{ NULL },	{ (char *)DEFAULT_MAX_FRIEND },		},
	{ PARAM_INT,	"max_enemy",		{ NULL },	{ (char *)DEFAULT_MAX_ENEMY },		},
	{ PARAM_INT,	"idle_timeout",		{ NULL },	{ (char *)DEFAULT_IDLE_TIMEOUT },	},
	{ PARAM_INT,	"lock_timeout",		{ NULL },	{ (char *)DEFAULT_LOCK_TIMEOUT },	},
	{ PARAM_INT,	"periodic_saving",	{ NULL },	{ (char *)DEFAULT_SAVE_TIMEOUT },	},
	{ PARAM_INT | PARAM_SEPARATOR,
					"cache_expire",		{ NULL },	{ (char *)DEFAULT_CACHE_TIMEOUT },	},

	{ PARAM_STRING,	"name_sysop",		{ NULL },	{ "Sysop" },						},
	{ PARAM_STRING,	"name_room_aide",	{ NULL },	{ "Room Aide" },					},
	{ PARAM_STRING,	"name_helper",		{ NULL },	{ "Helping Hand" },					},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"name_guest",		{ NULL },	{ "Guest" },						},

	{ PARAM_STRING,	"notify_login",		{ NULL },	{ "is formed from some <yellow>golden<magenta> stardust" },	},
	{ PARAM_STRING,	"notify_logout",	{ NULL },	{ "explodes into <yellow>golden<magenta> stardust" },			},
	{ PARAM_STRING, "notify_linkdead",	{ NULL },	{ "freezes up and crumbles to dust" },			},
	{ PARAM_STRING,	"notify_idle",		{ NULL },	{ "has been logged off due to inactivity" },	},
	{ PARAM_STRING,	"notify_locked",	{ NULL },	{ "is away from the terminal for a while" },	},
	{ PARAM_STRING,	"notify_unlocked",	{ NULL },	{ "has returned to the terminal" },				},
	{ PARAM_STRING, "notify_enter_chat",{ NULL },	{ "enters" },									},
	{ PARAM_STRING | PARAM_SEPARATOR,
					"notify_leave_chat",{ NULL },	{ "leaves" },									},

	{ PARAM_BOOL,	"have_xmsgs",		{ NULL },	{ (char *)-1 }, },
	{ PARAM_BOOL,	"have_emotes",		{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_feelings",	{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_questions",	{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_quick_x",		{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_talkedto",	{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_hold_msgs",	{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_followup",	{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_x_reply",		{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_calendar",	{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_worldclock",	{ NULL },	{ (char *)-1 },	},
	{ PARAM_BOOL,	"have_chatrooms",	{ NULL },	{ (char *)-1 }, },
	{ PARAM_BOOL,	"have_homeroom",	{ NULL },	{ (char *)-1 }, },
	{ PARAM_BOOL,	"have_mailroom",	{ NULL },	{ (char *)-1 }, },
	{ PARAM_BOOL,	"disabled_msg",		{ NULL },	{ (char *)-1 },	},
};


int init_Param(void) {
int i, num;

	if (param == NULL)
		return -1;

	num = sizeof(param)/sizeof(Param);

	for(i = 0; i < num; i++) {
		switch(param[i].type & PARAM_MASK) {
			case PARAM_STRING:
				Free(param[i].val.s);
				param[i].val.s = cstrdup(param[i].default_val.s);
				break;

			case PARAM_INT:
				param[i].val.d = param[i].default_val.d;
				break;

			case PARAM_UINT:
				param[i].val.u = param[i].default_val.u;
				break;

			case PARAM_LONG:
				param[i].val.l = param[i].default_val.l;
				break;

			case PARAM_ULONG:
				param[i].val.ul = param[i].default_val.ul;
				break;

			case PARAM_BOOL:
				param[i].val.bool = param[i].default_val.bool;
				break;

			default:
				fprintf(stderr, "init_Param: unknown type '%d' for param[%d] (%s)\n", param[i].type, i, param[i].var);
				return -1;
		}
	}
	return 0;
}


int load_Param(char *filename) {
AtomicFile *f;
char buf[PRINT_BUF], *p;
int i, num;

	if (param == NULL)
		return -1;

	if ((f = openfile(filename, "r")) == NULL)
		return 1;

	num = sizeof(param)/sizeof(Param);

	while(fgets(buf, PRINT_BUF, f->f) != NULL) {
		cstrip_line(buf);
		cstrip_spaces(buf);
		if (!*buf || *buf == '#')
			continue;

		if ((p = cstrchr(buf, ' ')) != NULL) {
			*p = 0;
			p++;
			if (!*p)
				continue;

			for(i = 0; i < num; i++) {
				if (!cstricmp(buf, param[i].var)) {
					switch(param[i].type & PARAM_MASK) {
						case PARAM_STRING:
							Free(param[i].val.s);
							param[i].val.s = cstrdup(p);
							break;

						case PARAM_INT:
							param[i].val.d = strtoul(p, NULL, 10);
							break;

						case PARAM_UINT:
							param[i].val.u = (unsigned int)strtoul(p, NULL, 10);
							break;

						case PARAM_LONG:
							param[i].val.l = (long)strtoul(p, NULL, 10);
							break;

						case PARAM_ULONG:
							param[i].val.ul = strtoul(p, NULL, 10);
							break;

						case PARAM_BOOL:
							if (!cstricmp(p, "yes") || !cstricmp(p, "on") || !cstricmp(p, "true") || !strcmp(p, "1"))
								param[i].val.bool = PARAM_TRUE;
							else
								if (!cstricmp(p, "no") || !cstricmp(p, "off") || !cstricmp(p, "false") || !strcmp(p, "0"))
									param[i].val.bool = PARAM_FALSE;
								else {
									fprintf(stderr, "load_Param: unknown value '%s' for param %s\n", p, param[i].var);
									param[i].val.bool = param[i].default_val.bool;
								}
							break;

						default:
							closefile(f);
							fprintf(stderr, "load_Param: unknown type '%d' for param[%d] (%s)\n", param[i].type, i, param[i].var);
							return -1;
					}
					break;
				}
			}
		}
	}
	closefile(f);
	return 0;
}

int save_Param(char *filename) {
AtomicFile *f;
int i, num;

	if ((f = openfile(filename, "w")) == NULL)
		return -1;

	fprintf(f->f, "#\n"
		"# %s param file %s\n"
		"#\n"
		"\n", PARAM_BBS_NAME, filename);

	num = sizeof(param)/sizeof(Param);

	for(i = 0; i < num; i++) {
		switch(param[i].type & PARAM_MASK) {
			case PARAM_STRING:
				fprintf(f->f, "%-22s %s\n", param[i].var, param[i].val.s);
				break;

			case PARAM_INT:
				fprintf(f->f, "%-22s %d\n", param[i].var, param[i].val.d);
				break;

			case PARAM_UINT:
				fprintf(f->f, "%-22s %u\n", param[i].var, param[i].val.u);
				break;

			case PARAM_LONG:
				fprintf(f->f, "%-22s %ld\n", param[i].var, param[i].val.ld);
				break;

			case PARAM_ULONG:
				fprintf(f->f, "%-22s %lu\n", param[i].var, param[i].val.lu);
				break;

			case PARAM_BOOL:
				fprintf(f->f, "%-22s %s\n", param[i].var, (param[i].val.bool == PARAM_FALSE) ? "no" : "yes");
				break;

			default:
				fclose(f->f);
				destroy_AtomicFile(f);
				fprintf(stderr, "save_Param: unknown type '%d' for param[%d] (%s)\n", param[i].type, i, param[i].var);
				return -1;
		}
		if (param[i].type & PARAM_SEPARATOR)
			fprintf(f->f, "\n");
	}
	fprintf(f->f, "\n# EOB\n");
	closefile(f);
	return 0;
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

	PARAM_CHECK("port_number",		PARAM_PORT_NUMBER,		DEFAULT_PORT_1234);
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
int i, num;
char buf[MAX_PATHLEN];

	num = sizeof(param)/sizeof(Param);

	for(i = 0; i < num; i++) {
		strcpy(buf, param[i].var);
		strcat(buf, "    ");

		switch(param[i].type & PARAM_MASK) {
			case PARAM_STRING:
				printf(" %-22s %s\n", buf, param[i].val.s);
				break;

			case PARAM_INT:
				printf(" %-22s %d\n", buf, param[i].val.d);
				break;

			case PARAM_UINT:
				printf(" %-22s %u\n", buf, param[i].val.u);
				break;

			case PARAM_LONG:
				printf(" %-22s %ld\n", buf, param[i].val.ld);
				break;

			case PARAM_ULONG:
				printf(" %-22s %lu\n", buf, param[i].val.lu);
				break;

			case PARAM_BOOL:
				printf(" %-22s %s\n", param[i].var, (param[i].val.bool == PARAM_FALSE) ? "no" : "yes");
				break;

			default:
				fprintf(stderr, "print_Param: unknown type '%d' for param[%d] (%s)\n", param[i].type, i, param[i].var);
		}
	}
	printf("\n");
}

/* EOB */
