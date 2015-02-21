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
	SU_Passwd.h	WJ99
*/

#ifndef SU_PASSWD_H_WJ99
#define SU_PASSWD_H_WJ99 1

#include "KVPair.h"

int load_SU_Passwd(char *);
int save_SU_Passwd(char *);
char *get_su_passwd(char *);

extern KVPair *su_passwd;

#endif	/* SU_PASSWD_H_WJ99 */

/* EOB */
