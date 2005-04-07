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
	Category.h	WJ105

	Categories are like floors
*/

#ifndef CATEGORY_H_WJ105
#define CATEGORY_H_WJ105	1

#include "StringList.h"

extern StringList *category;

int init_Category(void);
int load_Category(void);
int save_Category(void);
void add_Category(char *);
void remove_Category(char *);
int in_Category(char *);
int sort_category(void *, void *);

#endif	/* CATEGORY_H_WJ105 */

/* EOB */
