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
	HostMap.c	WJ100

	the HostMap file is no longer permanently resident in memory
	it works through the file cache
*/

#include "config.h"
#include "HostMap.h"
#include "CachedFile.h"
#include "Param.h"
#include "cstring.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>


/*
	this checks if the hostmap file exists and loads it in the cache
*/
int load_HostMap(void) {
File *f;

	if ((f = Fopen(PARAM_HOSTMAP_FILE)) == NULL)
		return -1;

	Fclose(f);
	return 0;
}


char *site_description(char *site, char *desc, int buflen) {
char buf[MAX_LONGLINE], *p;
File *f;
int len, site_len, line_no;

	if (site == NULL || !*site)
		return NULL;

	site_len = strlen(site);

	if ((f = Fopen(PARAM_HOSTMAP_FILE)) == NULL)
		return NULL;

	line_no = 0;
	while(Fgets(f, buf, MAX_LONGLINE) != NULL) {
		line_no++;
		if ((p = cstrchr(buf, '#')) != NULL)
			*p = 0;

		cstrip_line(buf);
		if (!*buf)
			continue;

		cstrip_spaces(buf);
		if ((p = cstrchr(buf, ' ')) == NULL) {
			log_warn("site_description(): %s: error in line %d, ignored", PARAM_HOSTMAP_FILE, line_no);
			continue;				/* error in line; ignore */
		}
		*p = 0;
		p++;
		if (!*p) {
			log_warn("site_description(): %s: error in line %d, ignored", PARAM_HOSTMAP_FILE, line_no);
			continue;				/* error, ignore */
		}

		len = strlen(buf);
		if (len > site_len)
			continue;

/* see if the end of the string matches */
		if (!strcmp(site+site_len-len, buf)) {
			cstrncpy(desc, p, buflen);
			Fclose(f);
			return desc;
		}
	}
	Fclose(f);
	return NULL;
}

/* EOB */
