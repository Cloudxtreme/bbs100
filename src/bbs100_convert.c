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
	bbs100_convert.c	WJ103

	converts data files to new "version=1" format
*/

#include "config.h"
#include "defines.h"
#include "copyright.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

void strip_line(char *buf) {
int l;

	if (buf == NULL)
		return;

	l = strlen(buf)-1;
	while(l >= 0 && (buf[l] == '\n' || buf[l] == '\r'))
		buf[l--] = 0;
}

int is_numeric(char *buf) {
	while(*buf && *buf >= '0' && *buf <= '9')
		buf++;

	if (!*buf)
		return 1;

	return 0;
}


FILE *open_file(char *filename) {
FILE *f;
char buf[1024];

	if ((f = fopen(filename, "r")) == NULL) {
		printf("failed to open file %s\n", filename);
		return NULL;
	}
	if (fgets(buf, 1024, f) == NULL) {
		fclose(f);
		return NULL;
	}
	strip_line(buf);
	if (!strcmp(buf, "version=1")) {
		printf("skipping %s, already converted\n", filename);
		fclose(f);
		return NULL;
	}
	rewind(f);
	return f;
}

FILE *create_file(char *name) {
FILE *f;
char filename[MAX_PATHLEN];

	cstrcpy(filename, name, MAX_PATHLEN);
	cstrcat(filename, ".new", MAX_PATHLEN);

	if ((f = fopen(filename, "w")) == NULL) {
		printf("failed to create file %s\n", filename);
		return NULL;
	}
	fprintf(f, "version=1\n");
	return f;
}

void save_str(FILE *f, FILE *f2, char *keyword) {
char buf[512];

	if (fgets(buf, 512, f) == NULL)
		return;

	strip_line(buf);
	if (*buf)
		fprintf(f2, "%s=%s\n", keyword, buf);
}

void save_strlist(FILE *f, FILE *f2, char *keyword) {
char buf[512];

	while(fgets(buf, 512, f) != NULL) {
		strip_line(buf);
		if (!*buf)
			break;

		fprintf(f2, "%s=%s\n", keyword, buf);
	}
}

void rename_file(char *filename) {
char oldname[MAX_PATHLEN];

	cstrcpy(oldname, filename, MAX_PATHLEN);
	cstrcat(oldname, ".new", MAX_PATHLEN);
	if (rename(oldname, filename) == -1)
		printf("failed to rename %s to %s\n", oldname, filename);
}

void convert_userdata(char *filename) {
FILE *f, *f2;
char buf[512];
int i;

	printf("converting userdata %s\n", filename);

	if ((f = open_file(filename)) == NULL)
		return;

	if ((f2 = create_file(filename)) == NULL) {
		fclose(f);
		return;
	}
/*
	warning: no checks done at all
*/
/*	fprintf(f2, "name=%s\n", buf);	we don't really know the name here ... */

	save_str(f, f2, "passwd");
	save_str(f, f2, "real_name");
	save_str(f, f2, "street");
	save_str(f, f2, "zipcode");
	save_str(f, f2, "city");
	save_str(f, f2, "state");
	save_str(f, f2, "country");
	save_str(f, f2, "phone");
	save_str(f, f2, "email");
	save_str(f, f2, "www");
	save_str(f, f2, "doing");
	save_str(f, f2, "reminder");
	save_str(f, f2, "default_anon");
	save_str(f, f2, "from_ip");
	save_str(f, f2, "from_ipnum");
	save_str(f, f2, "birth");
	save_str(f, f2, "last_logout");
	save_str(f, f2, "flags");
	save_str(f, f2, "logins");
	save_str(f, f2, "total_time");
	save_str(f, f2, "xsent");
	save_str(f, f2, "xrecv");
	save_str(f, f2, "esent");
	save_str(f, f2, "erecv");
	save_str(f, f2, "posted");
	save_str(f, f2, "read");
	save_str(f, f2, "colors");

	save_strlist(f, f2, "rooms");

	for(i = 0; i < 10; i++) {
		fgets(buf, 512, f);
		strip_line(buf);
		if (*buf)
			fprintf(f2, "quick=%d %s\n", i, buf);
	}
	save_strlist(f, f2, "friends");
	save_strlist(f, f2, "enemies");
	save_strlist(f, f2, "info");

	fgets(buf, 512, f);			/* time displacement (deprecated) */

	save_str(f, f2, "fsent");
	save_str(f, f2, "frecv");

	fclose(f);
	fclose(f2);

	rename_file(filename);
}

