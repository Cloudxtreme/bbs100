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
	cprintf.h	WJ105
*/

#ifndef CPRINTF_H_WJ105
#define CPRINTF_H_WJ105	1

#include "config.h"

#ifndef HAVE_VPRINTF
#error This package uses vprintf(), which you don't have
#endif


#ifndef HAVE_SNPRINTF
#define csnprintf	c_snprintf

int c_snprintf(char *, int, char *, ...);

#else
#define csnprintf	snprintf
#endif


#ifndef HAVE_VSNPRINTF
#define cvsnprintf	c_vsnprintf

#include <stdarg.h>

int c_vsnprintf(char *, int, char *, va_list);

#else
#define cvsnprintf	vsnprintf
#endif


#endif	/* CPRINTF_H_WJ105 */

/* EOB */
