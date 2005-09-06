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
	Feelings.c	WJ100
*/

#include "config.h"
#include "debug.h"
#include "Feeling.h"
#include "PList.h"
#include "mydirentry.h"
#include "Param.h"
#include "cstring.h"
#include "Memory.h"
#include "memset.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


KVPair *feelings = NULL;
int feelings_generation = 0;	/* increased whenever Sysop changes something */


/*
	this does not really 'load' the Feeling; it only sets the filename
	so we know where to find the Feeling file
*/
KVPair *load_Feeling(char *filename) {
KVPair *kv;
char *p;

	if ((kv = new_KVPair()) == NULL)
		return NULL;

	if ((p = cstrrchr(filename, '/')) != NULL)
		p++;
	else
		p = filename;

	KVPair_setpointer(kv, p, cstrdup(filename), (void (*)(void *))cstrfree);

	while((p = cstrchr(kv->key, '_')) != NULL)
		*p = ' ';
	return kv;
}

int init_Feelings(void) {
DIR *dirp;
struct dirent *direntp;
struct stat statbuf;
char buf[MAX_PATHLEN], *bufp;
KVPair *f;
int len;

	cstrcpy(buf, PARAM_FEELINGSDIR, MAX_PATHLEN);
	len = strlen(buf);
	buf[len++] = '/';
	buf[len] = 0;
	bufp = buf + len;

	if ((dirp = opendir(buf)) == NULL)
		return -1;

	listdestroy_KVPair(feelings);
	feelings = NULL;

	while((direntp = readdir(dirp)) != NULL) {
		cstrcpy(bufp, direntp->d_name, MAX_PATHLEN - len);

		if (!stat(buf, &statbuf) && S_ISREG(statbuf.st_mode)) {
			if ((f = load_Feeling(buf)) != NULL)
				feelings = prepend_KVPair(&feelings, f);
		}
	}
	closedir(dirp);

	sort_KVPair(&feelings, feeling_sort_func);
	return 0;
}

int feeling_sort_func(void *v1, void *v2) {
KVPair *kv1, *kv2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	kv1 = *(KVPair **)v1;
	kv2 = *(KVPair **)v2;

	if (kv1->key == NULL || kv2->key == NULL)
		return 0;

	return cstricmp(kv1->key, kv2->key);
}


/* make screen */
void make_feelings_screen(StringIO *screen, int width) {
KVPair *f, *f_cols[16];
int len, max_len = 0, i, j, rows, cols, buflen, total;
char buf[PRINT_BUF], fmt[MAX_LINE];

	if (screen == NULL)
		return;

	Enter(make_feelings_screen);

	free_StringIO(screen);

	if (width < 10)
		width = 10;

	if (width > PRINT_BUF/2)
		width = PRINT_BUF/2;

	total = 0;
	for(f = feelings; f != NULL; f = f->next) {
		len = strlen(f->key);
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

	memset(f_cols, 0, sizeof(KVPair *) * cols);

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

	bufprintf(fmt, MAX_LINE, "<yellow>%%3d <green>%%-%ds  ", max_len);

	for(j = 0; j < rows; j++) {
		buf[0] = 0;
		buflen = 0;
		for(i = 0; i < cols; i++) {
			if (f_cols[i] == NULL || f_cols[i]->key == NULL || !f_cols[i]->key[0])
				continue;

			buflen += bufprintf(buf+buflen, PRINT_BUF - buflen, fmt, j+i*rows+1, f_cols[i]->key);
			f_cols[i] = f_cols[i]->next;
		}
		buf[buflen++] = '\n';
		buf[buflen] = 0;

		put_StringIO(screen, buf);
	}
	Return;
}

/* EOB */
