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
	CachedFile.c	WJ100

	- works with a hash for fast searches
	- data is synced on write (close_file())

	The cache is 'list-hash' of cached file objects, which also have pointers
	to make a FIFO for the LRU algorithm.

	In this implementation, files are properly copied from and to the cache,
	and thus the user only sees the local copy of the File object
*/

#include "config.h"
#include "debug.h"
#include "CachedFile.h"
#include "Timer.h"
#include "Stats.h"
#include "cstring.h"
#include "Param.h"
#include "Memory.h"
#include "log.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

CachedFile **file_cache = NULL, *fifo_head = NULL, *fifo_tail = NULL;
int cache_size = 0, num_cached = 0;
Timer *expire_timer = NULL;

int init_FileCache(void) {
	if (!PARAM_MAX_CACHED)			/* apparently, this site wants no cache at all */
		return 0;

	if ((file_cache = (CachedFile **)Malloc(PARAM_MAX_CACHED * sizeof(CachedFile *), TYPE_POINTER)) == NULL)
		return -1;

	cache_size = PARAM_MAX_CACHED;

	if ((expire_timer = new_Timer(PARAM_CACHE_TIMEOUT * SECS_IN_MIN, cache_expire_timerfunc, TIMER_RESTART)) == NULL)
		log_err("init_FileCache(): failed to allocate a new Timer");
	else
		add_Timer(&timerq, expire_timer);

	return 0;
}

void deinit_FileCache(void) {
	if (expire_timer != NULL) {
		remove_Timer(&timerq, expire_timer);
		destroy_Timer(expire_timer);
		expire_timer = NULL;
	}
	if (file_cache != NULL) {
		int i;

		for(i = 0; i < cache_size; i++)
			destroy_CachedFile(file_cache[i]);
	}
	Free(file_cache);
	file_cache = NULL;
	fifo_head = fifo_tail = NULL;
	cache_size = num_cached = 0;
}

CachedFile *new_CachedFile(void) {
	if (!PARAM_MAX_CACHED)		/* we don't want a cache */
		return NULL;

	return (CachedFile *)Malloc(sizeof(CachedFile), TYPE_CACHEDFILE);
}

void destroy_CachedFile(CachedFile *cf) {
	if (cf == NULL)
		return;

	destroy_File(cf->f);
	Free(cf);
}


File *new_File(void) {
	return (File *)Malloc(sizeof(File), TYPE_FILE);
}

void destroy_File(File *f) {
	if (f == NULL)
		return;

	Free(f->filename);
	listdestroy_StringList(f->data);
	Free(f);
}


/*
	caculate the hash address
	this is accurate for about the last 9 to 10 characters of the filename
	(also depending on sizeof(int))
*/
int filecache_hash_addr(char *filename) {
int addr, c, data_len;
char *p, *slashp = NULL;

	if (filename == NULL || !*filename)
		return -1;

	cstrip_filename(filename);

/*
	We have a lot of files ending with 'Data' (UserData, RoomData, HomeData)
	To prevent hash pollution, don't calculate the addr for the last part of
	the filename if it ends with 'Data' (so we chop it off at the last '/')
	This is a hack to make the hash perform better
*/
	data_len = strlen(filename);
	if (data_len > 4 && !strcmp(filename+data_len-4, "Data")) {
		if ((slashp = cstrrchr(filename, '/')) != NULL && slashp != filename)
			*slashp = 0;
		else
			slashp = NULL;
	}
	p = filename;
	addr = *p;
	p++;
	while(*p) {
		c = *p - ' ';

		addr <<= 4;
		addr ^= c;

		p++;
	}
	addr %= cache_size;
	if (addr < 0)
		addr = -addr;

	if (slashp != NULL)
		*slashp = '/';				/* undo fix */
	return addr;
}


File *Fopen(char *filename) {
File *f;

	cstrip_filename(filename);

	if ((f = copy_from_cache(filename)) != NULL)
		return f;

	if ((f = new_File()) == NULL
		|| (f->data = f->datap = load_StringList(filename)) == NULL
		|| (f->filename = cstrdup(filename)) == NULL) {
		destroy_File(f);
		return NULL;
	}
	add_Cache(f);
	return f;
}

void Fclose(File *f) {
	if (f == NULL)
		return;

	if (!(f->flags & FILE_DIRTY) || f->filename == NULL || !f->filename[0]) {
		destroy_File(f);
		return;
	}
	save_StringList(f->data, f->filename);		/* sync to disk */
	copy_to_cache(f);
	destroy_File(f);
}



File *copy_from_cache(char *filename) {
File *f = NULL;
CachedFile *cf;

	if ((cf = in_Cache(filename)) == NULL
		|| (f = new_File()) == NULL
		|| (f->filename = cstrdup(cf->f->filename)) == NULL
		|| (f->data = f->datap = copy_StringList(cf->f->data)) == NULL) {
		destroy_File(f);
		return NULL;
	}
	return f;
}

