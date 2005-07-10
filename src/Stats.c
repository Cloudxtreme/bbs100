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
#include "FileFormat.h"

#include <stdio.h>
#include <stdlib.h>

Stats stats;


int load_Stats(Stats *st, char *filename) {
File *f;
int (*load_func)(File *, Stats *) = NULL;
int version;

	if (st == NULL || filename == NULL)
		return -1;

	memset(st, 0, sizeof(Stats));

	if ((f = Fopen(filename)) == NULL)
		return -1;

	version = fileformat_version(f);
	switch(version) {
		case -1:
			log_err("load_Stats(): error trying to determine file format version of %s", filename);
			load_func = NULL;
			break;

		case 0:
			Frewind(f);
			load_func = load_Stats_version0;
			break;

		case 1:
			load_func = load_Stats_version1;
			break;

		default:
			log_err("load_Stats(): don't know how to load version %d of %s", version, filename);
	}
	if (load_func != NULL && !load_func(f, st)) {
		Fclose(f);
		return 0;
	}
	Fclose(f);
	memset(st, 0, sizeof(Stats));
	return -1;
}


int load_Stats_version1(File *f, Stats *st) {
char buf[MAX_LINE], *p;

	while(Fgets(f, buf, MAX_LINE) != NULL) {
		FF1_PARSE;

		FF1_LOAD_LEN("oldest", st->oldest, MAX_NAME);
		FF1_LOAD_LEN("youngest", st->youngest, MAX_NAME);
		FF1_LOAD_LEN("most_logins", st->most_logins, MAX_NAME);
		FF1_LOAD_LEN("most_xsent", st->most_xsent, MAX_NAME);
		FF1_LOAD_LEN("most_xrecv", st->most_xrecv, MAX_NAME);
		FF1_LOAD_LEN("most_esent", st->most_esent, MAX_NAME);
		FF1_LOAD_LEN("most_erecv", st->most_erecv, MAX_NAME);
		FF1_LOAD_LEN("most_fsent", st->most_fsent, MAX_NAME);
		FF1_LOAD_LEN("most_frecv", st->most_frecv, MAX_NAME);
		FF1_LOAD_LEN("most_posted", st->most_posted, MAX_NAME);
		FF1_LOAD_LEN("most_read", st->most_read, MAX_NAME);

		FF1_LOAD_ULONG("oldest_birth", st->oldest_birth);
		FF1_LOAD_ULONG("oldest_age", st->oldest_age);
		FF1_LOAD_ULONG("youngest_birth", st->youngest_birth);
		FF1_LOAD_ULONG("logins", st->logins);
		FF1_LOAD_ULONG("xsent", st->xsent);
		FF1_LOAD_ULONG("xrecv", st->xrecv);
		FF1_LOAD_ULONG("esent", st->esent);
		FF1_LOAD_ULONG("erecv", st->erecv);
		FF1_LOAD_ULONG("fsent", st->fsent);
		FF1_LOAD_ULONG("frecv", st->frecv);
		FF1_LOAD_ULONG("posted", st->posted);
		FF1_LOAD_ULONG("read", st->read);

		FF1_LOAD_UNKNOWN;
	}
	return 0;
}

int load_Stats_version0(File *f, Stats *st) {
char buf[MAX_LINE];

/* oldest */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->oldest, buf, MAX_NAME);
	st->oldest[MAX_NAME-1] = 0;

/* youngest */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->youngest, buf, MAX_NAME);
	st->youngest[MAX_NAME-1] = 0;

/* most_logins */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_logins, buf, MAX_NAME);
	st->most_logins[MAX_NAME-1] = 0;

/* most_xsent */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_xsent, buf, MAX_NAME);
	st->most_xsent[MAX_NAME-1] = 0;

/* most_xrecv */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_xrecv, buf, MAX_NAME);
	st->most_xrecv[MAX_NAME-1] = 0;

/* most_esent */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_esent, buf, MAX_NAME);
	st->most_esent[MAX_NAME-1] = 0;

