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
	coredump.c	WJ106

	Make live core dumps; convenient when debugging
*/

#include "config.h"
#include "coredump.h"
#include "util.h"
#include "Param.h"
#include "bufprintf.h"
#include "sys_wait.h"
#include "Process.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifdef DEBUG

void dumpcore(User *usr) {
pid_t pid;
int err;

	Enter(dumpcore);

	log_msg("SYSOP %s requested a core dump", usr->name);

/*
	ignore SIGCHLD now; this merely supresses printing an error message;
	the code in Process.c likes watching over our children ...
*/
	ignore_sigchld = 1;

	if ((pid = fork()) == -1) {
		Perror(usr, "Failed to create a core dump");
		ignore_sigchld = 0;
		Return;
	}
	if (!pid)
		abort();			/* generate core dump */

	do {
		err = waitpid(pid, NULL, 0);
	} while(err == -1 && errno == EINTR);

	ignore_sigchld = 0;
/*
	Even though checking errors is correct, continue if waitpid() didn't
	see our child. The SIGCHLD handler may already have beaten us to it.

	if (err == -1 || !err) {
		Print(usr, "<red>Failed to collect a core dump\n");
		Return;
	}
*/
	if ((err = savecore()) == 0) {
		Print(usr, "<red>Failed to collect a core dump\n");
		Return;
	}
	if (err == -1) {
		Print(usr, "<red>Failed to move core file to<white> %s\n", PARAM_CRASHDIR);
		Return;
	}
	Print(usr, "<green>A core dump has been created under<white> %s\n", PARAM_CRASHDIR);
	Return;
}

#endif	/* DEBUG */

/*
	save core dumps in the directory under log/crash/
*/
int savecore(void) {
struct stat statbuf;

	if (!stat("core", &statbuf)) {
		char filename[MAX_PATHLEN];
		struct tm *tm;
		int i;

		if ((tm = localtime(&statbuf.st_ctime)) == NULL)
			return -1;

		bufprintf(filename, sizeof(filename), "%s/core.%04d%02d%02d", PARAM_CRASHDIR, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
		path_strip(filename);
		i = 1;
		while(!stat(filename, &statbuf) && i < 10) {
			bufprintf(filename, sizeof(filename), "%s/core.%04d%02d%02d-%d", PARAM_CRASHDIR, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, i);
			path_strip(filename);
			i++;
		}
		if (rename("core", filename) == -1)
			return -1;	/* error saving core */

		return 1;		/* saved core */
	}
	return 0;			/* no core found */
}

/* EOB */
