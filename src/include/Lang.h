/*
	bbs100 2.2 WJ105
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
	Lang.h	WJ105

	Multi-lingual support
*/

#ifndef LANG_H_WJ105
#define LANG_H_WJ105	1

#include "Hash.h"

typedef struct {
	char *name;
	Hash *hash;
} Lang;

extern Hash *languages;

int init_Lang(void);

Lang *new_Lang(void);
void destroy_Lang(Lang *);

Lang *add_language(char *);
Lang *load_phrasebook(char *);
char *translate(Lang *, char *);
char *translate_by_name(char *, char *);

void dump_Lang(Lang *);
void dump_languages(void);

#endif	/* LANG_H_WJ105 */

/* EOB */
