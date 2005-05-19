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

#include <time.h>

typedef struct Locale_tag Locale;

struct Locale_tag {
	char *name, *days[7], *months[12];

	char *(*print_date)(Locale *, struct tm *, int, char *);
	char *(*print_total_time)(Locale *, unsigned long, char *);
	char *(*print_number)(Locale *, unsigned long, char *);
	char *(*print_numberth)(Locale *, unsigned long, char *);
	char *(*possession)(Locale *, char *, char *, char *);
};

/*
	the list is only used during loading
*/
typedef struct KeymapList_tag KeymapList;

struct KeymapList_tag {
	List(KeymapList);
	unsigned char key, translated_key;
};

typedef struct {
	char *id;
	int size;
	unsigned char *keymap;
} Keymap;

typedef struct {
	char *name;
	Hash *hash, *keymaps, *unknown;
	Locale *locale;
	int refcount;
} Lang;

extern Hash *languages;
extern Lang *default_language;
extern int lang_debug;

int init_Lang(void);

Lang *new_Lang(void);
void destroy_Lang(Lang *);

Lang *load_Language(char *);
void unload_Language(char *);
Lang *load_phrasebook(char *, char *);
Lang *load_keymap(char *, char *);
Lang *add_language(char *);

char *translate(Lang *, char *);
char *translate_by_name(char *, char *);

void log_unknown_translation(char *, char *);

void dump_Lang(Lang *);
void dump_languages(void);

#endif	/* LANG_H_WJ105 */

/* EOB */
