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
	Chatter18	WJ97
	cstring.c
*/

#include <config.h>

#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>


char *cstrdup(char *s) {
char *p;

	if (s == NULL)
		return NULL;

	if ((p = (char *)Malloc(strlen(s)+1, TYPE_CHAR)) == NULL)
		return NULL;
	strcpy(p, s);
	return p;
}

char *cstrlwr(char *s) {
char *p;

	p = s;
	while(p && *p) {
		if (*p >= 'A' && *p <= 'Z')
			*p += ' ';
		p++;
	}
	return s;
}

char *cstrupr(char *s) {
char *p;

	p = s;
	while(p && *p) {
		if (*p >= 'a' && *p <= 'z')
			*p -= ' ';
		p++;
	}
	return s;
}

char *cstristr(char *s, char *substr) {
int i;

	if (!substr)
		return NULL;

	i = strlen(substr);
	while(s && *s) {
		if (!cstrnicmp(s, substr, i))
			return s;
		s++;
	}
	return NULL;
}

char *cstrstr(char *s, char *substr) {
int i;

	if (!substr)
		return NULL;

	i = strlen(substr);
	while(s && *s) {
		if (!strncmp(s, substr, i))
			return s;
		s++;
	}
	return NULL;
}

char *cstrichr(char *str, char kar) {
char *p;

	if (str == NULL || !*str)
		return NULL;

	if ((p = cstrchr(str, kar)) != NULL)
		return p;

	if (kar >= 'A' && kar <= 'Z')
		kar += ' ';
	else
		if (kar >= 'a' && kar <= 'z')
			kar -= ' ';
		else {
			return NULL;
		}
	p = cstrchr(str, kar);
	return p;
}

void chop(char *msg) {
int i;

	if (msg == NULL)
		return;

	i = strlen(msg)-1;
	while((i >= 0) && (msg[i] == '\r' || msg[i] == '\n'))
		msg[i--] = 0;
}

void cstrip_line(char *msg) {
char *p;
int i;

	if (msg == NULL || !*msg)
		return;

	while((p = cstrchr(msg, 27)) != NULL)		/* filter escape characters */
		memmove(p, p+1, strlen(p)+1);
	while((p = cstrchr(msg, '\t')) != NULL)		/* tab2space */
		*p = ' ';

	i = strlen(msg);
	while(i && *msg == ' ')
		memmove(msg, msg+1, i--);

	i--;
	while(i >= 0 && (msg[i] == ' ' || msg[i] == '\r' || msg[i] == '\n'))
		msg[i--] = 0;
	return;
}

void cstrip_spaces(char *msg) {
char *p;

	p = msg;
	while((p = cstrchr(p, ' ')) != NULL) {
		p++;
		while(*p == ' ')
			memmove(p, p+1, strlen(p));		/* strlen(p+1)+1 */
	}
}

/*
	strip double slashes from a filename
*/
void cstrip_filename(char *filename) {
char *p;

	p = filename;
	while((p = cstrchr(p, '/')) != NULL) {
		p++;
		while(*p == '/')
			memmove(p, p+1, strlen(p));		/* strlen(p+1)+1 */
	}
}

/*
	Splits a line into an array
	Just like Perl's split() function
	The substrings are not copied into the array; the string that is
	passed as parameter to split() is chopped in pieces

	The return value is allocated, it must be Free()d
*/
char **cstrsplit(char *line, char token) {
char **args, *p, *startp;
int num = 2;

	if (!line || !*line)
		return NULL;

	p = line;
	while((p = cstrchr(p, token)) != NULL) {
		num++;
		p++;
	}
	if ((args = (char **)Malloc(num * sizeof(char *), TYPE_POINTER)) == NULL)
		return NULL;

	startp = p = line;
	num = 0;
	while((p = cstrchr(p, token)) != NULL) {
		args[num++] = startp;
		*p = 0;
		p++;
		startp = p;
	}
	args[num++] = startp;
	args[num] = NULL;
	return args;
}

/*
	Joins splitted string-parts into one string again
	Use Free() to free the allocated string again
*/
char *cstrjoin(char **args) {
char *buf;
int i, l = 0;

	for(i = 0; args[i] != NULL && *args[i]; i++)
		l += strlen(args[i]) + 1;
	l++;
	if ((buf = (char *)Malloc(l, TYPE_CHAR)) == NULL)
		return NULL;

	*buf = 0;
	if (args == NULL)
		return buf;

	l = 0;
	for(i = 0; args[i] != NULL && *args[i]; i++) {
		strcat(buf, args[i]);
		l = strlen(buf);
		buf[l++] = ' ';
		buf[l] = 0;
	}
	if (l)
		buf[--l] = 0;
	return buf;
}

/* EOB */
