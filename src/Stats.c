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
	Stats.c	WJ99
*/

#include "config.h"
#include "debug.h"
#include "Stats.h"
#include "state_msg.h"
#include "copyright.h"
#include "User.h"
#include "cstring.h"
#include "util.h"
#include "strtoul.h"
#include "copyright.h"
#include "timeout.h"
#include "access.h"
#include "Param.h"
#include "Memory.h"
#include "CachedFile.h"
#include "AtomicFile.h"

#include <stdio.h>
#include <stdlib.h>

Stats stats;


int load_Stats(Stats *st, char *filename) {
AtomicFile *f;
char buf[MAX_LINE];

	if (st == NULL || filename == NULL)
		return -1;

	memset(st, 0, sizeof(Stats));

	if ((f = openfile(filename, "r")) == NULL)
		return -1;

/* oldest */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->oldest, buf, MAX_NAME);
	st->oldest[MAX_NAME-1] = 0;

/* youngest */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->youngest, buf, MAX_NAME);
	st->youngest[MAX_NAME-1] = 0;

/* most_logins */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_logins, buf, MAX_NAME);
	st->most_logins[MAX_NAME-1] = 0;

/* most_xsent */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_xsent, buf, MAX_NAME);
	st->most_xsent[MAX_NAME-1] = 0;

/* most_xrecv */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_xrecv, buf, MAX_NAME);
	st->most_xrecv[MAX_NAME-1] = 0;

/* most_esent */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_esent, buf, MAX_NAME);
	st->most_esent[MAX_NAME-1] = 0;

/* most_erecv */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_erecv, buf, MAX_NAME);
	st->most_erecv[MAX_NAME-1] = 0;

/* most_posted */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_posted, buf, MAX_NAME);
	st->most_posted[MAX_NAME-1] = 0;

/* most_read */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_read, buf, MAX_NAME);
	st->most_read[MAX_NAME-1] = 0;


/* oldest_birth */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->oldest_birth = strtoul(buf, NULL, 10);

/* youngest_birth */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->youngest_birth = strtoul(buf, NULL, 10);

/* logins */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->logins = strtoul(buf, NULL, 10);

/* xsent */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->xsent = strtoul(buf, NULL, 10);

/* xrecv */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->xrecv = strtoul(buf, NULL, 10);

/* esent */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->esent = strtoul(buf, NULL, 10);

/* erecv */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->erecv = strtoul(buf, NULL, 10);

/* posted */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->posted = strtoul(buf, NULL, 10);

/* read */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->read = strtoul(buf, NULL, 10);

/* oldest_age */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->oldest_age = strtoul(buf, NULL, 10);

/*
	stats for Feelings, as suggested by many, finally present in 1.1.6 and up
*/
/* most_fsent */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_fsent, buf, MAX_NAME);
	st->most_fsent[MAX_NAME-1] = 0;

/* most_frecv */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_frecv, buf, MAX_NAME);
	st->most_frecv[MAX_NAME-1] = 0;

/* fsent */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	st->fsent = strtoul(buf, NULL, 10);

/* frecv */
	if (fgets(buf, MAX_LINE, f->f) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	st->frecv = strtoul(buf, NULL, 10);

end_load_Stats:
	closefile(f);
	return 0;

err_load_Stats:
	closefile(f);
	memset(st, 0, sizeof(Stats));
	return -1;
}

int save_Stats(Stats *st, char *filename) {
AtomicFile *f;

	if (st == NULL || filename == NULL || (f = openfile(filename, "w")) == NULL)
		return -1;

	fprintf(f->f, "%s\n%s\n%s\n%s\n%s\n", st->oldest, st->youngest, st->most_logins, st->most_xsent, st->most_xrecv);
	fprintf(f->f, "%s\n%s\n%s\n%s\n", st->most_esent, st->most_erecv, st->most_posted, st->most_read);

	fprintf(f->f, "%lu\n%lu\n%lu\n%lu\n%lu\n", (unsigned long)st->oldest_birth, (unsigned long)st->youngest_birth, st->logins, st->xsent, st->xrecv);
	fprintf(f->f, "%lu\n%lu\n%lu\n%lu\n%lu\n", st->esent, st->erecv, st->posted, st->read, st->oldest_age);

	fprintf(f->f, "%s\n%s\n", st->most_fsent, st->most_frecv);
	fprintf(f->f, "%lu\n%lu\n", st->fsent, st->frecv);

	closefile(f);
	return 0;
}


