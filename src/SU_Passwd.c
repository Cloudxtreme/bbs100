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
	SU_Passwd.c	WJ99
*/

#include "config.h"
#include "SU_Passwd.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>


KVPair *su_passwd = NULL;


int load_SU_Passwd(char *filename) {
KVPair *kv;
AtomicFile *f;
int line_no, errors;
char buf[PRINT_BUF], *p;

	listdestroy_KVPair(su_passwd);
	su_passwd = NULL;

	if ((f = openfile(filename, "r")) == NULL)
		return -1;

	line_no = errors = 0;
	while(fgets(buf, PRINT_BUF, f->f) != NULL) {
		line_no++;
		cstrip_line(buf);
		if (!*buf)
			continue;

		if ((p = cstrchr(buf, ':')) == NULL) {
			log_err("load_SU_Passwd(%s): error in line %d", filename, line_no);
			errors++;
			continue;
		}
		*p = 0;
		p++;
		if (!*p || !*buf) {
			log_err("load_SU_Passwd(%s): error in line %d", filename, line_no);
			errors++;
			continue;
		}
		if ((kv = new_KVPair()) == NULL) {
			errors++;
			break;
		}
		KVPair_setstring(kv, buf, p);
		(void)add_KVPair(&su_passwd, kv);
	}
	closefile(f);

	if (errors) {
		listdestroy_KVPair(su_passwd);
		su_passwd = NULL;
	}
	return errors;
}

int save_SU_Passwd(char *filename) {
KVPair *su;
AtomicFile *f;

	if ((f = openfile(filename, "w")) == NULL)
		return -1;

	for(su = su_passwd; su != NULL; su = su->next)
		fprintf(f->f, "%s:%s\n", su->key, su->value.s);

	return closefile(f);
}

char *get_su_passwd(char *name) {
KVPair *su;

	if (name == NULL || !*name)
		return NULL;

	for(su = su_passwd; su != NULL; su = su->next) {
		if (!strcmp(su->key, name))
			return su->value.s;
	}
	return NULL;
}

/* EOB */
