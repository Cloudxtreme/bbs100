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
	Feeling.h	WJ100
*/

#ifndef FEELING_H_WJ100
#define FEELING_H_WJ100 1

#include "StringList.h"

#define add_Feeling(x,y)		(Feeling *)add_List((x), (y))
#define concat_Feeling(x,y)		(Feeling *)concat_List((x), (y))
#define remove_Feeling(x,y)		remove_List((x), (y))
#define listdestroy_Feeling(x)	listdestroy_List((x), destroy_Feeling)
#define rewind_Feeling(x)		(Feeling *)rewind_List(x)
#define unwind_Feeling(x)		(Feeling *)unwind_List(x)
#define sort_Feeling(x,y)		(Feeling *)sort_List((x), (y))

typedef struct Feeling_tag Feeling;

struct Feeling_tag {
	List(Feeling);

	char *name;
	StringList *str;
};

extern Feeling *feelings;
extern StringList *feelings_screen;

Feeling *new_Feeling(void);
void destroy_Feeling(Feeling *);
Feeling *load_Feeling(char *);
int init_Feelings(void);
int feeling_sort_func(void *, void *);
void make_feelings_screen(int);

#endif	/* FEELING_H_WJ100 */

/* EOB */
