/*
    bbs100 1.2.2 WJ103
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
	SU_Passwd.c	WJ99
*/

#include "config.h"
#include "SU_Passwd.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"

#include <stdio.h>
#include <stdlib.h>

SU_Passwd *su_passwd = NULL;


SU_Passwd *new_SU_Passwd(void) {
SU_Passwd *su;

	if ((su = (SU_Passwd *)Malloc(sizeof(SU_Passwd), TYPE_SU_PASSWD)) == NULL)
		return NULL;

	return su;
}

void destroy_SU_Passwd(SU_Passwd *su) {
	Free(su);
}


SU_Passwd *load_SU_Passwd(char *filename) {
SU_Passwd *su = NULL, *new_su;
AtomicFile *f;
char buf[MAX_CRYPTED+MAX_NAME+10], *p;

	if ((f = openfile(filename, "r")) == NULL)
		return NULL;

	while(fgets(buf, MAX_CRYPTED+MAX_NAME+10, f->f) != NULL) {
		cstrip_line(buf);
		if (!*buf)
			continue;

		if ((p = cstrchr(buf, ':')) == NULL)
			continue;

		*p = 0;
		p++;
		if (!*p || !*buf)
			continue;

		if ((new_su = new_SU_Passwd()) == NULL)
			break;

		strncpy(new_su->name, buf, MAX_NAME);
		new_su->name[MAX_NAME] = 0;

		strncpy(new_su->passwd, p, MAX_CRYPTED);
		new_su->passwd[MAX_CRYPTED] = 0;

		add_SU_Passwd(&su, new_su);
	}
	closefile(f);
	return su;
}

int save_SU_Passwd(SU_Passwd *su, char *filename) {
AtomicFile *f;

	if ((f = openfile(filename, "w")) == NULL)
		return -1;

	while(su != NULL) {
		fprintf(f->f, "%s:%s\n", su->name, su->passwd);
		su = su->next;
	}
	closefile(f);
	return 0;
}

char *get_su_passwd(char *name) {
SU_Passwd *su;

	if (name == NULL || !*name)
		return NULL;

	for(su = su_passwd; su != NULL; su = su->next) {
		if (!strcmp(su->name, name))
			return su->passwd;
	}
	return NULL;
}

/* EOB */
