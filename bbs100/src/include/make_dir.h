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
	make_dir.h	WJ105
*/

#ifndef MAKE_DIR_H_WJ105
#define MAKE_DIR_H_WJ105	1

#include "config.h"

#ifdef HAVE_MKDIR

#include <sys/stat.h>
#include <sys/types.h>

#define make_dir(x,y)		mkdir((x),(y))

#else

#define make_dir(x,y)		my_mkdir((x),(y))

int my_mkdir(char *, mode_t);

#endif	/* HAVE_MKDIR */


#ifdef HAVE_RMDIR

#include <sys/stat.h>
#include <sys/types.h>

#define remove_dir(x)		rmdir((x))

#else

#define remove_dir(x)		my_rmdir((x))

int my_rmdir(char *);

#endif	/* HAVE_RMDIR */

#endif	/* MAKE_DIR_H_WJ105 */

/* EOB */
