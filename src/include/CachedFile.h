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
	CachedFile.h	WJ100
*/

#ifndef CACHEDFILE_H_WJ100
#define CACHEDFILE_H_WJ100 1

#include "StringList.h"
#include "StringIO.h"
#include "Timer.h"

#include <stdarg.h>
#include <time.h>

#define FILE_OPEN		1		/* file is open */
#define FILE_DIRTY		2		/* file is dirty and should be synced */

#define FSEEK_SET		0
#define FSEEK_CUR		1
#define FSEEK_END		2

typedef struct File_tag File;

struct File_tag {
	int flags;
	char *filename;
	StringIO *data;
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
extern Timer *expire_timer;

int init_FileCache(void);
void deinit_FileCache(void);

CachedFile *new_CachedFile(void);
void destroy_CachedFile(CachedFile *);

File *new_File(void);
void destroy_File(File *);

File *Fopen(char *);
int Fclose(File *);
void Fcancel(File *);
File *Fcreate(char *);
void Frewind(File *);
char *Fgets(File *, char *, int);
int Fputs(File *, char *);
int vFprintf(File *, char *, va_list);
int Fprintf(File *, char *, ...);
StringList *Fgetlist(File *);
void Fputlist(File *, StringList *);
int Fget_StringIO(File *, StringIO *);
int Fput_StringIO(File *, StringIO *);
int unlink_file(char *);
int rename_dir(char *, char *);

CachedFile *in_Cache(char *);
void add_Cache(File *);
void remove_Cache_filename(char *);
int resize_Cache(void);
void fifo_remove_Cache(CachedFile *);
void fifo_add_Cache(CachedFile *);

void cache_expire_timerfunc(void *);

#endif	/* CACHEDFILE_H_WJ100 */

/* EOB */
