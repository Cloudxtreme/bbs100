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
	copyright.c	WJ99
*/

#include <config.h>

#include "copyright.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>


char *print_copyright(int full, char *progname) {
static char buf[256];

	strcpy(buf, "bbs100 ");
	strcat(buf, VERSION);

	if (progname != NULL && *progname) {
		strcat(buf, " ");
		strcat(buf, progname);
	}
	strcat(buf, " by Walter de Jong <walter@heiho.net> (C) 2003\n");

	if (full) {
		struct utsname uts;

		strcat(buf, "running on ");
		if (!uname(&uts))
			sprintf(buf+strlen(buf), "%s, %s %s %s ", uts.nodename, uts.machine, uts.sysname, uts.release);
		strcat(buf, "[" SYSTEM "]\n");
	}
	return buf;
}

/* EOB */
