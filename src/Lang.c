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

#include "config.h"
#include "Lang.h"
#include "Memory.h"
#include "AtomicFile.h"
#include "Param.h"
#include "log.h"
#include "cstring.h"
#include "util.h"
#include "defines.h"
#include "debug.h"
#include "mydirentry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


Hash *languages = NULL;
static Lang *default_language = NULL;


int init_Lang(void) {
	if (languages == NULL && (languages = new_Hash()) == NULL)
		return -1;

	languages->hashaddr = hashaddr_ascii;

	if (!strcmp(PARAM_DEFAULT_LANGUAGE, "bbs100")) {
		default_language = NULL;
		return 0;
	}
	if ((default_language = load_Language(PARAM_DEFAULT_LANGUAGE)) == NULL) {
		log_err("failed to load default language %s", PARAM_DEFAULT_LANGUAGE);
		return -1;
	}
	return 0;
}

Lang *new_Lang(void) {
Lang *l;

	if ((l = (Lang *)Malloc(sizeof(Lang), TYPE_LANG)) == NULL)
		return NULL;

	if ((l->hash = new_Hash()) == NULL) {
		Free(l);
		return NULL;
	}
	l->hash->hashaddr = hashaddr_ascii;
	return l;
}

void destroy_Lang(Lang *l) {
int i;
HashList *hl;

	if (l == NULL)
		return;

	if (l->refcount != 0)
		log_err("destroy_Lang(): refcount != 0 (%d)", l->refcount);

	if (l->name != NULL) {
		Free(l->name);
		l->name = NULL;
	}
/*
	free all data that is stored in the hash
*/
	for(i = 0; i < l->hash->size; i++) {
		for(hl = l->hash->hash[i]; hl != NULL; hl = hl->next) {
			Free((char *)hl->value);
			hl->value = NULL;
		}
	}
	destroy_Hash(l->hash);
	l->hash = NULL;
	Free(l);
}

/*
	demand load language directory for the specified language

	it keeps a refcount, so this function assumes that when you load it,
	you're actually using it too
*/
Lang *load_Language(char *lang) {
Lang *l;
char dirname[MAX_PATHLEN], filename[MAX_PATHLEN];
DIR *dirp;
struct dirent *direntp;

	if (lang == NULL || !*lang)
		return NULL;

	if ((l = in_Hash(languages, lang)) != NULL) {
		l->refcount++;
		log_debug("load_Language(%s): refcount == %d", lang, l->refcount);
		return l;
	}
	log_debug("load_Language(): loading %s", lang);
	sprintf(dirname, "%s/%s", PARAM_LANGUAGEDIR, lang);
	path_strip(dirname);

	if ((dirp = opendir(dirname)) == NULL) {
		log_debug("load_Language(): failed to opendir %s", dirname);
		return NULL;
	}
	while((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_name[0] == '.')
			continue;

		if (strlen(dirname) + strlen(direntp->d_name) + 2 >= MAX_PATHLEN) {
			log_err("load_Language(%s): path too long, skipped", lang);
			continue;
		}
		sprintf(filename, "%s/%s", dirname, direntp->d_name);
		path_strip(filename);

		l = load_phrasebook(lang, filename);
	}
	closedir(dirp);

	if (l != NULL) {
		l->refcount = 1;
		log_debug("load_Language(%s): refcount == %d", lang, l->refcount);
		dump_Lang(l);
	} else
		log_debug("load_Language(%s): failed to load", lang);
	return l;
}

/*
	unload a language, unless it is still in use
*/
void unload_Language(char *lang) {
Lang *l;

	if (lang == NULL || !*lang || (l = in_Hash(languages, lang)) == NULL)
		return;

	l->refcount--;
	log_debug("unload_language(%s): refcount == %d", lang, l->refcount);

	if (l->refcount <= 0) {
		log_debug("unload_language(): removing language %s", lang);

		remove_Hash(languages, lang);
		destroy_Lang(l);
	}
}

