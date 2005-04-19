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

typedef struct Wrapper_tag Wrapper;

struct Wrapper_tag {
	List(Wrapper);

	int allow;
	unsigned long net, mask;
	char *comment;
};

extern Wrapper *wrappers;

Wrapper *new_Wrapper(int, unsigned long, unsigned long, char *);
void destroy_Wrapper(Wrapper *);
Wrapper *make_Wrapper(Wrapper **, char *, char *, char *, char *);
int load_Wrapper(Wrapper **, char *);
int save_Wrapper(Wrapper *, char *);
int allow_Wrapper(Wrapper *, char *);

#endif	/* _WRAPPER_H_WJ99 */

/* EOB */
