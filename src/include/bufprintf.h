/*
    bbs100 3.1 WJ107
    Copyright (C) 2007  Walter de Jong <walter@heiho.net>

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
	bufprintf.h	WJ105
*/

#ifndef BUFPRINTF_H_WJ105
#define BUFPRINTF_H_WJ105	1

#include <stdarg.h>

int bufprintf(char *, int, char *, ...);
int bufvprintf(char *, int, char *, va_list);

#endif	/* BUFPRINTF_H_WJ105 */

/* EOB */