void copy_to_cache(File *f) {
CachedFile *cf;

	if (f == NULL || f->filename == NULL || !f->filename[0])
		return;

	if ((cf = in_Cache(f->filename)) != NULL) {
		listdestroy_StringList(cf->f->data);
		cf->f->data = cf->f->datap = copy_StringList(f->data);
	} else
		add_Cache(f);
}


/*
	Note: new files are not cached until they are closed
*/
File *Fcreate(char *filename) {
File *f;

	cstrip_filename(filename);

	if ((f = new_File()) == NULL
		|| (f->filename = cstrdup(filename)) == NULL) {
		destroy_File(f);
		return NULL;
	}
	f->flags |= FILE_DIRTY;			/* ensure that it will be cached */
	return f;
}

void Frewind(File *f) {
	if (f == NULL)
		return;

	f->datap = f->data;
}

char *Fgets(File *f, char *buf, int max) {
	if (f == NULL || buf == NULL || f->datap == NULL)
		return NULL;

	strncpy(buf, f->datap->str, max);
	buf[max-1] = 0;
	f->datap = f->datap->next;
	return buf;
}

int Fputs(File *f, char *buf) {
StringList *sl;

	if (f == NULL || buf == NULL || (sl = new_StringList(buf)) == NULL)
		return -1;

	f->datap = add_StringList(&f->datap, sl);
	if (f->data == NULL)
		f->data = f->datap;

	f->flags |= FILE_DIRTY;
	return 0;
}

int vFprintf(File *f, char *fmt, va_list ap) {
	if (f == NULL || fmt == NULL)
		return -1;

	f->datap = vadd_String(&f->datap, fmt, ap);
	if (f->data == NULL)
		f->data = f->datap;

	f->flags |= FILE_DIRTY;
	return 0;
}

int Fprintf(File *f, char *fmt, ...) {
va_list ap;

	va_start(ap, fmt);
	return vFprintf(f, fmt, ap);
}


StringList *Fgetlist(File *f) {
StringList *sl = NULL, *root = NULL;

	if (f == NULL)
		return NULL;

	while(f->datap != NULL) {
		if (f->datap->str == NULL || !f->datap->str[0]) {
			f->datap = f->datap->next;
			break;
		}
		if ((sl = add_StringList(&sl, new_StringList(f->datap->str))) == NULL)
			break;
	
		if (root == NULL)
			root = sl;

		f->datap = f->datap->next;
	}
	return root;
}

void Fputlist(File *f, StringList *sl) {
	if (f == NULL)
		return;

	while(sl != NULL) {
		f->datap = add_StringList(&f->datap, new_StringList(sl->str));
		sl = sl->next;
	}
	f->datap = add_StringList(&f->datap, new_StringList(""));
	f->flags |= FILE_DIRTY;
}


int unlink_file(char *filename) {
	if (filename == NULL || !*filename)
		return -1;

	cstrip_filename(filename);

	remove_Cache_filename(filename);
	unlink(filename);			/* Note: this call could fail :P */
	return 0;
}

int rename_dir(char *dirname, char *newdirname) {
CachedFile *cf, *cf_next;
char buf[MAX_PATHLEN], *bufp;
int l, i;

	if (dirname == NULL || newdirname == NULL || !*dirname || !*newdirname)
		return -1;

	cstrip_filename(dirname);
	cstrip_filename(newdirname);

	rename(dirname, newdirname);		/* this does the job */

/* the rest of this routine is to satisfy the cache :P */

	strcpy(buf, dirname);
	bufp = buf + strlen(buf)-1;
	if (*bufp != '/') {
		bufp++;
		*bufp = '/';
		bufp++;
		*bufp = 0;
	}
	l = strlen(buf);

/* scan the cache for the file in the directory, and rename it */
	for(i = 0; i < cache_size; i++) {
		for(cf = file_cache[i]; cf != NULL; cf = cf_next) {
			cf_next = cf->hash.next;

			if (cf->f != NULL && !strncmp(cf->f->filename, buf, l)) {

/*
	Note: rename_dir() is used only for two things:
	1. when users are nuked, they are moved to trash/
	2. when rooms are nuked, they are moved to trash/

	I used to rename all entries, but in fact they need not be cached any
	longer so now we just remove them from the cache
*/
				remove_Cache_filename(cf->f->filename);
			}
		}
	}
	return 0;
}


CachedFile *in_Cache(char *filename) {
CachedFile *cf;
int addr;

	if (file_cache == NULL || filename == NULL || !*filename
		|| (addr = filecache_hash_addr(filename)) == -1)
		return NULL;

	cf = file_cache[addr];
	while(cf != NULL) {
		if (!strcmp(cf->f->filename, filename)) {
			stats.cache_hit++;
			fifo_remove(cf);		/* hit; move to beginning of FIFO */
			fifo_add(cf);
			return cf;
		}
		cf = cf->hash.next;
	}
	stats.cache_miss++;
	return NULL;
}

