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
	PList.h    WJ100
*/

#ifndef PLIST_H_WJ100
#define PLIST_H_WJ100 1

#include "List.h"

#define add_PList(x,y)			(PList *)add_List((x), (y))
#define concat_PList(x,y)		(PList *)concat_List((x), (y))
#define remove_PList(x,y)		remove_List((x), (y))
#define listdestroy_PList(x)	listdestroy_List((x), destroy_PList)
#define rewind_PList(x)			(PList *)rewind_List(x)
#define unwind_PList(x)			(PList *)unwind_List(x)
#define sort_PList(x,y)			(PList *)sort_List((x), (y))

typedef struct PList_tag PList;

struct PList_tag {
	List(PList);
	void *p;
};

PList *new_PList(void *);
void destroy_PList(PList *);
PList *in_PList(PList *, void *);

#endif	/* PLIST_H_WJ100 */

/* EOB */