void convert_roomdata(char *filename) {
FILE *f, *f2;

	printf("converting roomdata %s\n", filename);

	if ((f = open_file(filename)) == NULL)
		return;

	if ((f2 = create_file(filename)) == NULL) {
		fclose(f);
		return;
	}
	save_str(f, f2, "name");
	save_str(f, f2, "generation");
	save_str(f, f2, "flags");
	save_str(f, f2, "roominfo_changed");
	save_strlist(f, f2, "info");
	save_strlist(f, f2, "room_aides");
	save_strlist(f, f2, "invited");
	save_strlist(f, f2, "kicked");
	save_strlist(f, f2, "chat_history");

	fclose(f);
	fclose(f2);

	rename_file(filename);
}

void convert_message(char *filename) {
FILE *f, *f2;

	printf("converting message %s\n", filename);

	if ((f = open_file(filename)) == NULL)
		return;

	if ((f2 = create_file(filename)) == NULL) {
		fclose(f);
		return;
	}
	save_str(f, f2, "mtime");
	save_str(f, f2, "deleted");
	save_str(f, f2, "flags");
	save_str(f, f2, "from");
	save_str(f, f2, "anon");
	save_str(f, f2, "deleted_by");
	save_str(f, f2, "subject");
	save_strlist(f, f2, "to");
	save_strlist(f, f2, "msg");
	save_str(f, f2, "reply_number");

	fclose(f);
	fclose(f2);

	rename_file(filename);
}

void convert_statistics(char *filename) {
FILE *f, *f2;

	printf("converting statistics %s\n", filename);

	if ((f = open_file(filename)) == NULL)
		return;

	if ((f2 = create_file(filename)) == NULL) {
		fclose(f);
		return;
	}
	save_str(f, f2, "oldest");
	save_str(f, f2, "youngest");
	save_str(f, f2, "most_logins");
	save_str(f, f2, "most_xsent");
	save_str(f, f2, "most_xrecv");
	save_str(f, f2, "most_esent");
	save_str(f, f2, "most_erecv");
	save_str(f, f2, "most_posted");
	save_str(f, f2, "most_read");
	save_str(f, f2, "oldest_birth");
	save_str(f, f2, "youngest_birth");
	save_str(f, f2, "logins");
	save_str(f, f2, "xsent");
	save_str(f, f2, "xrecv");
	save_str(f, f2, "esent");
	save_str(f, f2, "erecv");
	save_str(f, f2, "posted");
	save_str(f, f2, "read");
	save_str(f, f2, "oldest_age");
	save_str(f, f2, "most_fsent");
	save_str(f, f2, "most_frecv");
	save_str(f, f2, "fsent");
	save_str(f, f2, "frecv");

	fclose(f);
	fclose(f2);

	rename_file(filename);
}


int main(int argc, char **argv) {
char buf[MAX_PATHLEN], *p;

	printf("%s", print_copyright(SHORT, "bbs100_convert", buf, MAX_PATHLEN));

	if (argc <= 1 || (argc > 1 && strcmp(argv[1], "--"))) {
		p = basename(argv[0]);
		printf("specify filenames on stdin\n"
			"typical usage is, from the bbs' basedir:\n"
			"\n");
		printf("find users -type f -print | %s --\n", p);
		printf("find rooms -type f -print | %s --\n", p);
		printf("echo etc/statistics | %s --\n\n", p);
		return 1;
	}
	while(fgets(buf, MAX_PATHLEN, stdin) != NULL) {
		strip_line(buf);
		if (!*buf)
			continue;

		if ((p = cstrrchr(buf, '/')) == NULL)
			p = buf;
		else {
			p++;
			if (!*p)
				continue;
		}
		if (!strcmp(p, "UserData")) {
			convert_userdata(buf);
			continue;
		}
		if (!strcmp(p, "HomeData")) {
			convert_roomdata(buf);
			continue;
		}
		if (!strcmp(p, "MailData")) {
			convert_roomdata(buf);
			continue;
		}
		if (!strcmp(p, "RoomData")) {
			convert_roomdata(buf);
			continue;
		}
		if (is_numeric(p)) {
			convert_message(buf);
			continue;
		}
		if (!strcmp(p, "stats")) {
			convert_statistics(buf);
			continue;
		}
		printf("skipping %s\n", buf);
	}
	return 0;
}

/* EOB */
