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
	Util.h	WJ99
*/

#ifndef UTIL_H_WJ99
#define UTIL_H_WJ99 1

#include <config.h>

#include "User.h"
#include "log.h"
#include "sys_time.h"

#include <stdarg.h>

#define RND_STR(x)			((x)[rtc % (sizeof(x)/sizeof(char *))])

#define ONE_SECOND			1
#define SECS_IN_MIN			(60 * ONE_SECOND)
#define SECS_IN_HOUR		(60 * SECS_IN_MIN)
#define SECS_IN_DAY			(24 * SECS_IN_HOUR)
#define SECS_IN_WEEK		(7 * SECS_IN_DAY)

#define UserError(a,b,c,d,e,f)	do {											\
		Put((a), "<red>ERROR: <yellow>" b "\n\n");								\
		log_err("%d %s %s%s: [%s] %s", (c), (d), (e), (f), (a)->name, (b));		\
	} while(0)

#ifdef __GNUC__
  #define Perror(x,y)		UserError((x), y, __LINE__, __FILE__, __PRETTY_FUNCTION__, "()")
#else
  #define Perror(x,y)		UserError((x), y, __LINE__, __FILE__, "", "")
#endif


#define YESNO_YES		1
#define YESNO_NO		0
#define YESNO_UNDEF		-1

#define OVERRULE		1

#define BLACK			0
#define BACKGROUND		0
#define RED				1
#define GREEN			2
#define YELLOW			3
#define BLUE			4
#define MAGENTA			5
#define CYAN			6
#define WHITE			7
#define HOTKEY			8

typedef struct {
	char *name;
	int value;
	char key;
} ColorTable;


void Put(User *, char *);
int color_by_name(char *);
int long_color_code(User *, char *);
int color_strlen(char *);
int Ansi_Color(User *, int);
void default_colors(User *);
int yesno(User *, char, char);
int user_exists(char *);
int next_helping_hand(User *);
User *check_helping_hand(User *);
void system_broadcast(int, char *);
char *print_date(User *, time_t);
char *print_total_time(unsigned long);
char *print_number(unsigned long);
char *print_numberth(unsigned long);
char *name_with_s(char *);
char *room_name(User *, Room *);
unsigned long get_mail_top(char *);
char *get_basename(char *);
int rm_rf_trashdir(char *);
int mkdir_p(char *);
char *path_join(char *, char *);
char *path_strip(char *);

extern ColorTable color_table[];
extern char *Months[];
extern char *Days[];

#endif	/* UTIL_H_WJ99 */

/* EOB */
