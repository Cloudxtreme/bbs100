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
	StringIO.c	WJ105

	s->size is size of the memory buffer
	s->len is length of data in the buffer
	s->pos is the current data pointer
*/

#include "config.h"
#include "StringIO.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"

#include <stdio.h>
#include <stdlib.h>


StringIO *new_StringIO(void) {
	return (StringIO *)Malloc(sizeof(StringIO), TYPE_STRINGIO);
}

void destroy_StringIO(StringIO *s) {
	if (s == NULL)
		return;

	s->len = s->size = s->pos = 0;
	Free(s->buf);
	Free(s);
}

int grow_StringIO(StringIO *s) {
int newsize;
char *p;

	if (s == NULL)
		return -1;
/*
	grow in steps
*/
	if (s->size <= 0)
		newsize = STRINGIO_MINSIZE;
	else
		if (s->size < STRINGIO_BLKSIZE)
			newsize = 2 * s->size;
		else
			newsize = s->size + STRINGIO_BLKSIZE;

	if ((p = (char *)Malloc(newsize, TYPE_CHAR)) == NULL)
		return -1;
/*
	this is not the quickest way there is
	a chain-link of buffers would be faster, but this is much easier
*/
	if (s->buf != NULL) {
		memcpy(p, s->buf, s->len);
		Free(s->buf);
	}
	s->buf = p;
	s->size = newsize;
	return 0;
}

int trunc_StringIO(StringIO *s, int len) {
char *p;

	if (s == NULL || len < 0)
		return -1;

	if (len == s->len)
		return 0;

	if (!len) {
		free_StringIO(s);
		return 0;
	}
	if ((p = (char *)Malloc(len, TYPE_CHAR)) == NULL)
		return -1;

	if (s->buf != NULL) {
		memcpy(p, s->buf, s->len);
		Free(s->buf);
	}
	s->buf = p;

	s->len = len;
	if (s->pos > len)
		s->pos = len;

	return 0;
}

int shift_StringIO(StringIO *s) {
	if (s == NULL)
		return -1;

	if (s->buf == NULL || s->pos <= 0)
		return 0;

	if (s->len > 0) {
		memmove(s->buf, s->buf + s->pos, s->len);
		s->len -= s->pos;
	}
	s->pos = 0;
	return 0;
}

/*
	warning: buf must be large enough; smallest buf is 2 bytes in size
	because it always ends with a 0-byte
*/
int read_StringIO(StringIO *s, char *buf, int len) {
int bytes_read;

	if (s == NULL || buf == NULL || len < 0)
		return -1;

	if (s->buf == NULL || len == 0)
		return 0;

	if (len > 1)
		len--;

	bytes_read = s->len - s->pos;
	if (bytes_read > len)
		bytes_read = len;

	if (bytes_read > 0)
		memcpy(buf, s->buf + s->pos, bytes_read);

	buf[bytes_read] = 0;

	s->pos += bytes_read;
	return bytes_read;
}

int write_StringIO(StringIO *s, char *str, int len) {
int bytes_written, fit, n;

	if (s == NULL || str == NULL || len < 0)
		return -1;

	if (!len)
		return 0;

	bytes_written = 0;
	while(bytes_written < len) {
		fit = s->size - 1 - s->pos;
		if (fit <= 0) {
			if (grow_StringIO(s) < 0)
				return bytes_written;
			continue;
		}
		n = len - bytes_written;
		if (n > fit)
			n = fit;

		memcpy(s->buf + s->pos, str + bytes_written, n);
		s->pos += n;
		if (s->pos > s->len) {
			s->len = s->pos;
			s->buf[s->len] = 0;
		}
		bytes_written += n;
	}
	return bytes_written;
}

int tell_StringIO(StringIO *s) {
	return (s == NULL) ? -1 : s->pos;
}

int rewind_StringIO(StringIO *s) {	
	return seek_StringIO(s, 0, STRINGIO_SET);
}

int seek_StringIO(StringIO *s, int relpos, int whence) {
int newpos;

	if (s == NULL)
		return -1;

	switch(whence) {
		case STRINGIO_SET:
			newpos = relpos;
			break;

		case STRINGIO_CUR:
			newpos = s->pos + relpos;
			break;

		case STRINGIO_END:
			newpos = s->len + relpos;
			break;

		default:
			return -1;
	}
	if (newpos < 0)
		return -1;

/*
	if you want to support holes, remove this bit
*/
	if (newpos > s->len)
		return -1;

	s->pos = newpos;
	return s->pos;
}

int copy_StringIO(StringIO *dest, StringIO *src) {
	if (dest == NULL || src == NULL)
		return -1;

	free_StringIO(dest);
	return concat_StringIO(dest, src);
}

int concat_StringIO(StringIO *dest, StringIO *src) {
int bytes_read;
char buf[1024];

	if (dest == NULL || src == NULL)
		return -1;

	seek_StringIO(src, 0, STRINGIO_SET);
	while((bytes_read = read_StringIO(src, buf, 1024)) > 0)
		write_StringIO(dest, buf, bytes_read);

	return 0;
}

int load_StringIO(StringIO *s, char *filename) {
AtomicFile *f;
char buf[PRINT_BUF];
int err, err2;

	if (s == NULL || filename == NULL || !*filename)
		return -1;

	if ((f = openfile(filename, "r")) == NULL)
		return -1;

	while((err = fread(buf, 1, PRINT_BUF, f->f)) > 0) {
		err2 = write_StringIO(s, buf, err);
		if (err2 != err) {
			closefile(f);
			return -1;
		}
	}
	closefile(f);
	return 0;
}

int save_StringIO(StringIO *s, char *filename) {
AtomicFile *f;
char buf[PRINT_BUF];
int err, err2;

	if (s == NULL || filename == NULL || !*filename)
		return -1;

	if ((f = openfile(filename, "w")) == NULL)
		return -1;

	rewind_StringIO(s);
	while((err = read_StringIO(s, buf, PRINT_BUF)) > 0) {
		err2 = fwrite(buf, 1, err, f->f);
		if (err2 != err) {
			closefile(f);
			return -1;
		}
	}
	closefile(f);
	return 0;
}

char *gets_StringIO(StringIO *s, char *buf, int size) {
int pos;
char *p;

	if (s == NULL || buf == NULL || size <= 1)
		return NULL;

	pos = tell_StringIO(s);
	if (read_StringIO(s, buf, size) <= 0)
		return NULL;

	if ((p = cstrchr(buf, '\n')) != NULL) {
		*p = 0;
		seek_StringIO(s, pos + strlen(buf) + 1, STRINGIO_SET);
	}
	return buf;
}

int put_StringIO(StringIO *s, char *str) {
	if (s == NULL || str == NULL || !*str)
		return 0;

	return write_StringIO(s, str, strlen(str));
}

int vprint_StringIO(StringIO *s, char *fmt, va_list args) {
char buf[PRINT_BUF];

	if (s == NULL || fmt == NULL || !*fmt)
		return 0;

	vsprintf(buf, fmt, args);
	va_end(args);

	return put_StringIO(s, buf);
}

int print_StringIO(StringIO *s, char *fmt, ...) {
va_list args;

	if (s == NULL || fmt == NULL || !*fmt)
		return 0;

	va_start(args, fmt);
	return vprint_StringIO(s, fmt, args);
}

void free_StringIO(StringIO *s) {
	if (s == NULL)
		return;

	s->len = s->size = s->pos = 0;
	Free(s->buf);
	s->buf = NULL;
}

/* EOB */