void update_stats(User *usr) {
int updated = 0;

	if (usr == NULL)
		return;

	if (usr->online_timer < rtc)		/* bug prevention :P (?) */
		usr->total_time += (rtc - usr->online_timer);
	usr->online_timer = rtc;

	if (is_guest(usr->name))
		return;

	if (usr->total_time >= stats.oldest_age) {
		stats.oldest_age = usr->total_time;
		stats.oldest_birth = usr->birth;
		strcpy(stats.oldest, usr->name);
		updated++;
	}
	if (usr->logins >= stats.logins) {
		stats.logins = usr->logins;
		strcpy(stats.most_logins, usr->name);
		updated++;
	}
	if (usr->xsent >= stats.xsent) {
		stats.xsent = usr->xsent;
		strcpy(stats.most_xsent, usr->name);
		updated++;
	}
	if (usr->xrecv >= stats.xrecv) {
		stats.xrecv = usr->xrecv;
		strcpy(stats.most_xrecv, usr->name);
		updated++;
	}
	if (usr->esent >= stats.esent) {
		stats.esent = usr->esent;
		strcpy(stats.most_esent, usr->name);
		updated++;
	}
	if (usr->erecv >= stats.erecv) {
		stats.erecv = usr->erecv;
		strcpy(stats.most_erecv, usr->name);
		updated++;
	}
	if (usr->fsent >= stats.fsent) {
		stats.fsent = usr->fsent;
		strcpy(stats.most_fsent, usr->name);
		updated++;
	}
	if (usr->frecv >= stats.frecv) {
		stats.frecv = usr->frecv;
		strcpy(stats.most_frecv, usr->name);
		updated++;
	}
	if (usr->posted >= stats.posted) {
		stats.posted = usr->posted;
		strcpy(stats.most_posted, usr->name);
		updated++;
	}
	if (usr->read >= stats.read) {
		stats.read = usr->read;
		strcpy(stats.most_read, usr->name);
		updated++;
	}
	if (updated && save_Stats(&stats, PARAM_STAT_FILE)) {
		Perror(usr, "Failed to save stats");
	}
}

void print_stats(User *usr) {
char buf[PRINT_BUF], *p;
int l, w;
unsigned long num;

	if (usr == NULL)
		return;

	Enter(print_stats);

	update_stats(usr);

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

	sprintf(buf, "<yellow>This is <white>%s<yellow>, %s", PARAM_BBS_NAME,
		print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL));
	cstrip_line(buf);

/* kludge for newlines :P */
	if ((p = cstrchr(buf, '\n')) != NULL) {
		*p = 0;

		usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));
		strcpy(buf, print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL));
		if ((p = cstrchr(buf, '\n')) != NULL) {
			p++;
			strcpy(buf, p);
			cstrip_line(buf);
		}
	}
	usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));

	usr->more_text = add_String(&usr->more_text,
		"<green>The system was last booted on <cyan>%s", print_date(usr, stats.uptime));
	usr->more_text = add_String(&usr->more_text,
		"<green>Uptime is <yellow>%s", print_total_time(rtc - stats.uptime));
	usr->more_text = add_String(&usr->more_text,
		"<yellow>%s<green> successful login%s made since boot time",
		print_number(stats.num_logins), (stats.num_logins == 1UL) ? "" : "s");

	if (reboot_timer != NULL)
		usr->more_text = add_StringList(&usr->more_text, new_StringList("<red>The system is rebooting"));
	if (shutdown_timer != NULL)
		usr->more_text = add_StringList(&usr->more_text, new_StringList("<red>The system is shutting down"));

	if (usr->runtime_flags & RTF_SYSOP) {
		usr->more_text = add_StringList(&usr->more_text, new_StringList(""));

		l =  sprintf(buf,   "<green>Cache size: <yellow>%s", print_number(cache_size));
		l += sprintf(buf+l, "<white>/<yellow>%s<green>   ", print_number(num_cached));
		l += sprintf(buf+l, "hits: <yellow>%s<green>   ", print_number(stats.cache_hit));
		l += sprintf(buf+l, "misses: <yellow>%s<green>   ", print_number(stats.cache_miss));
		if ((stats.cache_hit + stats.cache_miss) > 0)
			sprintf(buf+l, "rate: <yellow>%lu<white>%%", 100UL * stats.cache_hit / (stats.cache_hit + stats.cache_miss));
		else
			sprintf(buf+l, "rate: <yellow>%lu<white>%%", 0UL);
		usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));

		usr->more_text = add_String(&usr->more_text, "<green>Total memory in use: <yellow>%s <green>bytes", print_number(memory_total));
	}
	usr->more_text = add_StringList(&usr->more_text, new_StringList(""));
	usr->more_text = add_StringList(&usr->more_text, new_StringList("<yellow>User statistics"));

	usr->more_text = add_String(&usr->more_text, "<green>Youngest user is <white>%s<green>, created on <cyan>%s<green>", stats.youngest, print_date(usr, stats.youngest_birth));
	usr->more_text = add_String(&usr->more_text, "Oldest user is <white>%s<green>,", stats.oldest);
	usr->more_text = add_String(&usr->more_text, "online for <yellow>%s<green>", print_total_time(stats.oldest_age));
	usr->more_text = add_StringList(&usr->more_text, new_StringList(""));

