/*
	hostmap.c	WJ100
*/

#include "config.h"
#include "HostMap.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"

#include <stdio.h>
#include <stdlib.h>

HostMap *hostmap = NULL;

HostMap *new_HostMap(char *site, char *desc) {
HostMap *hm;

	if ((hm = (HostMap *)Malloc(sizeof(HostMap), TYPE_HOSTMAP)) == NULL)
		return NULL;

	if ((hm->site = cstrdup(site)) == NULL) {
		destroy_HostMap(hm);
		return NULL;
	}
	if ((hm->desc = cstrdup(desc)) == NULL) {
		destroy_HostMap(hm);
		return NULL;
	}
	return hm;
}

void destroy_HostMap(HostMap *hm) {
	if (hm == NULL)
		return;

	Free(hm->site);
	Free(hm->desc);
	Free(hm);
}

int load_HostMap(char *filename) {
AtomicFile *f;
char buf[256], *p;

	listdestroy_HostMap(hostmap);
	hostmap = NULL;

	if ((f = openfile(filename, "r")) == NULL)
		return 1;

	while(fgets(buf, 256, f->f) != NULL) {
		if ((p = cstrchr(buf, '#')) != NULL)
			*p = 0;

		cstrip_line(buf);
		if (!*buf)
			continue;

		cstrip_spaces(buf);
		if ((p = cstrchr(buf, ' ')) == NULL)
			continue;				/* error in line; ignore */
		*p = 0;
		p++;
		if (!*p)
			continue;				/* error, ignore */

		add_HostMap(&hostmap, new_HostMap(buf, p));
	}
	closefile(f);

	hostmap = sort_HostMap(hostmap, hostmap_sort_func);
	return 0;
}

/*
	sort by lenght, place longest first
*/
int hostmap_sort_func(void *v1, void *v2) {
HostMap *m1, *m2;
int l1, l2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	m1 = *(HostMap **)v1;
	m2 = *(HostMap **)v2;

	if (m1 == NULL || m2 == NULL)
		return 0;

	l1 = strlen(m1->site);
	l2 = strlen(m2->site);
	if (l1 > l2)
		return -1;

	if (l1 < l2)
		return 1;

	return 0;
}

char *HostMap_desc(char *site) {
HostMap *hm;
int len, len2;

	if (site == NULL || !*site)
		return NULL;

	for(hm = hostmap; hm != NULL; hm = hm->next) {
		len = strlen(hm->site);
		len2 = strlen(site);
		if (len > len2)
			continue;

/* see if the end of the string matches */
		if (!strcmp(site+len2-len, hm->site))
			return hm->desc;
	}
	return NULL;
}

/* EOB */
