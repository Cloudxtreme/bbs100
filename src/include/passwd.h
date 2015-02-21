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
	passwd.h	WJ99
*/

#ifndef PASSWD_H_WJ99
#define PASSWD_H_WJ99 1

#include <config.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#define MAX_PASSPHRASE		256
#define MAX_CRYPTED			420

void init_crypt(void);
int detect_std_crypt(void);
char *crypt_phrase(char *, char *);
int verify_phrase(char *, char *);

#endif	/* PASSWD_H_WJ99 */

/* EOB */
