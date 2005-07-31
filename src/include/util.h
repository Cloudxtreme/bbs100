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
	util.h	WJ99
*/

#ifndef UTIL_H_WJ99
#define UTIL_H_WJ99 1

#include <config.h>

#include "User.h"
#include "Timezone.h"
#include "log.h"
#include "sys_time.h"

#include <stdarg.h>
#include <time.h>

#define RND_STR(x)			((x)[rtc % (sizeof(x)/sizeof(char *))])

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
#define NUM_COLORS		9

#define AUTO_COLOR_FORCED		1	/* there's a color code in front of the symbol, forcing the color */

#define FORMAT_NUMBERED			1	/* number the entries in print_columns() */
#define FORMAT_NO_UNDERSCORES	2	/* convert underscores to spaces */

typedef struct {
	char *name;
	int value;
	char key;
} ColorTable;

int Out(User *, char *);
int Out_text(StringIO *, User *, char *, int *, int *, int, int);
int word_len(char *);
int hackerz_mode(int);
int color_by_name(char *);
int long_color_code(StringIO *, User *, char *, int *, int *, int, int);
void print_hotkey(User *, char, char *, int *);
void expand_hline(char *, char *, int, int);
void expand_center(char *, char *, int, int);
int skip_long_color_code(char *);
int color_strlen(char *);
int color_index(char *, int);
void auto_color(User *, char *);
void restore_colorbuf(User *, int, char *);
void restore_color(User *, int);
int Ansi_Color(User *, int);
int Color_Ansi(User *, int);
int color_key_index(int);
void default_colors(User *);
void wipe_line(User *);
int yesno(User *, char, char);
int user_exists(char *);
int next_helping_hand(User *);
User *check_helping_hand(User *);
void system_broadcast(int, char *);
struct tm *tz_time(Timezone *, time_t);
struct tm *user_time(User *, time_t);
char *print_date(User *, time_t, char *);
char *print_total_time(unsigned long, char *);
char *print_number(unsigned long, char *);
char *print_numberth(unsigned long, char *);
char *possession(char *, char *, char *);
char *room_name(User *, Room *, char *);
unsigned long get_mail_top(char *);
char *get_basename(char *);
int rm_rf_trashdir(char *);
int mkdir_p(char *);
char *path_strip(char *);
long fread_int32(FILE *);

StringList *StringIO_to_StringList(StringIO *);
int StringList_to_StringIO(StringList *sl, StringIO *);
void print_columns(User *usr, StringList *raw_list, int flags);
void buffer_text(User *);
void clear_buffer(User *);
StringList *make_talked_to(User *);

extern ColorTable color_table[NUM_COLORS];

#endif	/* UTIL_H_WJ99 */

/* EOB */
