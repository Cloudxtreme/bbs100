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
	Chatter18	WJ97
	main.c
*/

#include "config.h"
#include "debug.h"
#include "copyright.h"
#include "defines.h"
#include "main.h"
#include "inet.h"
#include "passwd.h"
#include "screens.h"
#include "Stats.h"
#include "Wrapper.h"
#include "Process.h"
#include "SU_Passwd.h"
#include "Signals.h"
#include "Timer.h"
#include "util.h"
#include "log.h"
#include "crc32.h"
#include "Param.h"
#include "SymbolTable.h"
#include "Feeling.h"
#include "cstring.h"
#include "Memory.h"
#include "CachedFile.h"
#include "HostMap.h"
#include "AtomicFile.h"
#include "Timezone.h"
#include "Worldclock.h"
#include "Category.h"
#include "Conn.h"
#include "ConnUser.h"
#include "ConnResolv.h"
#include "bufprintf.h"
#include "helper.h"
#include "OnlineUser.h"
#include "coredump.h"
#include "my_fcntl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <sys/types.h>


extern StringList *banished;

char *param_file = NULL;
int nologin_active = 1;


static void write_pidfile(void) {
AtomicFile *f;

	if ((f = openfile(PARAM_PID_FILE, "w")) != NULL) {
		fprintf(f->f, "%lu\n", (unsigned long)getpid());
		closefile(f);
	}
}

static void check_nologin(void) {
char filename[MAX_PATHLEN];

	bufprintf(filename, sizeof(filename), "%s/%s", PARAM_CONFDIR, NOLOGIN_FILE);
	if (file_exists(filename)) {
		nologin_active = 1;
		printf("NOTE: nologin is active, users will not be able to login\n\n");
	} else
		nologin_active = 0;				/* users can login */
}

void exit_program(int reboot) {
	deinit_Signal();

	if (reboot)
		log_msg("exit_program(): rebooting");
	else
		log_msg("exit_program(): shutting down");

/*
	neatly close the main listening socket
*/
	if (AllConns != NULL) {
		shut_allconns();
		sleep(2);
	}
	if (save_Stats(&stats, PARAM_STAT_FILE))
		log_err("failed to save stats");

	killall_process();					/* kill helper procs like the resolver */
	unlink(PARAM_PID_FILE);				/* shutdown, remove pidfile */
	exit(reboot);
}

void usage(void) {
	printf("options:\n"
		"  -f <file>  Specify parameters file (default: etc/param)\n"
		"  -d         Don't run as daemon (for debuggers)\n"
	);
	printf("\n"
		"bbs100 is usually started using the 'bbs' script rather than calling\n"
		"the binary directly. Instead of using command line arguments, the\n"
		"parameters are specified in the socalled param file.\n"
	);
	exit(1);
}

