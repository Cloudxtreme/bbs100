/*
    bbs100 3.2 WJ107
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

#ifndef LOG_H_WJ103
#define LOG_H_WJ103	1

#include <stdio.h>
#include <stdarg.h>

#define MAX_INTERNAL_LOG	50

int init_log(void);
void log_entry(FILE *, char *, char, va_list);
void log_msg(char *, ...);
void log_info(char *, ...);
void log_err(char *, ...);
void log_warn(char *, ...);
void log_debug(char *, ...);
void log_auth(char *, ...);
void log_rotate(void);

#endif	/* LOG_H_WJ103 */

/* EOB */
