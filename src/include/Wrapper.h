/*
    bbs100 2.2 WJ105
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
	Wrapper.h	WJ99
*/

#ifndef _WRAPPER_H_WJ99
#define _WRAPPER_H_WJ99 1

#include "List.h"

#define add_Wrapper(x,y)		add_List((x), (y))
#define concat_Wrapper(x,y)		concat_List((x), (y))
#define remove_Wrapper(x,y)		remove_List((x), (y))
#define rewind_Wrapper(x)		(Wrapper *)rewind_List((x))
#define listdestroy_Wrapper(x)	listdestroy_List((x), destroy_Wrapper)

#define WRAPPER_ALLOW			1	/* allow connection (default is deny) */
#define WRAPPER_APPLY_ALL		2	/* this rule applies to all users (default is New users only) */
#define WRAPPER_IP4				4	/* print as IPv4 address (default is IPv6) */
#define WRAPPER_MIXED			8	/* print as ::IPv6:IPv4 notation (default is IPv6) */

/* flag for allow_Wrapper() */
#define WRAPPER_NEW_USER		0
#define WRAPPER_ALL_USERS		1

typedef struct Wrapper_tag Wrapper;

struct Wrapper_tag {
	List(Wrapper);

	int flags, addr[8], mask[8];
	char *comment;
};

extern Wrapper *AllWrappers;

Wrapper *new_Wrapper(void);
void destroy_Wrapper(Wrapper *);
Wrapper *set_Wrapper(Wrapper *, int, int [8], int [8], char *);
Wrapper *make_Wrapper(char *, char *, char *, char *);
int load_Wrapper(char *);
int save_Wrapper(Wrapper *, char *);
int allow_Wrapper(char *, int);
int allow_one_Wrapper(Wrapper *w, char *, int);
int mask_Wrapper(Wrapper *w, int [8]);

int read_inet_addr(char *, int [8], int *);
int read_inet_mask(char *, int [8], int);

char *print_inet_addr(int [8], char *, int);
char *print_ipv4_addr(int [8], char *);
char *print_ipv6_addr(int [8], char *, int);
char *print_inet_mask(int [8], char *, int);

void ipv4_bitmask(int, int [8]);
void ipv6_bitmask(int, int [8]);

#endif	/* _WRAPPER_H_WJ99 */

/* EOB */
