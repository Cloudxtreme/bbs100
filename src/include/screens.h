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
	screens.h	WJ99
*/

#ifndef SCREENS_H_WJ99
#define SCREENS_H_WJ99 1

#include "User.h"

extern StringIO *crash_screen;

int init_screens(void);
int load_screen(StringIO *, char *);
int display_screen(User *, char *);
void display_text(User *, StringIO *);

#endif	/* SCREENS_H_WJ99 */

/* EOB */
