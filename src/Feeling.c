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
	Feelings.c	WJ100
*/

#include "config.h"
#include "debug.h"
#include "Feeling.h"
#include "PList.h"
#include "StringList.h"
#include "mydirentry.h"
#include "Param.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


Feeling *feelings = NULL;
StringList *feelings_screen = NULL;


Feeling *new_Feeling(void) {
Feeling *f;

	if ((f = (Feeling *)Malloc(sizeof(Feeling), TYPE_FEELING)) == NULL)
		return NULL;

	return f;
}

void destroy_Feeling(Feeling *f) {
	if (f == NULL)
		return;

	Free(f->name);
	listdestroy_StringList(f->str);
	Free(f);
}

Feeling *load_Feeling(char *filename) {
Feeling *f;
char *p;

	if ((f = new_Feeling()) == NULL)
		return NULL;

	if ((f->str = load_StringList(filename)) == NULL) {
		Free(f);
		return NULL;
	}
	if ((p = cstrrchr(filename, '/')) != NULL) {
		p++;
		f->name = cstrdup(p);
	} else
		f->name = cstrdup(filename);

	while((p = cstrchr(f->name, '_')) != NULL)
		*p = ' ';
	return f;
}


int init_Feelings(void) {
DIR *dirp;
struct dirent *direntp;
struct stat statbuf;
char buf[MAX_PATHLEN], *bufp;
Feeling *f;

	strcpy(buf, PARAM_FEELINGSDIR);
	bufp = buf + strlen(buf);
	*bufp = '/';
	bufp++;
	*bufp = 0;

	if ((dirp = opendir(buf)) == NULL)
		return -1;

	listdestroy_Feeling(feelings);
	feelings = NULL;

	while((direntp = readdir(dirp)) != NULL) {
		strcpy(bufp, direntp->d_name);

		if (!stat(buf, &statbuf) && S_ISREG(statbuf.st_mode)) {
			if ((f = load_Feeling(buf)) != NULL)
				feelings = add_Feeling(&feelings, f);
		}
	}
	closedir(dirp);

	feelings = rewind_Feeling(feelings);
	feelings = sort_Feeling(feelings, feeling_sort_func);
	return 0;
}

int feeling_sort_func(void *v1, void *v2) {
Feeling *f1, *f2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	f1 = *(Feeling **)v1;
	f2 = *(Feeling **)v2;

	if (f1->name == NULL || f2->name == NULL)
		return 0;

	return cstricmp(f1->name, f2->name);
}


/* make screen */
void make_feelings_screen(int width) {
Feeling *f, *f_cols[16];
StringList *sl;
int len, max_len = 0, i, j, rows, cols, buflen, total;
char buf[PRINT_BUF], fmt[128];

	Enter(make_feelings_screen);

	listdestroy_StringList(feelings_screen);
	feelings_screen = NULL;

	if (width < 1 || width > 1000)
		width = 79;

	total = 0;
	for(f = feelings; f != NULL; f = f->next) {
		len = strlen(f->name);
		if (len > max_len)
			max_len = len;

		total++;
	}
	cols = width / (max_len + 6);
	if (cols < 1)
		cols = 1;
	else
		if (cols > 15)
			cols = 15;

	rows = total / cols;
	if (total % cols)
		rows++;

	memset(f_cols, 0, sizeof(Feeling *) * cols);

/* fill in array of pointers to columns */
	f = feelings;
	for(i = 0; i < cols; i++) {
		f_cols[i] = f;
		for(j = 0; j < rows; j++) {
			if (f == NULL)
				break;

			f = f->next;
		}
	}

/* make the feelings screen with sorted columns */

	sprintf(fmt, "<yellow>%%3d <green>%%-%ds  ", max_len);

	for(j = 0; j < rows; j++) {
		buf[0] = 0;
		buflen = 0;
		for(i = 0; i < cols; i++) {
			if (f_cols[i] == NULL || f_cols[i]->name == NULL)
				continue;

			sprintf(buf+buflen, fmt, j+i*rows+1, f_cols[i]->name);
			buflen = strlen(buf);

			f_cols[i] = f_cols[i]->next;
		}
		if ((sl = new_StringList(buf)) == NULL)
			break;

		feelings_screen = add_StringList(&feelings_screen, sl);
	}
	feelings_screen = rewind_StringList(feelings_screen);
	Return;
}

/* EOB */
