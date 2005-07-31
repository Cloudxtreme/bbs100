/*
    bbs100 3.0 WJ105
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
	StringList.h    WJ98
*/

#ifndef STRINGLIST_H_WJ98
#define STRINGLIST_H_WJ98 1

#include "List.h"

#include <stdarg.h>

#define add_StringList(x,y)			(StringList *)add_List((x), (y))
#define concat_StringList(x,y)		(StringList *)concat_List((x), (y))
#define remove_StringList(x,y)		(StringList *)remove_List((x), (y))
#define listdestroy_StringList(x)	listdestroy_List((x), destroy_StringList)
#define rewind_StringList(x)		(StringList *)rewind_List(x)
#define unwind_StringList(x)		(StringList *)unwind_List(x)
#define sort_StringList(x, y)		(StringList *)sort_List((x), (y))

typedef struct StringList_tag StringList;

struct StringList_tag {
	List(StringList);

	char *str;
};

StringList *new_StringList(char *);
void destroy_StringList(StringList *);
StringList *in_StringList(StringList *, char *);
char *str_StringList(StringList *);
StringList *load_StringList(char *);
int save_StringList(StringList *, char *);
StringList *copy_StringList(StringList *);
StringList *vadd_String(StringList **, char *, va_list);
StringList *add_String(StringList **, char *, ...);
int alphasort_StringList(void *, void *);

#endif	/* STRINGLIST_H_WJ98 */

/* EOB */
