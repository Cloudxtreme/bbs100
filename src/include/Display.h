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
	Display.h	WJ105
*/

#ifndef DISPLAY_H_WJ105
#define DISPLAY_H_WJ105	1

#include "StringIO.h"

typedef struct {
	StringIO *buf;
	int term_width, term_height;
	int cpos, line;
} Display;

Display *new_Display(void);
void destroy_Display(Display *);

#endif	/* DISPLAY_H_WJ105 */

/* EOB */
