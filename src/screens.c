/*
    bbs100 1.2.2 WJ103
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
	screens.c	WJ99
*/

#include "config.h"
#include "screens.h"

#ifndef NULL
#define NULL	0L
#endif

StringList *login_screen = NULL, *logout_screen = NULL, *nologin_screen = NULL,
	*motd_screen = NULL, *crash_screen = NULL, *first_login = NULL,
	*help_std = NULL, *help_config = NULL, *help_roomconfig = NULL,
	*help_sysop = NULL, *gpl_screen = NULL, *mods_screen = NULL;

/* EOB */
