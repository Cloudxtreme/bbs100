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
	inet_aton.h	WJ99
*/

#ifndef INET_ATON_H_WJ99
#define INET_ATON_H_WJ99 1

#ifndef HAVE_INET_ATON
#define HAVE_INET_ATON 1

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int inet_aton(char *buf, struct in_addr *inp) {
int n1, n2, n3, n4;

	if (buf == NULL || inp == NULL)
		return -1;

	if (sscanf(buf, "%d.%d.%d.%d", &n1, &n2, &n3, &n4) != 4)
		return -1;

    if (n1 < 0 || n1 > 255
		|| n2 < 0 || n2 > 255
		|| n3 < 0 || n3 > 255
		|| n4 < 0 || n4 > 255)
		return -1;

	inp->s_addr = inet_addr(buf);
	return 0;
}

#endif

#endif	/* INET_ATON_H_WJ99 */

/* EOB */