/*
	determine width of next block of text
	I like to pretty-print this screen...
*/
	w = strlen(stats.most_logins);
	if ((l = strlen(stats.most_xsent)) > w)
		w = l;
	if ((l = strlen(stats.most_xrecv)) > w)
		w = l;
	if ((l = strlen(stats.most_esent)) > w)
		w = l;
	if ((l = strlen(stats.most_erecv)) > w)
		w = l;
	if ((l = strlen(stats.most_fsent)) > w)
		w = l;
	if ((l = strlen(stats.most_frecv)) > w)
		w = l;
	if ((l = strlen(stats.most_posted)) > w)
		w = l;
	if ((l = strlen(stats.most_read)) > w)
		w = l;

	usr->more_text = add_String(&usr->more_text, "Most logins are by                     <white>%-*s<green>: <yellow>%s<green>", w, stats.most_logins, print_number(stats.logins));
	usr->more_text = add_String(&usr->more_text, "Most eXpress Messages were sent by     <white>%-*s<green>: <yellow>%s<green>", w, stats.most_xsent, print_number(stats.xsent));
	usr->more_text = add_String(&usr->more_text, "Most eXpress Messages were received by <white>%-*s<green>: <yellow>%s<green>", w, stats.most_xrecv, print_number(stats.xrecv));
	usr->more_text = add_String(&usr->more_text, "Most emotes were sent by               <white>%-*s<green>: <yellow>%s<green>", w, stats.most_esent, print_number(stats.esent));
	usr->more_text = add_String(&usr->more_text, "Most emotes were received by           <white>%-*s<green>: <yellow>%s<green>", w, stats.most_erecv, print_number(stats.erecv));
	usr->more_text = add_String(&usr->more_text, "Most Feelings were sent by             <white>%-*s<green>: <yellow>%s<green>", w, stats.most_fsent, print_number(stats.fsent));
	usr->more_text = add_String(&usr->more_text, "Most Feelings were received by         <white>%-*s<green>: <yellow>%s<green>", w, stats.most_frecv, print_number(stats.frecv));
	usr->more_text = add_String(&usr->more_text, "Most messages were posted by           <white>%-*s<green>: <yellow>%s<green>", w, stats.most_posted, print_number(stats.posted));
	usr->more_text = add_String(&usr->more_text, "Most messages were read by             <white>%-*s<green>: <yellow>%s<green>", w, stats.most_read, print_number(stats.read));

	if (!is_guest(usr->name)) {
		usr->more_text = add_StringList(&usr->more_text, new_StringList(""));
		usr->more_text = add_StringList(&usr->more_text, new_StringList("<yellow>Your statistics"));

		l  = sprintf(buf, "<green>eXpress Messages sent: <yellow>%-15s", print_number(usr->xsent));
		sprintf(buf+l, "<green> received: <yellow>%s", print_number(usr->xrecv));
		usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));

		l = sprintf(buf, "<green>Emotes sent          : <yellow>%-15s", print_number(usr->esent));
		sprintf(buf+l, "<green> received: <yellow>%s", print_number(usr->erecv));
		usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));

		l = sprintf(buf, "<green>Feelings sent        : <yellow>%-15s", print_number(usr->fsent));
		sprintf(buf+l, "<green> received: <yellow>%s", print_number(usr->frecv));
		usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));

		l = sprintf(buf, "<green>Messages posted      : <yellow>%-15s", print_number(usr->posted));
		sprintf(buf+l, "<green> read    : <yellow>%s", print_number(usr->read));
		usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));

		usr->more_text = add_StringList(&usr->more_text, new_StringList(""));

		usr->more_text = add_String(&usr->more_text, "<green>Account created on <cyan>%s<green>", print_date(usr, usr->birth));
		l = sprintf(buf, "You have logged on <yellow>%s<green> times, ", print_number(usr->logins));

		num = (unsigned long)((rtc - usr->birth) / (30 * 24 * 3600UL));
		if (num == 0UL)
			num = 1UL;
		num = usr->logins / num;

		sprintf(buf+l, "an average of <yellow>%s<green> time%s per month", print_number(num), (num == 1UL) ? "" : "s");
		usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));
		usr->more_text = add_String(&usr->more_text, "Your total online time is <yellow>%s", print_total_time(usr->total_time));
	}
	read_more(usr);
	Return;
}

/* EOB */
