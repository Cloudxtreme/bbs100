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
	readlogs	WJ99
*/

#include "copyright.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define OPT_NO_LOG		1		/* don't display log name */
#define OPT_NO_DATE		2		/* don't display date */
#define OPT_NO_TIME		4		/* don't display time */
#define OPT_NOT_PRETTY	8		/* no pretty date/time formatting */

#define DATE_LEN		10
#define TIME_LEN		8
#define DATE_TIME_LEN	20

typedef struct {
	char name[32], buf[256];
	FILE *f;
} Logfile;

Logfile log[4] = {
	{ "auth",		"", NULL },
	{ "messages",	"", NULL },
	{ "errors",		"", NULL },
	{ "debug",		"", NULL }
};


int main(int argc, char **argv) {
int i, smallest, num, day, flags, len, do_time = 0;
char buf[256], timebuf[DATE_TIME_LEN+1] = "", *p;
time_t t;
struct tm *tm;

	printf("%s", print_copyright(SHORT, "readlogs"));

	t = time(NULL);
	tm = gmtime(&t);
	day = tm->tm_mday;

	flags = 0;

	for(i = 1; i < argc; i++) {
		for(p = argv[i]; *p; p++) {
			if (*p >= '0' && *p <= '9') {
				day = atoi(argv[i]);
				if (day < 1 || day > 31) {
					printf("Invalid day number\n");
					return -1;
				}
				break;
			} else {
				switch(*p) {
					case '-':
						break;

					case 'l':
						flags |= OPT_NO_LOG;
						break;

					case 'd':
						flags |= OPT_NO_DATE;
						break;

					case 't':
						flags |= OPT_NO_TIME;
						break;

					case 'p':
						flags |= OPT_NOT_PRETTY;
						break;

					case 'h':
					case '?':
						printf("usage: readlogs [day number] [options]\n"
							"Options:\n"
							"  -h    Help; display this information\n"
							"  -v    Display version information\n"
							"  -l    Do not display log name\n"
							"  -d    Do not display date (excludes -p)\n"
							"  -t    Do not display time (excludes -p)\n"
							"  -p    Do not pretty format date and time\n"
						);
					case 'v':
						return 1;

					default:
						printf("Unknown option '%c'\n", *p);
						return -1;
				}
			}
		}
	}
	num = sizeof(log)/sizeof(Logfile);
	for(i = 0; i < num; i++) {
		sprintf(buf, "log/%s/%s.%02d", log[i].name, log[i].name, day);

		if ((log[i].f = fopen(buf, "r")) == NULL) {
			printf("failed to open %s, continuing\n", buf);
			continue;
		}
		if (fgets(log[i].buf, 256, log[i].f) == NULL) {
			fclose(log[i].f);
			log[i].f = NULL;
		}
	}
	for(;;) {
		for(smallest = 0; smallest < num; smallest++) {
			if (log[smallest].f != NULL)
				break;
		}
		if (smallest >= num)
			break;

		for(i = smallest+1; i < num; i++)
			if (log[i].f != NULL && strcmp(log[smallest].buf, log[i].buf) > 0)
				smallest = i;


		if (strlen(log[smallest].buf) > (DATE_TIME_LEN+1)) {
			if (flags & OPT_NO_LOG) {
				*buf = 0;
				len = 0;
			} else {
				sprintf(buf, "%8s", log[smallest].name);
				len = strlen(buf);
			}
			if (!(flags & OPT_NO_DATE)) {
				if (len) {
					buf[len++] = ' ';
					buf[len] = 0;
				}
				strncpy(buf+len, log[smallest].buf, DATE_LEN);

				if (!(flags & OPT_NOT_PRETTY)) {
					if (!strncmp(timebuf, log[smallest].buf, DATE_TIME_LEN)) {
						for(i = 0; i < DATE_LEN; i++)
							buf[len+i] = ' ';
						do_time = 0;
					} else {
						strncpy(timebuf, log[smallest].buf, DATE_TIME_LEN);
						timebuf[DATE_TIME_LEN] = 0;
						do_time = 1;
					}
				}
				len += DATE_LEN;
				buf[len] = 0;
			}
			if (!(flags & OPT_NO_TIME)) {
				if (len) {
					buf[len++] = ' ';
					buf[len] = 0;
				}
				strncpy(buf+len, log[smallest].buf+DATE_LEN+1, TIME_LEN);

				if (!(flags & OPT_NOT_PRETTY)) {
					if (do_time || strncmp(timebuf, log[smallest].buf, DATE_TIME_LEN)) {
						strncpy(timebuf, log[smallest].buf, DATE_TIME_LEN);
						timebuf[DATE_TIME_LEN] = 0;
					} else {
						for(i = 0; i < TIME_LEN; i++)
							buf[len+i] = ' ';
					}
					do_time = 0;
				}
				len += TIME_LEN;
				buf[len] = 0;
			}
			if (len) {
				buf[len++] = ' ';
				buf[len] = 0;
			}
			printf("%s%s", buf, log[smallest].buf + DATE_TIME_LEN);
		}
		if (fgets(log[smallest].buf, 256, log[smallest].f) == NULL) {
			fclose(log[smallest].f);
			log[smallest].f = NULL;
		}
	}
	return 0;
}

/* EOB */