int main(int argc, char **argv) {
int debugger = 0;
char buf[MAX_LONGLINE];

	if (argv[0][0] != '(') {
		char *old_argv0, *new_argv0 = "(bbs100 main)";

		old_argv0 = argv[0];
		argv[0] = new_argv0;
		execv(old_argv0, argv);			/* restart with new name */

		fprintf(stderr, "bbs100: startup failed\n");
		exit(-1);
	}
	Enter(main);

	rtc = time(NULL);

	if (init_Memory()) {
		fprintf(stderr, "bbs100: out of memory (?)\n");
		exit(-1);
	}
	printf("%s\n", print_copyright(SHORT, "main", buf, MAX_LONGLINE));
	printf("bbs100 comes with ABSOLUTELY NO WARRANTY. This is free software.\n"
		"For details, see the GNU General Public License.\n\n");

	if (argc > 1) {
		int c;

		while((c = getopt(argc, argv, "f:dh")) != -1) {
			switch(c) {
				case 'f':
					param_file = cstrdup(optarg);
					break;

				case 'd':
					debugger = 1;
					break;

				case '?':
				case 'h':
					usage();
					break;

				default:
					usage();
			}
		}
		if (param_file == NULL && optind < argc)
			param_file = cstrdup(argv[optind]);
	}
	if (param_file == NULL)
		param_file = cstrdup("etc/param");

	if (param_file == NULL) {
		fprintf(stderr, "bbs100: out of memory (?)\n");
		exit(-1);
	}
	sleep(2);							/* display banner */

#ifdef SETVBUF_REVERSED
	setvbuf(stdout, _IOLBF, NULL, 256);
	setvbuf(stderr, _IOLBF, NULL, 256);
#else
	setvbuf(stdout, NULL, _IOLBF, 256);
	setvbuf(stderr, NULL, _IOLBF, 256);
#endif
	gen_crc32_table();

	init_Param();
	path_strip(param_file);
	printf("loading param file %s ... ", param_file);
	if (load_Param(param_file)) {
		printf("failed\n");
		exit(-1);
	} else
		printf("ok\n");
	check_Param();
	print_Param();

	umask(PARAM_UMASK);

	if (chdir(PARAM_BASEDIR)) {
		fprintf(stderr, "bbs100: failed to change directory to basedir '%s'\n", PARAM_BASEDIR);
		exit(-1);
	}
	write_pidfile();
	if (savecore() < 0)
		printf("failed to save core dump under %s\n", PARAM_CRASHDIR);

	init_Signal(debugger);
	bbs_init_process();

	if (init_FileCache()) {
		fprintf(stderr, "bbs100: failed to initialize file cache\n");
		exit_program(SHUTDOWN);
	}
	if (init_screens()) {
		fprintf(stderr, "bbs100: failed to initialize screens\n");
		exit_program(SHUTDOWN);
	}
	printf("loading stat_file %s ... ", PARAM_STAT_FILE);
	printf("%s\n", (load_Stats(&stats, PARAM_STAT_FILE) != 0) ? "failed" : "ok");

	printf("loading hostmap %s ... ", PARAM_HOSTMAP_FILE);
	printf("%s\n", (load_HostMap() != 0) ? "failed" : "ok");

	printf("loading hosts_access %s ... ", PARAM_HOSTS_ACCESS_FILE);
	printf("%s\n", (load_Wrapper(PARAM_HOSTS_ACCESS_FILE) != 0) ? "failed" : "ok");

	printf("loading banished_file %s ... ", PARAM_BANISHED_FILE);
	printf("%s\n", ((banished = load_StringList(PARAM_BANISHED_FILE)) == NULL) ? "failed" : "ok");

	printf("loading su_passwd_file %s ... ", PARAM_SU_PASSWD_FILE);
	printf("%s\n", (load_SU_Passwd(PARAM_SU_PASSWD_FILE) == 0) ? "ok" : "failed");

#ifdef DEBUG
	printf("loading symbol table %s ... ", PARAM_SYMTAB_FILE);
	printf("%s\n", (load_SymbolTable(PARAM_SYMTAB_FILE) != 0) ? "failed" : "ok");
#endif

	printf("loading feelings from %s ... ", PARAM_FEELINGSDIR);
	printf("%s\n", (init_Feelings() != 0) ? "failed" : "ok");

	printf("loading default timezone %s ... ", PARAM_DEFAULT_TIMEZONE);
	printf("%s\n", (init_Timezone() != 0) ? "failed" : "ok");

	init_Worldclock();
	init_Category();

	if (init_Room()) {
		printf("fatal: failed to initialize the rooms message system\n");
		exit_program(SHUTDOWN);
	}
	if (init_OnlineUser()) {
		printf("fatal: Failed to initialize the online users hash\n");
		exit_program(SHUTDOWN);
	}
	init_crypt();					/* init salt table for passwd encryption */
	init_helper();					/* reset helping hands */

	check_nologin();

	if (init_ConnUser())			/* startup inet */
		exit_program(SHUTDOWN);

	if (debugger)
		printf("running under debugger, signal handling disabled\n\n");

	if (init_log(debugger))			/* start logging to files */
		exit_program(SHUTDOWN);

 	log_info("bbs restart");
 	log_auth("bbs restart");

	if (debugger)
		log_msg("running under debugger");

	init_ConnResolv();				/* start the asynchronous name resolver */

	stats.uptime = rtc = time(NULL);

	if (nologin_active)
		log_warn("nologin is active");

	mainloop();
	exit_program(SHUTDOWN);			/* clean shutdown */
	Return 0;
}

/* EOB */
