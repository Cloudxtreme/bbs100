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
#include "log.h"
#include "cstring.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Hash *languages = NULL;


int init_Lang(void) {
	if (languages == NULL && (languages = new_Hash()) == NULL)
		return -1;

	languages->hashaddr = hashaddr_ascii;
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
	if (l == NULL)
		return;

	if (l->name != NULL) {
		Free(l->name);
		l->name = NULL;
	}
	destroy_Hash(l->hash);
	l->hash = NULL;
}

Lang *add_language(char *lang) {
Lang *l;

	if (lang == NULL)
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

Lang *load_phrasebook(char *filename) {
Lang *l = NULL;
AtomicFile *f;
char line_buf[PRINT_BUF], buf[PRINT_BUF], keybuf[32], *p;
int line_no, errors, continued, len, key;

	if (filename == NULL)
		return NULL;

	if ((f = openfile(filename, "r")) == NULL)
		return NULL;

	line_no = errors = continued = 0;
	line_buf[0] = 0;

	while(fgets(buf, PRINT_BUF, f->f) != NULL) {
		line_no++;

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

		if ((p = cstrchr(line_buf, ' ')) == NULL)
			p = cstrchr(line_buf, '\t');
		if (p == NULL) {
			log_err("load_Lang(%s): syntax error in line %d\n", filename, line_no);
			errors++;
			continue;
		}
		p++;
		if (!*p) {
			log_err("load_Lang(%s): syntax error in line %d\n", filename, line_no);
			errors++;
			continue;
		}
		*p = 0;
		p++;
		if (!*p) {
			log_err("load_Lang(%s): syntax error in line %d\n", filename, line_no);
			errors++;
			continue;
		}
/*
	if new definition, compute new key for the foreign strings to follow
	they are stored with the the key of the English text as their own key,
	so the translated text can be found
*/
		if (!strcmp(line_buf, "bbs100")) {
			key = hashaddr_ascii(p);
			sprintf(keybuf, "%x", key);
			continue;
		}
		if ((l = add_language(line_buf)) == NULL) {
			log_err("load_Lang(%s): failed to add language %s\n", filename, line_buf);
			errors++;
			break;
		}
		if (add_Hash(l->hash, keybuf, p) == -1) {
			log_err("load_Lang(%s): failed to add a new phrase\n", filename);
			errors++;
			break;
		}
	}
	closefile(f);

	if (errors) {
		destroy_Lang(l);
		return NULL;
	}
	return l;
}

/*
	translate a given system text into a loaded phrasebook text
*/
char *translate(Lang *l, char *text) {
int key;
char keybuf[32], *p;

	if (l == NULL || text == NULL || !*text)
		return text;

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

	for(i = 0; i < languages->size; i++) {
		for(hl = languages->hash[i]; hl != NULL; hl = hl->next) {
			log_debug("--- dumping language %s", hl->key);
			dump_Lang((Lang *)hl->value);
		}
	}
	log_debug("--- end dump_languages()");
}

/* EOB */
