/*
    bbs100 1.2.1 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	SU_Passwd.h	WJ99
*/

#ifndef SU_PASSWD_H_WJ99
#define SU_PASSWD_H_WJ99 1

#include "List.h"
#include "defines.h"
#include "passwd.h"

#define add_SU_Passwd(x,y)			add_List((x), (y))
#define concat_SU_Passwd(x,y)		concat_List((x), (y))
#define remove_SU_Passwd(x,y)		remove_List((x), (y))
#define rewind_SU_Passwd(x,y)		(SU_Passwd *)rewind_List((x), (y))
#define unwind_SU_Passwd(x,y)		(SU_Passwd *)unwind_List((x), (y))
#define listdestroy_SU_Passwd(x)	listdestroy_List((x), destroy_SU_Passwd)

typedef struct SU_Passwd_tag SU_Passwd;

struct SU_Passwd_tag {
	List(SU_Passwd);

	char name[MAX_NAME];
	char passwd[MAX_CRYPTED];
};

SU_Passwd *new_SU_Passwd(void);
void destroy_SU_Passwd(SU_Passwd *);

SU_Passwd *load_SU_Passwd(char *);
int save_SU_Passwd(SU_Passwd *, char *);
char *get_su_passwd(char *);

extern SU_Passwd *su_passwd;

#endif	/* SU_PASSWD_H_WJ99 */

/* EOB */
