/*
    bbs100 1.2.1 WJ103
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
	Chatter18	WJ97
	main.c
*/

#include "config.h"
#include "debug.h"
#include "copyright.h"
#include "defines.h"
#include "main.h"
#include "inet.h"
#include "screens.h"
#include "Stats.h"
#include "Wrapper.h"
#include "Process.h"
#include "SU_Passwd.h"
#include "Signal.h"
#include "Timer.h"
#include "util.h"
#include "log.h"
#include "Param.h"
#include "SymbolTable.h"
#include "Feeling.h"
#include "cstring.h"
#include "Memory.h"
#include "CachedFile.h"
#include "HostMap.h"
#include "OnlineUser.h"
#include "AtomicFile.h"
#include "ZoneInfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

extern StringList *banished;

char *param_file;


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
		char filename[MAX_PATHLEN], *path;
		struct tm *tm;
		int i;

		tm = localtime(&statbuf.st_ctime);
		sprintf(filename, "core.%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
		path = path_join(PARAM_CRASHDIR, filename);
		i = 1;
		while(!stat(path, &statbuf) && i < 10) {
			sprintf(filename, "core.%04d%02d%02d-%d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, i);
			path = path_join(PARAM_CRASHDIR, filename);
			i++;
		}
		if (rename("core", path) == -1)
			printf("savecore(): failed to save core as %s\n\n", path);
		else
			printf("saving core as %s ...\n\n", path);
	}
	return 0;
}

static void goto_background(void) {
pid_t pid;
int fd;

	if ((fd = open("/dev/tty", O_WRONLY 	/* detach from tty */

#ifdef O_NOCTTY
| O_NOCTTY
#endif
	)) != -1) {
#ifdef TIOCNOTTY
		ioctl(fd, TIOCNOTTY);
#endif
		close(fd);
	}
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

	ignore_signals = 1;

	if (reboot) {
		log_msg("exit_program(): rebooting");
	} else {
		log_msg("exit_program(): shutting down");
	}
	if (main_socket > 0) {
		shutdown(main_socket, 2);
		close(main_socket);

		if (save_Stats(&stats, PARAM_STAT_FILE)) {
			log_err("failed to save stats");
		}
	}
/*	kill_process();	*/

	if (reboot) {
		execlp(PARAM_PROGRAM_MAIN, PARAM_PROGRAM_MAIN, NULL);	/* reboot */
		code = -1;												/* reboot failed */
	}
	unlink(PARAM_PID_FILE);				/* shutdown, remove pidfile */

	exit(code);
}

