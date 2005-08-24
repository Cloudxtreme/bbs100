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
#include "Signal.h"
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
#include "OnlineUser.h"
#include "AtomicFile.h"
#include "Timezone.h"
#include "Worldclock.h"
#include "Category.h"
#include "ConnUser.h"
#include "ConnResolv.h"
#include "bufprintf.h"
#include "BinAlloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
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

/*
	save core dumps in the directory under log/crash/
*/
static int savecore(void) {
struct stat statbuf;

	if (!stat("core", &statbuf)) {
		char filename[MAX_PATHLEN];
		struct tm *tm;
		int i;

		if ((tm = localtime(&statbuf.st_ctime)) == NULL)
			return 0;

		bufprintf(filename, MAX_PATHLEN, "%s/core.%04d%02d%02d", PARAM_CRASHDIR, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
		path_strip(filename);
		i = 1;
		while(!stat(filename, &statbuf) && i < 10) {
			bufprintf(filename, MAX_PATHLEN, "%s/core.%04d%02d%02d-%d", PARAM_CRASHDIR, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, i);
			path_strip(filename);
			i++;
		}
		if (rename("core", filename) == -1)
			printf("savecore(): failed to save core as %s\n\n", filename);
		else
			printf("saving core as %s\n\n", filename);
	}
	return 0;
}

static void goto_background(void) {
pid_t pid;

#ifdef TIOCNOTTY
int fd;

	if ((fd = open("/dev/tty", O_WRONLY 	/* detach from tty */

#ifdef O_NOCTTY
| O_NOCTTY
#endif
	)) != -1) {
		ioctl(fd, TIOCNOTTY);
		close(fd);
	}
#endif	/* TIOCNOTTY */

	pid = fork();							/* go to background */
	if (pid == (pid_t)-1L) {
		log_err("failed to fork()");
		exit_program(SHUTDOWN);
	}
	if (pid != (pid_t)0L)					/* parent: exit */
		exit(0);

	write_pidfile();						/* our pid has changed */

	setsid();								/* become new process group leader */

	pid = fork();
	if (pid == (pid_t)-1L) {
		log_err("failed to fork()");
		exit_program(SHUTDOWN);
	}
	if (pid != (pid_t)0L)					/* parent: exit */
		exit(0);

	write_pidfile();
}

void exit_program(int reboot) {
int code = 0;

	deinit_Signal();

	if (reboot)
		log_msg("exit_program(): rebooting");
	else
		log_msg("exit_program(): shutting down");

	if (save_Stats(&stats, PARAM_STAT_FILE))
		log_err("failed to save stats");

	killall_process();					/* kill helper procs like the resolver */

	if (reboot) {
		execlp(PARAM_PROGRAM_MAIN, PARAM_PROGRAM_MAIN, NULL);	/* reboot */
		log_err("reboot failed");
		code = -1;
	}
	unlink(PARAM_PID_FILE);				/* shutdown, remove pidfile */

	exit(code);
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
int debugger = 0, i;
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

	if (init_Memory()) {
		fprintf(stderr, "bbs100: out of memory (?)\n");
		exit(-1);
	}
	if (init_BinAlloc()) {
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

	rtc = time(NULL);

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

	if (!PARAM_HAVE_MEMCACHE)
		deinit_MemCache();

	if (!PARAM_HAVE_BINALLOC)
		disable_BinAlloc();

	umask(PARAM_UMASK);

	if (chdir(PARAM_BASEDIR)) {
		fprintf(stderr, "bbs100: failed to change directory to basedir '%s'\n", PARAM_BASEDIR);
		exit(-1);
	}
	write_pidfile();

	savecore();

	if (!debugger)
		init_Signal();

	bbs_init_process();

	if (init_FileCache()) {
		fprintf(stderr, "bbs100: failed to initialize file cache\n");
		exit(-1);
	}
	if (init_screens()) {
		fprintf(stderr, "bbs100: failed to initialize screens\n");
		exit(-1);
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

	printf("loading symbol table %s ... ", PARAM_SYMTAB_FILE);
	printf("%s\n", (load_SymbolTable(PARAM_SYMTAB_FILE) != 0) ? "failed" : "ok");

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
		printf("fatal: failed to initialize the online users hash\n");
		exit_program(SHUTDOWN);
	}
	init_crypt();					/* init salt table for passwd encryption */

	if (init_ConnUser())			/* startup inet */
		exit_program(SHUTDOWN);

	if (debugger)
		printf("running under debugger, signal handling disabled\n"
			"running under debugger, not going to background\n"
			"\n"
		);

	init_log();						/* start logging to files */

 	log_info("bbs restart");
	log_entry(stderr, "bbs restart", 'I', NULL);

	if (!debugger)
		goto_background();
	else
		log_msg("running under debugger");

	for(i = 0; i < NUM_TYPES; i++)
		log_debug("init_Memory(): sizeof(%s) == %d", Types_table[i].type, Types_table[i].size);

	init_ConnResolv();

	stats.uptime = rtc = time(NULL);

	nologin_active = 0;				/* users can login */
	mainloop();
	exit_program(SHUTDOWN);			/* clean shutdown */
	Return 0;
}

/* EOB */