/*
	load a language file
*/
Lang *load_phrasebook(char *lang, char *filename) {
Lang *l = NULL;
AtomicFile *f;
char line_buf[PRINT_BUF], buf[PRINT_BUF], keybuf[32];
int line_no, errors, continued, len, key;

	if (filename == NULL)
		return NULL;

	log_debug("load_phrasebook(): loading %s", filename);

	if ((l = add_language(lang)) == NULL) {
		log_err("load_phrasebook(): failed to add language %s", lang);
		return NULL;
	}
	if ((f = openfile(filename, "r")) == NULL) {
		log_err("load_phrasebook(): failed to open file %s", filename);
		return NULL;
	}
	line_no = errors = continued = 0;
	line_buf[0] = keybuf[0] = 0;

	while(fgets(buf, PRINT_BUF, f->f) != NULL) {
		line_no++;

		if (!continued)
			line_buf[0] = 0;

		cstrip_spaces(buf);
		chop(buf);

		if (*buf == '#')
			continue;

		if (!*buf && !continued)
			continue;
/*
	lines can be continued by adding a trailing backslash, which adds a newline in the text
*/
		len = strlen(buf) - 1;
		if (len > 0 && buf[len] == '\\') {
			buf[len] = 0;
			cstrip_spaces(buf);
			continued = 1;
		} else
			continued = 0;

		len = strlen(line_buf);
		if (continued) {
			line_buf[len++] = '\n';
			line_buf[len] = 0;
		}
		if (len + strlen(buf) + 1 >= PRINT_BUF) {
			log_err("load_Lang(%s): line too long on line %d, truncated", filename, line_no);
			buf[len + strlen(buf) + 1 - PRINT_BUF] = 0;
		}
		strcpy(line_buf + len, buf);

		if (continued)
			continue;

		if (!*keybuf) {
/*
	if new definition, compute new key for the foreign line that follows
	They are stored with the key of the 'bbs100' text as their own key,
	so the translated text can be found
	Note that it cannot be translated backwards, because there is no index for it
*/
			key = hashaddr_ascii(line_buf);
			sprintf(keybuf, "%x", key);
			continue;
		}
		if (add_Hash(l->hash, keybuf, cstrdup(line_buf)) == -1) {
			log_err("load_Lang(%s): failed to add a new phrase\n", filename);
			errors++;
			break;
		}
		keybuf[0] = 0;
	}
	closefile(f);

	if (errors) {
		destroy_Lang(l);
		return NULL;
	}
	return l;
}

Lang *add_language(char *lang) {
Lang *l;

	if (lang == NULL || languages == NULL)
		return NULL;

	cstrlwr(lang);
	if ((l = (Lang *)in_Hash(languages, lang)) == NULL) {
		if ((l = new_Lang()) == NULL)
			return NULL;

		if ((l->name = cstrdup(lang)) == NULL) {
			destroy_Lang(l);
			return NULL;
		}
		if (add_Hash(languages, lang, l) == -1) {
			destroy_Lang(l);
			return NULL;
		}
	}
	return l;
}

/*
	translate a given system text into a loaded phrasebook text
*/
char *translate(Lang *l, char *text) {
int key;
char keybuf[32], *p;

	if (text == NULL || !*text)
		return text;

	if (l == NULL) {
		if (!strcmp(PARAM_DEFAULT_LANGUAGE, "bbs100"))
			return text;

		l = default_language;
		if (l == NULL)
			return text;
	}
	key = hashaddr_ascii(text);
	sprintf(keybuf, "%x", key);
	if ((p = (char *)in_Hash(l->hash, keybuf)) == NULL)
		return text;

	return p;
}

char *translate_by_name(char *lang, char *text) {
	return translate((Lang *)in_Hash(languages, lang), text);
}


void dump_Lang(Lang *l) {
int i;
HashList *hl;

	log_debug("Lang {");
	log_debug("  name = [%s]", l->name);
	log_debug("  hash = {");

	for(i = 0; i < l->hash->size; i++)
		for(hl = l->hash->hash[i]; hl != NULL; hl = hl->next)
			log_debug("    \"%s\" : \"%s\"", hl->key, (char *)hl->value);

	log_debug("  }");
	log_debug("}");
}

void dump_languages(void) {
int i;
HashList *hl;

	log_debug("--- dump_languages():");

	if (languages == NULL) {
		log_debug("languages == NULL");
		return;
	}
	for(i = 0; i < languages->size; i++) {
		for(hl = languages->hash[i]; hl != NULL; hl = hl->next) {
			log_debug("--- dumping language %s", hl->key);
			dump_Lang((Lang *)hl->value);
		}
	}
	log_debug("--- end dump_languages()");
}

/* EOB */