void add_Cache(File *f) {
int addr;
CachedFile *cf;

	if (!PARAM_MAX_CACHED
		|| f == NULL || f->filename == NULL || file_cache == NULL
		|| (addr = filecache_hash_addr(f->filename)) == -1
		|| (cf = new_CachedFile()) == NULL)
		return;

	if ((cf->f = new_File()) == NULL
		|| (cf->f->data = copy_StringList(f->data)) == NULL
		|| (cf->f->filename = cstrdup(f->filename)) == NULL) {
		destroy_CachedFile(cf);
		return;
	}
	if (file_cache[addr] != NULL) {
		file_cache[addr]->hash.prev = cf;		/* add cf to hash list element */
		cf->hash.next = file_cache[addr];
	}
	file_cache[addr] = cf;
	fifo_add(cf);								/* add to beginning of FIFO */
	num_cached++;

	if (num_cached > cache_size)				/* is the cache full enough? */
		remove_Cache_filename(fifo_tail->f->filename);	/* expire tail of FIFO */
}


/*
	delete entry from the cache
*/
void remove_Cache_filename(char *filename) {
CachedFile *cf;
int addr;

/*
	first, do an in_Cache(), but don't count the hit because it's misleading
	to see hits and misses rise at the same time from within the BBS
*/
	if (file_cache == NULL || filename == NULL || !*filename
		|| (addr = filecache_hash_addr(filename)) == -1)
		return;

	cf = file_cache[addr];
	while(cf != NULL) {
		if (!strcmp(cf->f->filename, filename))
			break;
		cf = cf->hash.next;
	}
	if (cf == NULL)
		return;

	if (cf->hash.prev == NULL) {			/* in root element */
		file_cache[addr] = cf->hash.next;
		if (file_cache[addr] != NULL)
			file_cache[addr]->hash.prev = NULL;
		cf->hash.next = NULL;
	} else {
		cf->hash.prev->hash.next = cf->hash.next;
		if (cf->hash.next != NULL)
			cf->hash.next->hash.prev = cf->hash.prev;
	}
	fifo_remove(cf);
	destroy_CachedFile(cf);
	num_cached--;
}

/*
	resizes the cache to new PARAM_MAX_CACHED
*/
int resize_Cache(void) {
CachedFile **new_fc, *cf, *cf_next;
int old_size, i, addr, n;

	if (!PARAM_MAX_CACHED) {				/* resized to 0 */
		deinit_FileCache();
		return 0;
	}
	if ((new_fc = (CachedFile **)Malloc(PARAM_MAX_CACHED * sizeof(CachedFile *), TYPE_POINTER)) == NULL) {
		PARAM_MAX_CACHED = cache_size;		/* keep old size */
		return -1;
	}
	old_size = cache_size;
	cache_size = PARAM_MAX_CACHED;

	n = 0;
	for(i = 0; i < old_size; i++) {
		for(cf = file_cache[i]; cf != NULL; cf = cf_next) {
			cf_next = cf->hash.next;
			cf->hash.prev = cf->hash.next = NULL;

			if (n < cache_size) {
				if ((addr = filecache_hash_addr(cf->f->filename)) != -1) {
					if (new_fc[addr] != NULL) {
						new_fc[addr]->hash.prev = cf;		/* add cf to hash list element */
						cf->hash.next = new_fc[addr];
					}
					new_fc[addr] = cf;
				}
				n++;
			} else {										/* cache was downsized */
				fifo_remove(cf);
				destroy_CachedFile(cf);
			}
		}
	}
	Free(file_cache);			/* Free() the old filecache */

	cache_size = PARAM_MAX_CACHED;
	num_cached = n;
	file_cache = new_fc;		/* set new filecache */
	return 0;
}


/*
	put CachedFile at beginning of FIFO
*/
void fifo_add(CachedFile *cf) {
	if (cf == NULL)
		return;

	cf->fifo.prev = cf->fifo.next = NULL;
	if (fifo_head == NULL)
		fifo_head = fifo_tail = cf;
	else {
		fifo_head->fifo.prev = cf;
		cf->fifo.next = fifo_head;
		fifo_head = cf;
	}
	if (fifo_tail == NULL)
		fifo_tail = fifo_head;

	cf->timestamp = rtc;
}

void fifo_remove(CachedFile *cf) {
	if (cf == NULL)
		return;

	if (cf->fifo.prev != NULL)
		cf->fifo.prev->fifo.next = cf->fifo.next;
	else
		fifo_head = cf->fifo.next;

	if (cf->fifo.next != NULL)
		cf->fifo.next->fifo.prev = cf->fifo.prev;
	else
		fifo_tail = cf->fifo.prev;

	if (fifo_tail == NULL)
		fifo_tail = fifo_head;
	else
		if (fifo_head == NULL)
			fifo_head = fifo_tail;
}

/*
	throw away cached files that have not been used for a long time
	Note that they have already been synced
*/
void cache_expire_timerfunc(void *dummy) {
time_t expire_time;

	expire_time = rtc - PARAM_CACHE_TIMEOUT;
	while(fifo_tail != NULL && fifo_tail->f != NULL && fifo_tail->timestamp < expire_time)
		remove_Cache_filename(fifo_tail->f->filename);
}

/* EOB */
