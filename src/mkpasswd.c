/*
    bbs100 3.0 WJ105
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
	mkpasswd.c	WJ99
*/

#include "config.h"
#include "copyright.h"
#include "passwd.h"
#include "sys_time.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <termios.h>

time_t rtc = (time_t)0UL;

void getpassword(char *prompt, char *buf, int maxlen) {
struct termios term, saved_term;

	if (buf == NULL)
		return;

	tcgetattr(0, &term);
	tcgetattr(0, &saved_term);
	term.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &term);

	if (prompt != NULL) {
		printf("%s", prompt);
		fflush(stdout);
	}
	if (fgets(buf, maxlen, stdin) == NULL)
		*buf = 0;

	tcsetattr(0, TCSANOW, &saved_term);

	printf("\n");
}

int main(void) {
char buf[128], buf2[128], crypt_buf[MAX_CRYPTED];

	printf("%s", print_copyright(SHORT, "mkpasswd", buf));

	getpassword("Enter password: ", buf, 128);
	if (!*buf)
		exit(1);
	buf[127] = 0;

	getpassword("Enter it again (for verification): ", buf2, 128);
	if (!*buf2)
		exit(1);
	buf2[127] = 0;

	if (strcmp(buf, buf2)) {
		printf("Passwords didn't match\n");
		exit(-1);
	}
	rtc = time(NULL);
	init_crypt();
	printf("%s\n", crypt_phrase(buf, crypt_buf));
	exit(0);
	return 0;
}

/* EOB */
