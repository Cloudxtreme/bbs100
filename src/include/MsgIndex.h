/*
    bbs100 1.2.3 WJ104
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
	message.h	WJ99
*/

#ifndef MSGINDEX_H_WJ99
#define MSGINDEX_H_WJ99 1

#include <config.h>

#include "List.h"

#define add_MsgIndex(x,y)			add_List((x), (y))
#define concat_MsgIndex(x,y)		concat_List((x), (y))
#define remove_MsgIndex(x,y)		remove_List((x), (y))
#define rewind_MsgIndex(x)			(MsgIndex *)rewind_List(x)
#define unwind_MsgIndex(x)			(MsgIndex *)unwind_List(x)
#define sort_MsgIndex(x, y)			(MsgIndex *)sort_List((x), (y))
#define listdestroy_MsgIndex(x)		listdestroy_List((x), destroy_MsgIndex)

typedef struct MsgIndex_tag MsgIndex;

struct MsgIndex_tag {
	List(MsgIndex);

	unsigned long number;
};

MsgIndex *new_MsgIndex(unsigned long);
void destroy_MsgIndex(MsgIndex *);

#endif	/* MSGINDEX_H_WJ99 */

/* EOB */
