/*
    bbs100 3.0 WJ106
    Copyright (C) 2006  Walter de Jong <walter@heiho.net>

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
	edit_param.h	WJ105
*/

#ifndef EDIT_PARAM_H_WJ105
#define EDIT_PARAM_H_WJ105	1

#include "User.h"

void change_int_param(User *, char, int *);
void change_int0_param(User *, char, int *);
void change_octal_param(User *, char, int *);
void change_string_param(User *, char, char **, char *);

#endif	/* EDIT_PARAM_H_WJ105 */

/* EOB */
