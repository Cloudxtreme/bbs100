/*
	hostmap.c	WJ100
*/

#include "config.h"
#include "HostMap.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"
#include "Hash.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

static Hash *hostmap = NULL;

int load_HostMap(char *filename) {
AtomicFile *f;
char buf[256], *p;
int line_no;

	destroy_Hash(hostmap);
	hostmap = new_Hash();

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
		add_Hash(hostmap, buf, cstrdup(p), Free);
		if (in_Hash(hostmap, buf) == NULL) {
			log_debug("load_HostMap(): bug in Hash");
			abort();
		}
	}
	closefile(f);
	return 0;
}

char *HostMap_desc(char *site) {
char *desc;

	desc = (char *)in_Hash(hostmap, site);
	log_debug("HostMap_desc(%s): %s", site, desc);
	return desc;
}

/* EOB */
