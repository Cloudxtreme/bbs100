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
	src_sum.c	WJ105

	bugs: this should be a script
*/

#include "config.h"
#include "md5.h"
#include "mydirentry.h"
#include "copyright.h"
#include "source_sum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


int sum_file(char *filename, unsigned char digest[MD5_DIGITS]) {
int fd, err;
char buf[512];
md5_context ctx;

	md5_starts(&ctx);

	if ((fd = open(filename, O_RDONLY)) < 0) {
		fprintf(stderr, "failed to open file %s\n", filename);
		return -1;
	}
	while((err = read(fd, buf, 512)) > 0)
		md5_update(&ctx, buf, err);

	close(fd);
	md5_finish(&ctx, digest);
	return 0;
}

void print_md5_digest(char *filename, unsigned char digest[MD5_DIGITS]) {
int i;

	for(i = 0; i < MD5_DIGITS; i++)
		printf("%02x", digest[i] & 0xff);
	printf("  %s\n", filename);
}

void fprint_source_entry(FILE *f, char *filename, unsigned char digest[MD5_DIGITS]) {
int i;
char *p;

	if ((p = strrchr(filename, '.')) != NULL)
		*p = 0;

	fprintf(f, "\t{ \"%s\",\t{ ", filename);
	for(i = 0; i < 15; i++)
		fprintf(f, "0x%02x,", digest[i] & 0xff);
	fprintf(f, "0x%02x }, },\n", digest[15]);

	if (p != NULL)
		*p = '.';
}

int source_sum(void) {
FILE *f;
DIR *dirp;
struct dirent *direntp;
unsigned char digest[MD5_DIGITS];
int l;

	if ((dirp = opendir(".")) == NULL) {
		fprintf(stderr, "failed to open current directory\n");
		return -1;
	}
	if ((f = fopen("build_sums.c", "w+")) == NULL) {
		fprintf(stderr, "failed to write build_sums.c\n");
		return -1;
	}
	fprintf(f,
		"/*\n"
		"	build_sums.c\n"
		"\n"
		"	This code was compile-time generated\n"
		"	(changes will be lost)\n"
		"*/\n"
		"\n"
		"#include \"source_sum.h\"\n"
		"\n"
		"SourceSum build_sums[] = {\n"
	);
	while((direntp = readdir(dirp)) != NULL) {
		if (!strcmp(direntp->d_name, "build_sums.c"))
			continue;

		l = strlen(direntp->d_name);
		if (l > 2 && direntp->d_name[l-2] == '.' && direntp->d_name[l-1] == 'c') {
			sum_file(direntp->d_name, digest);
			fprint_source_entry(f, direntp->d_name, digest);
			print_md5_digest(direntp->d_name, digest);
		}
	}
	closedir(dirp);

	memset(digest, 0, MD5_DIGITS);
	print_md5_digest("NULL", digest);

	fprintf(f, "};\n"
		"\n"
		"/* EOB */\n"
	);
	fclose(f);
	return 0;
}

int main(void) {
char buf[256];

	printf("%s", print_copyright(SHORT, "src_sum", buf));
	source_sum();
	return 0;
}

/* EOB */
