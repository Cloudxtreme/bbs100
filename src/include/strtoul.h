/*
    bbs100 2.0 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	strtoul.h	WJ99
*/

#ifndef STRTOUL_H_WJ99
#define STRTOUL_H_WJ99 1

#include <config.h>

#ifndef HAVE_STRTOUL
#define HAVE_STRTOUL 1

#include <stdio.h>
#include <stdlib.h>

unsigned long strtoul(char *ptr, char **endptr, int base) {
	if (endptr != NULL) {
		fprintf(stderr, "BUG %s %d: unsupported endptr for strtoul() ; must be NULL\n",
			__FILE__, __LINE__);
		return (unsigned long)-1UL;
	}
	if (base == 10)
		return (unsigned long)atol(ptr);

	if (base == 16) {
		long l = -1L;

		sscanf(ptr, "%lx", &l);
		return (unsigned long)l;
	}
	fprintf(stderr, "BUG %s %d: unsupported base for strtoul(ptr, endptr, %d);\n",
		__FILE__, __LINE__, base);
	return (unsigned long)-1UL;
}

#endif

#endif	/* STRTOUL_H_WJ99 */

/* EOB */

