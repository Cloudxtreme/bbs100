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
	mkpasswd.c	WJ99
*/

#include "config.h"
#include "copyright.h"
#include "passwd.h"
#include "sys_time.h"
#include "cstring.h"
#include "memset.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <termios.h>

time_t rtc = (time_t)0UL;

void getpassword(char *prompt, char *buf, int maxlen) {
struct termios term, saved_term;
int c, n;

	if (buf == NULL || maxlen <= 1)
		return;

	tcgetattr(0, &term);
	tcgetattr(0, &saved_term);

	term.c_iflag |= ISTRIP;
	term.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;

	tcsetattr(0, TCSANOW, &term);

	if (prompt != NULL) {
		printf("%s", prompt);
		fflush(stdout);
	}
	memset(buf, 0, maxlen);

/*
	simple input routine, as fgets(stdin) really didn't do what I expected ...
*/
	n = 0;
	while((c = fgetc(stdin)) != EOF) {
		if (c == 0x7f || c == '\b') {
			if (n > 0) {
				buf[--n] = 0;
				printf("\b \b");
				fflush(stdout);
			}
			continue;
		}
		if (c == '\n' || c == '\r')
			break;

		if (n < maxlen) {
			buf[n++] = c;
			printf("*");
			fflush(stdout);
		}
	}
	buf[n] = 0;
/*
	if (fgets(buf, maxlen, stdin) == NULL)
		*buf = 0;
*/
	buf[maxlen-1] = 0;

	tcsetattr(0, TCSANOW, &saved_term);

	printf("\n");
}

int main(void) {
char buf[MAX_LONGLINE], buf2[MAX_LINE], crypt_buf[MAX_CRYPTED];

	printf("%s", print_copyright(SHORT, "mkpasswd", buf, MAX_LONGLINE));

	getpassword("Enter password: ", buf, MAX_LINE);
	if (!*buf)
		exit(1);
	buf[MAX_LINE-1] = 0;

	getpassword("Enter it again (for verification): ", buf2, MAX_LINE);
	if (!*buf2)
		exit(1);
	buf2[MAX_LINE-1] = 0;

	if (strcmp(buf, buf2)) {
		printf("Passwords didn't match\n");
		exit(-1);
	}
	rtc = time(NULL);
	init_crypt();

	printf("%s\n", crypt_phrase(buf, crypt_buf));

	if (verify_phrase(buf, crypt_buf))
		printf("\nERROR: verify_phrase() failed; passphrase encryption is NOT working correctly\n\n");

	exit(0);
	return 0;
}

/* EOB */
