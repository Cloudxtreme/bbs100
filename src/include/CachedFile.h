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
	CachedFile.h	WJ100
*/

#ifndef CACHEDFILE_H_WJ100
#define CACHEDFILE_H_WJ100 1

#include "StringList.h"

#include <stdarg.h>
#include <time.h>

#define FILE_DIRTY		1

typedef struct File_tag File;

struct File_tag {
	unsigned int flags;
	char *filename;
	StringList *data, *datap;
};

typedef struct CachedFile_tag CachedFile;
typedef struct CachedFileList_tag CachedFileList;

struct CachedFileList_tag {
	List(CachedFile);
};

struct CachedFile_tag {
	CachedFileList hash;		/* pointers for 'hashlist' */
	CachedFileList fifo;		/* pointer for FIFO (LRU algorithm) */
	File *f;					/* the cached file */
	time_t timestamp;			/* last cache hit */
};

extern int cache_size;
extern int num_cached;

int init_FileCache(void);
void deinit_FileCache(void);

CachedFile *new_CachedFile(void);
void destroy_CachedFile(CachedFile *);

File *new_File(void);
void destroy_File(File *);

File *Fopen(char *);
void Fclose(File *);
File *Fcreate(char *);
char *Fgets(File *, char *, int);
int Fputs(File *, char *);
int vFprintf(File *, char *, va_list);
int Fprintf(File *, char *, ...);
StringList *Fgetlist(File *);
void Fputlist(File *, StringList *);
int unlink_file(char *);
int rename_dir(char *, char *);

File *copy_from_cache(char *);
void copy_to_cache(File *);
CachedFile *in_Cache(char *);
void add_Cache(File *);
void remove_Cache_filename(char *);
int resize_Cache(void);
void fifo_remove(CachedFile *);
void fifo_add(CachedFile *);

void cache_expire_timerfunc(void *);

#endif	/* CACHEDFILE_H_WJ100 */

/* EOB */
