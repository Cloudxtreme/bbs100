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
	HostMap.c	WJ100
*/

#include "config.h"
#include "HostMap.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"
#include "KVPair.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

static KVPair *hostmap = NULL;

static int hostmap_sort_func(void *, void *);


int load_HostMap(char *filename) {
AtomicFile *f;
KVPair *kv;
char buf[256], *p;
int line_no;

	listdestroy_KVPair(hostmap);
	hostmap = NULL;

	if ((f = openfile(filename, "r")) == NULL)
		return 1;

	line_no = 0;
	while(fgets(buf, 256, f->f) != NULL) {
		line_no++;
		if ((p = cstrchr(buf, '#')) != NULL)
			*p = 0;

		cstrip_line(buf);
		if (!*buf)
			continue;

		cstrip_spaces(buf);
		if ((p = cstrchr(buf, ' ')) == NULL) {
			log_warn("load_HostMap(%s): error in line %d, ignored", filename, line_no);
			continue;				/* error in line; ignore */
		}
		*p = 0;
		p++;
		if (!*p) {
			log_warn("load_HostMap(%s): error in line %d, ignored", filename, line_no);
			continue;				/* error, ignore */
		}
		if ((kv = new_KVPair()) == NULL)
			break;

		KVPair_setstring(kv, buf, p);
		add_KVPair(&hostmap, kv);
	}
	closefile(f);

	hostmap = sort_KVPair(hostmap, hostmap_sort_func);
	return 0;
}

/*
	sort by key lenght, place longest first
*/
static int hostmap_sort_func(void *v1, void *v2) {
KVPair *kv1, *kv2;
int l1, l2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	kv1 = *(KVPair **)v1;
	kv2 = *(KVPair **)v2;

	if (kv1 == NULL || kv2 == NULL)
		return 0;

	if (kv1->key == NULL)
		l1 = 0;
	else
		l1 = strlen(kv1->key);

	if (kv2->key == NULL)
		l2 = 0;
	else
		l2 = strlen(kv2->key);

	if (l1 > l2)
		return -1;

	if (l1 < l2)
		return 1;

	return 0;
}

char *HostMap_desc(char *site) {
KVPair *kv;
int len, len2;

	if (site == NULL || !*site)
		return NULL;

	len2 = strlen(site);

	for(kv = hostmap; kv != NULL; kv = kv->next) {
		if (kv->key == NULL)
			len = 0;
		else
			len = strlen(kv->key);

		if (len > len2)
			continue;

/* see if the end of the string matches */
		if (!strcmp(site+len2-len, kv->key))
			return kv->value.s;
	}
	return NULL;
}



/* EOB */