/* most_erecv */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_erecv, buf, MAX_NAME);
	st->most_erecv[MAX_NAME-1] = 0;

/* most_posted */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_posted, buf, MAX_NAME);
	st->most_posted[MAX_NAME-1] = 0;

/* most_read */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_read, buf, MAX_NAME);
	st->most_read[MAX_NAME-1] = 0;

/* oldest_birth */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->oldest_birth = strtoul(buf, NULL, 10);

/* youngest_birth */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->youngest_birth = strtoul(buf, NULL, 10);

/* logins */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->logins = strtoul(buf, NULL, 10);

/* xsent */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->xsent = strtoul(buf, NULL, 10);

/* xrecv */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->xrecv = strtoul(buf, NULL, 10);

/* esent */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->esent = strtoul(buf, NULL, 10);

/* erecv */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->erecv = strtoul(buf, NULL, 10);

/* posted */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->posted = strtoul(buf, NULL, 10);

/* read */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->read = strtoul(buf, NULL, 10);

/* oldest_age */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_Stats;

	cstrip_line(buf);
	st->oldest_age = strtoul(buf, NULL, 10);

/*
	stats for Feelings, as suggested by many, finally present in 1.1.6 and up
*/
/* most_fsent */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_fsent, buf, MAX_NAME);
	st->most_fsent[MAX_NAME-1] = 0;

/* most_frecv */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	strncpy(st->most_frecv, buf, MAX_NAME);
	st->most_frecv[MAX_NAME-1] = 0;

/* fsent */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	st->fsent = strtoul(buf, NULL, 10);

/* frecv */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto end_load_Stats;

	cstrip_line(buf);
	st->frecv = strtoul(buf, NULL, 10);

end_load_Stats:
	Fclose(f);
	return 0;

err_load_Stats:
	Fclose(f);
	memset(st, 0, sizeof(Stats));
	return -1;
}


int save_Stats(Stats *st, char *filename) {
int ret;
File *f;

	if (st == NULL || filename == NULL || (f = Fcreate(filename)) == NULL)
		return -1;

	ret = save_Stats_version1(f, st);
	Fclose(f);
	return ret;
}

int save_Stats_version1(File *f, Stats *st) {
	FF1_SAVE_VERSION;

	FF1_SAVE_STR("oldest", st->oldest);
	FF1_SAVE_STR("youngest", st->youngest);
	FF1_SAVE_STR("most_logins", st->most_logins);
	FF1_SAVE_STR("most_xsent", st->most_xsent);
	FF1_SAVE_STR("most_xrecv", st->most_xrecv);
	FF1_SAVE_STR("most_esent", st->most_esent);
	FF1_SAVE_STR("most_erecv", st->most_erecv);
	FF1_SAVE_STR("most_fsent", st->most_fsent);
	FF1_SAVE_STR("most_frecv", st->most_frecv);
	FF1_SAVE_STR("most_posted", st->most_posted);
	FF1_SAVE_STR("most_read", st->most_read);

	Fprintf(f, "oldest_birth=%lu", (unsigned long)st->oldest_birth);
	Fprintf(f, "oldest_age=%lu", st->oldest_age);
	Fprintf(f, "youngest_birth=%lu", (unsigned long)st->youngest_birth);
	Fprintf(f, "logins=%lu", st->logins);
	Fprintf(f, "xsent=%lu", st->xsent);
	Fprintf(f, "xrecv=%lu", st->xrecv);
	Fprintf(f, "esent=%lu", st->esent);
	Fprintf(f, "erecv=%lu", st->erecv);
	Fprintf(f, "fsent=%lu", st->fsent);
	Fprintf(f, "frecv=%lu", st->frecv);
	Fprintf(f, "posted=%lu", st->posted);
	Fprintf(f, "read=%lu", st->read);

	return 0;
}