int main(int argc, char **argv) {
	if (argv[0][0] != '(') {
		char *old_argv0, *new_argv0 = "(bbs100 main)";

		old_argv0 = argv[0];
		argv[0] = new_argv0;
		execv(old_argv0, argv);			/* restart with new name */

		printf("startup failed\n");
		exit(-1);
	}
	Enter(main);

	if (init_Memory()) {
		printf("Out of memory (?)\n");
		exit(-1);
	}
	printf("%s\n", print_copyright(SHORT, "main"));
	printf("bbs100 comes with ABSOLUTELY NO WARRANTY. This is free software.\n"
		"For details, see the GNU General Public License.\n\n");
	sleep(2);

	if (argc > 1)
		param_file = cstrdup(argv[1]);
	else
		param_file = cstrdup("etc/param");

	if (param_file == NULL) {
		printf("Out of memory (?)\n");
		exit(-1);
	}
	rtc = time(NULL);
	umask(007);							/* allow owner and group, deny others */

#ifdef SETVBUF_REVERSED
	setvbuf(stdout, _IOLBF, NULL, 256);
	setvbuf(stderr, _IOLBF, NULL, 256);
#else
	setvbuf(stdout, NULL, _IOLBF, 256);
	setvbuf(stderr, NULL, _IOLBF, 256);
#endif
	init_Param();
	printf("loading param file %s ... ", param_file);
	if (load_Param(param_file)) {
		printf("failed, using defaults\n");
		init_Param();
	} else
		printf("ok\n");
	print_Param();

	write_pidfile();

	if (chdir(PARAM_BASEDIR)) {
		printf("failed to change directory to basedir '%s'\n", PARAM_BASEDIR);
		exit(-1);
	}
	savecore();
	init_Signal();

	if (init_FileCache()) {
		printf("failed to initialize file cache\n");
		exit(-1);
	}
	printf("loading gpl_screen %s ... ", PARAM_GPL_SCREEN);
	if ((gpl_screen = load_StringList(PARAM_GPL_SCREEN)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading mods_screen %s ... ", PARAM_MODS_SCREEN);
	if ((mods_screen = load_StringList(PARAM_MODS_SCREEN)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading login_screen %s ... ", PARAM_LOGIN_SCREEN);
	if ((login_screen = load_StringList(PARAM_LOGIN_SCREEN)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading logout_screen %s ... ", PARAM_LOGOUT_SCREEN);
	if ((logout_screen = load_StringList(PARAM_LOGOUT_SCREEN)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading motd_screen %s ... ", PARAM_MOTD_SCREEN);
	if ((motd_screen = load_StringList(PARAM_MOTD_SCREEN)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading crash_screen %s ... ", PARAM_CRASH_SCREEN);
	if ((crash_screen = load_StringList(PARAM_CRASH_SCREEN)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading first_login %s ... ", PARAM_FIRST_LOGIN);
	if ((first_login = load_StringList(PARAM_FIRST_LOGIN)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading stat_file %s ... ", PARAM_STAT_FILE);
	if (load_Stats(&stats, PARAM_STAT_FILE))
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading hostmap %s ... ", PARAM_HOSTMAP_FILE);
	if (load_HostMap(PARAM_HOSTMAP_FILE))
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading hosts_access %s ... ", PARAM_HOSTS_ACCESS_FILE);
	if (load_Wrapper(&wrappers, PARAM_HOSTS_ACCESS_FILE))
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading banished_file %s ... ", PARAM_BANISHED_FILE);
	if ((banished = load_StringList(PARAM_BANISHED_FILE)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading su_passwd_file %s ... ", PARAM_SU_PASSWD_FILE);
	if ((su_passwd = load_SU_Passwd(PARAM_SU_PASSWD_FILE)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading help_std %s ... ", PARAM_HELP_STD);
	if ((help_std = load_StringList(PARAM_HELP_STD)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading help_config %s ... ", PARAM_HELP_CONFIG);
	if ((help_config = load_StringList(PARAM_HELP_CONFIG)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading help_roomconfig %s ... ", PARAM_HELP_ROOMCONFIG);
	if ((help_roomconfig = load_StringList(PARAM_HELP_ROOMCONFIG)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading help_sysop %s ... ", PARAM_HELP_SYSOP);
	if ((help_sysop = load_StringList(PARAM_HELP_SYSOP)) == NULL)
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading symbol table %s ... ", PARAM_SYMTAB_FILE);
	if (load_SymbolTable(PARAM_SYMTAB_FILE))
		printf("failed\n");
	else
		printf("ok\n");

	printf("loading feelings from %s ... ", PARAM_FEELINGSDIR);
	if (init_Feelings())
		printf("failed\n");
	else
		printf("ok\n");

	init_ZoneInfo();

	if (init_Room()) {
		log_err("fatal: Failed to initialize the rooms message system");
		exit_program(SHUTDOWN);
	}
	if (init_OnlineUser()) {
		log_err("fatal: Failed to initialize the online users hash");
		exit_program(SHUTDOWN);
	}
	init_crypt();			/* init salt table for passwd encryption */

	if ((main_socket = inet_sock(PARAM_PORT_NUMBER)) < 0) {		/* startup inet */
		printf("failed to listen on port %d\n", PARAM_PORT_NUMBER);
		exit_program(SHUTDOWN);
	}
	printf("up and running pid %lu, listening at port %d\n", (unsigned long)getpid(), PARAM_PORT_NUMBER);

	init_log();				/* start logging to files */

	log_info("bbs restart");
	log_entry(stderr, "bbs restart", 'I', NULL);

/*
	Note: You will want to comment this line out when you compile with -g
	      in order to be able to use a debugger
*/
	goto_background();

	if (init_process())
		log_err("helper daemons startup failed");

	stats.uptime = rtc = time(NULL);

	mainloop();
	exit_program(SHUTDOWN);						/* clean shutdown */
	Return 0;
}

/* EOB */
