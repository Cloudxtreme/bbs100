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
	Wrapper.h	WJ99
*/

#ifndef _WRAPPER_H_WJ99
#define _WRAPPER_H_WJ99 1

#include "List.h"

#include <netinet/in.h>

#define add_Wrapper(x,y)		(Wrapper *)add_List((x), (y))
#define concat_Wrapper(x,y)		(Wrapper *)concat_List((x), (y))
#define remove_Wrapper(x,y)		(Wrapper *)remove_List((x), (y))
#define rewind_Wrapper(x)		(Wrapper *)rewind_List((x))
#define listdestroy_Wrapper(x)	listdestroy_List((x), destroy_Wrapper)

#define WRAPPER_ALLOW			1	/* allow connection (default is deny) */
#define WRAPPER_APPLY_ALL		2	/* this rule applies to all users (default is New users only) */
#define WRAPPER_IP4				4	/* print as IPv4 address (default is IPv6) */
#define WRAPPER_MIXED			8	/* print as ::IPv6:IPv4 notation (default is IPv6) */

/* flag for allow_Wrapper() */
#define WRAPPER_NEW_USER		0
#define WRAPPER_ALL_USERS		1

typedef union {
	struct in_addr ipv4;
	struct in6_addr ipv6;
	char saddr[1];
} IP_addr;

typedef struct Wrapper_tag Wrapper;

struct Wrapper_tag {
	List(Wrapper);

	int flags;
	IP_addr addr, mask;
	char *comment;
};

extern Wrapper *AllWrappers;

Wrapper *new_Wrapper(void);
void destroy_Wrapper(Wrapper *);
Wrapper *set_Wrapper(Wrapper *, int, IP_addr *, IP_addr *, char *);
Wrapper *make_Wrapper(char *, char *, char *, char *);
int load_Wrapper(char *);
int save_Wrapper(Wrapper *, char *);
int allow_Wrapper(char *, int);
int allow_one_Wrapper(Wrapper *w, char *, int);
int mask_Wrapper(Wrapper *w, IP_addr *);

int read_wrapper_addr(Wrapper *, char *);
int read_wrapper_mask(Wrapper *, char *);

int read_inet_addr(char *, IP_addr *, int *);
int read_inet_mask(char *, IP_addr *, int);

char *print_inet_addr(IP_addr *, char *, int, int);
char *print_inet_mask(IP_addr *, char *, int, int);

void ip_bitmask(int, IP_addr *, int);

#endif	/* _WRAPPER_H_WJ99 */

/* EOB */