int save_Stats_version0(File *f, Stats *st) {
	Fputs(f, st->oldest);
	Fputs(f, st->youngest);
	Fputs(f, st->most_logins);
	Fputs(f, st->most_xsent);
	Fputs(f, st->most_xrecv);
	Fputs(f, st->most_esent);
	Fputs(f, st->most_erecv);
	Fputs(f, st->most_posted);
	Fputs(f, st->most_read);

	Fprintf(f, "%lu", (unsigned long)st->oldest_birth);
	Fprintf(f, "%lu", (unsigned long)st->youngest_birth);
	Fprintf(f, "%lu", st->logins);
	Fprintf(f, "%lu", st->xsent);
	Fprintf(f, "%lu", st->xrecv);
	Fprintf(f, "%lu", st->esent);
	Fprintf(f, "%lu", st->erecv);
	Fprintf(f, "%lu", st->posted);
	Fprintf(f, "%lu", st->read);
	Fprintf(f, "%lu", st->oldest_age);

	Fputs(f, st->most_fsent);
	Fputs(f, st->most_frecv);

	Fprintf(f, "%lu", st->fsent);
	Fprintf(f, "%lu", st->frecv);
	return 0;
}


void update_stats(User *usr) {
int updated = 0;

	if (usr == NULL)
		return;

	if (!usr->online_timer)
		usr->online_timer = rtc;
	if (usr->online_timer < rtc)
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
char copyright_buf[2*MAX_LINE], date_buf[MAX_LINE];
int l, w;
unsigned long num;

	if (usr == NULL)
		return;

	Enter(print_stats);

	update_stats(usr);

	if (usr->text == NULL && (usr->text = new_StringIO()) == NULL) {
		Perror(usr, "Out of memory");
		Return;
	} else
		free_StringIO(usr->text);

	print_StringIO(usr->text, "<yellow>This is <white>%s<yellow>, %s", PARAM_BBS_NAME,
		print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL, copyright_buf));

	print_StringIO(usr->text, "<green>The system was last booted on <cyan>%s\n", print_date(usr, stats.uptime, date_buf));
	print_StringIO(usr->text, "<green>Uptime is <yellow>%s\n", print_total_time(rtc - stats.uptime, date_buf));
	print_StringIO(usr->text, "<yellow>%s<green> successful login%s made since boot time\n",
		print_number(stats.num_logins, date_buf), (stats.num_logins == 1UL) ? "" : "s");

	if (reboot_timer != NULL)
		put_StringIO(usr->text, "<red>The system is rebooting\n");
	if (shutdown_timer != NULL)
		put_StringIO(usr->text, "<red>The system is shutting down\n");

	if (usr->runtime_flags & RTF_SYSOP) {
		put_StringIO(usr->text, "\n");

		print_StringIO(usr->text, "<green>Cache size: <yellow>%s", print_number(cache_size, date_buf));
		print_StringIO(usr->text, "<white>/<yellow>%s<green>   ", print_number(num_cached, date_buf));
		print_StringIO(usr->text, "hits: <yellow>%s<green>   ", print_number(stats.cache_hit, date_buf));
		print_StringIO(usr->text, "misses: <yellow>%s<green>   ", print_number(stats.cache_miss, date_buf));
		if ((stats.cache_hit + stats.cache_miss) > 0)
			print_StringIO(usr->text, "rate: <yellow>%lu<white>%%", 100UL * stats.cache_hit / (stats.cache_hit + stats.cache_miss));
		else
			print_StringIO(usr->text, "rate: <yellow>%lu<white>%%", 0UL);

		print_StringIO(usr->text, "\n<green>Total memory in use: <yellow>%s <green>bytes\n", print_number(memory_total, date_buf));
	}
	print_StringIO(usr->text, "\n<yellow>User statistics\n");

	print_StringIO(usr->text, "<green>Youngest user is <white>%s<green>, created on <cyan>%s\n", stats.youngest, print_date(usr, stats.youngest_birth, date_buf));
	print_StringIO(usr->text, "<green>Oldest user is <white>%s<green>, online for <yellow>%s\n", stats.oldest, print_total_time(stats.oldest_age, date_buf));

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

	print_StringIO(usr->text, "\n<green>Most logins are by                     <white>%-*s<green> : <yellow>%s\n", w, stats.most_logins, print_number(stats.logins, date_buf));
	if (PARAM_HAVE_XMSGS) {
		print_StringIO(usr->text, "<green>Most eXpress Messages were sent by     <white>%-*s<green> : <yellow>%s\n", w, stats.most_xsent, print_number(stats.xsent, date_buf));
		print_StringIO(usr->text, "<green>Most eXpress Messages were received by <white>%-*s<green> : <yellow>%s\n", w, stats.most_xrecv, print_number(stats.xrecv, date_buf));
	}
	if (PARAM_HAVE_EMOTES) {
		print_StringIO(usr->text, "<green>Most emotes were sent by               <white>%-*s<green> : <yellow>%s\n", w, stats.most_esent, print_number(stats.esent, date_buf));
		print_StringIO(usr->text, "<green>Most emotes were received by           <white>%-*s<green> : <yellow>%s\n", w, stats.most_erecv, print_number(stats.erecv, date_buf));
	}
	if (PARAM_HAVE_FEELINGS) {
		print_StringIO(usr->text, "<green>Most Feelings were sent by             <white>%-*s<green> : <yellow>%s\n", w, stats.most_fsent, print_number(stats.fsent, date_buf));
		print_StringIO(usr->text, "<green>Most Feelings were received by         <white>%-*s<green> : <yellow>%s\n", w, stats.most_frecv, print_number(stats.frecv, date_buf));
	}
	print_StringIO(usr->text, "<green>Most messages were posted by           <white>%-*s<green> : <yellow>%s\n", w, stats.most_posted, print_number(stats.posted, date_buf));
	print_StringIO(usr->text, "<green>Most messages were read by             <white>%-*s<green> : <yellow>%s\n", w, stats.most_read, print_number(stats.read, date_buf));

	if (!is_guest(usr->name)) {
		put_StringIO(usr->text, "\n<yellow>Your statistics\n");

		if (PARAM_HAVE_XMSGS) {
			print_StringIO(usr->text, "<green>eXpress Messages sent: <yellow>%-15s", print_number(usr->xsent, date_buf));
			print_StringIO(usr->text, "<green> received: <yellow>%s\n", print_number(usr->xrecv, date_buf));
		}
		if (PARAM_HAVE_EMOTES) {
			print_StringIO(usr->text, "<green>Emotes sent          : <yellow>%-15s", print_number(usr->esent, date_buf));
			print_StringIO(usr->text, "<green> received: <yellow>%s\n", print_number(usr->erecv, date_buf));
		}
		if (PARAM_HAVE_FEELINGS) {
			print_StringIO(usr->text, "<green>Feelings sent        : <yellow>%-15s", print_number(usr->fsent, date_buf));
			print_StringIO(usr->text, "<green> received: <yellow>%s\n", print_number(usr->frecv, date_buf));
		}
		print_StringIO(usr->text, "<green>Messages posted      : <yellow>%-15s", print_number(usr->posted, date_buf));
		print_StringIO(usr->text, "<green> read    : <yellow>%s\n", print_number(usr->read, date_buf));

		print_StringIO(usr->text, "\n<green>Account created on <cyan>%s<green>\n", print_date(usr, usr->birth, date_buf));
		print_StringIO(usr->text, "You have logged on <yellow>%s<green> times, ", print_number(usr->logins, date_buf));

		num = (unsigned long)((rtc - usr->birth) / (unsigned long)(30 * SECS_IN_DAY));
		if (num == 0UL)
			num = 1UL;
		num = usr->logins / num;

		print_StringIO(usr->text, "an average of <yellow>%s<green> time%s per month\n", print_number(num, date_buf), (num == 1UL) ? "" : "s");
		print_StringIO(usr->text, "Your total online time is <yellow>%s\n", print_total_time(usr->total_time, date_buf));
	}
	read_text(usr);
	Return;
}

/* EOB */
