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
	util.c	WJ99
*/

#include "config.h"
#include "util.h"
#include "debug.h"
#include "edit.h"
#include "cstring.h"
#include "mydirentry.h"
#include "strerror.h"
#include "strtoul.h"
#include "CachedFile.h"
#include "Param.h"
#include "access.h"
#include "Memory.h"
#include "OnlineUser.h"
#include "locale_system.h"
#include "mkdir.h"
#include "source_sum.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#define HACK_CHANCE	((rand() % 20) < 4)


ColorTable color_table[NUM_COLORS] = {
	{ "Black",		30,	KEY_CTRL('Z')	},
	{ "Red",		31,	KEY_CTRL('R')	},
	{ "Green",		32,	KEY_CTRL('G')	},
	{ "Yellow",		33,	KEY_CTRL('Y')	},
	{ "Blue",		34,	KEY_CTRL('B')	},
	{ "Magenta",	35,	KEY_CTRL('M')	},
	{ "Cyan",		36,	KEY_CTRL('C')	},
	{ "White",		37,	KEY_CTRL('W')	},
	{ "Hotkeys",	33, KEY_CTRL('K')	}
};

static char last_helping_hand[MAX_NAME] = "";


int Out(User *usr, char *str) {
	if (usr == NULL || str == NULL || !*str)
		return 0;

	if (usr->display == NULL && (usr->display = new_Display()) == NULL)
		return 0;

	return Out_text(usr->conn->output, usr, str, &usr->display->cpos, &usr->display->line, -1, 0);
}

/*
	- it takes the cursor position into account for <hline> and <center> tags
	- when max_lines > -1, can display a limited number of lines
	  (used in --More-- prompt reading)
	- returns position in the string where it stopped
	- if dev is NULL, it produces no output, but does run the function and update the cursor pos
	- force_auto_color_off is used for hline tags, that really don't like auto-coloring

	- do_auto_color says if it's OK to auto-color symbols
	- dont_auto_color is for controlling exceptions to the rule of auto-coloring
*/
int Out_text(StringIO *dev, User *usr, char *str, int *cpos, int *lines, int max_lines, int force_auto_color_off) {
char buf[20], c;
int pos, n, do_auto_color = 0, dont_auto_color, color, is_symbol;

	if (usr == NULL || usr->display == NULL || str == NULL || cpos == NULL || lines == NULL)
		return 0;

	if (max_lines > -1 && *lines >= max_lines)
		return 0;

	dont_auto_color = force_auto_color_off;

	pos = 0;
	while(*str) {
		is_symbol = 0;
		pos++;
		c = *str;
		if ((usr->flags & USR_HACKERZ) && HACK_CHANCE)
			c = hackerz_mode(c);

/*
	word-wrap in display function
	CHARSET1 contains characters that break the line

	we also like auto-coloring symbols
*/
		if (cstrchr(WRAP_CHARSET1, c) != NULL) {
			if (c != ' ')
				is_symbol = 1;

/*
	for some characters auto-coloring can be straight ugly
	e.g, the space, dot, question mark
*/
			if (c != ' ' && c != '?' && (usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
				if (c == '.') {
/*
	auto-coloring a single dot is ugly, but multiple ones is fun
*/
					if (str[1] == '.' || (pos > 0 && str[-1] == '.'))
						do_auto_color = 1;
					else
						do_auto_color = 0;
				} else
					do_auto_color = 1;
			}
			if (*cpos + word_len(str+1) >= usr->display->term_width) {
				if (c != ' ') {
					if (do_auto_color) {
						auto_color(usr, buf);
						put_StringIO(dev, buf);
					}
					write_StringIO(dev, str, 1);

					if (do_auto_color) {
						restore_colorbuf(usr, usr->color, buf);
						put_StringIO(dev, buf);
						do_auto_color = 0;
					}
				}
				if (str[1] == ' ') {
					str++;
					pos++;
				}
				put_StringIO(dev, "\r\n");
				*cpos = 0;
				(*lines)++;
				if (max_lines > -1 && *lines >= max_lines)
					return pos;

				dont_auto_color = force_auto_color_off;

				if (*str)
					str++;
				continue;
			}
		} else {
/*
	pretty word-wrap: CHARSET2 contains characters that wrap along with the line
	mind that the < character is also used for long color codes
*/
			if (c == '@' || (c != '<' && cstrchr(WRAP_CHARSET2, c) != NULL)) {
				is_symbol = 1;
				if (c != '#' && (usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color)
					do_auto_color = 1;

				if (*cpos + word_len(str+1) >= usr->display->term_width) {
					write_StringIO(dev, "\r\n", 2);
					*cpos = 0;
					(*lines)++;
					if (max_lines > -1 && *lines >= max_lines)
						return pos-1;

					dont_auto_color = force_auto_color_off;
				}
			}
		}
		switch(c) {
			case '\b':
				write_StringIO(dev, "\b", 1);
				if (*cpos)
					(*cpos)--;

				dont_auto_color = force_auto_color_off;
				break;

			case '\n':
				write_StringIO(dev, "\r\n", 2);
				*cpos = 0;

				(*lines)++;
				if (max_lines > -1 && *lines >= max_lines)
					return pos;

				dont_auto_color = force_auto_color_off;
				break;

			case KEY_CTRL('Q'):								/* don't auto-color this string */
				dont_auto_color = AUTO_COLOR_FORCED;
				break;

			case KEY_CTRL('X'):
				write_StringIO(dev, "\r", 1);
				*cpos = 0;

				dont_auto_color = force_auto_color_off;
				break;

			case KEY_CTRL('A'):
				if (usr->flags & USR_BEEP)
					write_StringIO(dev, "\a", 1);

				dont_auto_color = force_auto_color_off;
				break;

			case KEY_CTRL('Z'):
			case KEY_CTRL('R'):
			case KEY_CTRL('G'):
			case KEY_CTRL('Y'):
			case KEY_CTRL('B'):
			case KEY_CTRL('M'):
			case KEY_CTRL('C'):
			case KEY_CTRL('W'):
/*			case KEY_CTRL('F'):		*/
				if (usr->flags & USR_ANSI) {
					usr->color = c;
					color = Ansi_Color(usr, c);
					if (usr->flags & USR_BOLD)
						sprintf(buf, "\x1b[1;%dm", color);
					else
						sprintf(buf, "\x1b[%dm", color);
					put_StringIO(dev, buf);
					dont_auto_color = AUTO_COLOR_FORCED;
				}
				break;

			case KEY_CTRL('K'):
				str++;
				if (!*str)
					break;

				print_hotkey(usr, *str, buf, cpos);
				put_StringIO(dev, buf);

				dont_auto_color = force_auto_color_off;
				break;

			case KEY_CTRL('N'):
				if (usr->flags & USR_ANSI) {
					sprintf(buf, "\x1b[0;%dm", color_table[usr->colors[BACKGROUND]].value+10);
					put_StringIO(dev, buf);
				} else
					if (usr->flags & USR_BOLD)
						put_StringIO(dev, "\x1b[0m");

				if (usr->flags & USR_BOLD)
					put_StringIO(dev, "\x1b[1m");

				dont_auto_color = force_auto_color_off;
				break;

			case KEY_CTRL('D'):
				if (usr->flags & (USR_ANSI | USR_BOLD))
					put_StringIO(dev, "\x1b[0m");

				dont_auto_color = force_auto_color_off;
				break;

/* long codes are specified as '<yellow>', '<beep>', etc. */

			case '<':
				n = long_color_code(dev, usr, str, cpos, lines, max_lines, dont_auto_color);
				if (n <= 0)
					dont_auto_color = force_auto_color_off;
				else
					dont_auto_color = AUTO_COLOR_FORCED;
				str += n;
				pos += n;
				break;

			default:
				if (do_auto_color) {
					auto_color(usr, buf);
					put_StringIO(dev, buf);
				}
				write_StringIO(dev, &c, 1);
				(*cpos)++;

				if (dont_auto_color != AUTO_COLOR_FORCED || !is_symbol)
					dont_auto_color = force_auto_color_off;

				if (do_auto_color) {
					restore_colorbuf(usr, usr->color, buf);
					put_StringIO(dev, buf);
					do_auto_color = 0;
				}
		}
		if (*str)
			str++;
	}
/*	Flush(usr);		the buffering code and mainloop() will flush for us */
	return pos;
}

/*
	try to determine the length of the next word
	this is used by the word wrapper in Out()
	do NOT try to use this function for anything else, because it is
	way crappy
	scans at most only 15 characters ahead
*/
int word_len(char *str) {
int len;

	len = 0;

	while(*str) {
		switch(*str) {
			case '<':
				str += skip_long_color_code(str);
				break;

			case KEY_CTRL('X'):
			case '\r':
				return -1000;		/* fool him */

			case ' ':
			case '\n':
			case '\t':
			case '-':				/* OK to break on a dash */
				return len;

			default:
				if (cstrchr(WRAP_CHARSET1, *str) != NULL || cstrchr(WRAP_CHARSET2, *str) != NULL)
					return len;

/* count as printable character (this is NOT always the case, however) */
				if (*str >= ' ' && *str <= '~') {
					len++;
					if (len >= 15)
						return len;
				}
		}
		if (*str)
			str++;
	}
	return len;
}

/*
	convert character to character in hackerz mode
*/
int hackerz_mode(int c) {
	switch(c) {
		case 'o':
		case 'O':
			c = '0';
			break;

		case 'a':
		case 'A':
			if ((rand() % 10) > 5)
				c = '@';
			else
				c = '4';
			break;

		case 'e':
		case 'E':
			c = '3';
			break;

		case 's':
		case 'S':
			switch(rand() % 10) {
				case 0: c = 'z'; break;
				case 1: c = 'Z'; break;
				case 2: c = '$'; break;
				case 3: c = '5'; break;
				case 4: c = 'S'; break;
			}
			break;

		case 't':
		case 'T':
			c = '+';
			break;

		case 'i':
		case 'I':
		case 'l':
		case 'L':
			c = '1';
			break;

		case '9':
			c = 'g';
			break;

		case 'g':
		case 'G':
			c = '9';
			break;

		default:
			if (c >= 'a' && c <= 'z')
				c = ctoupper(c);
			else
				c = ctolower(c);
	}
	return c;
}

int color_by_name(char *name) {
int i;

	for(i = 0; i < NUM_COLORS; i++) {
		if (i == HOTKEY)
			continue;

		if (!cstricmp(name, color_table[i].name))
			return color_table[i].key;
	}
/*
	if (!cstricmp(name, "blink") || !cstricmp(name, "flash"))
		return KEY_CTRL('F');
*/
	if (!cstricmp(name, "hotkey"))
		return KEY_CTRL('K');

	if (!cstricmp(name, "beep"))
		return KEY_CTRL('A');

	if (!cstricmp(name, "normal"))
		return KEY_CTRL('N');

	if (!cstricmp(name, "default"))
		return KEY_CTRL('D');

	if (!cstricmp(name, "cr"))
		return KEY_CTRL('X');

	if (!cstricmp(name, "lt"))
		return '<';

	if (!cstricmp(name, "gt"))
		return '>';

	return 0;
}

/*
	long color codes look like '<yellow>', '<white>', '<hotkey>', etc.

	parsing the long color code is a slow function...
	but it makes programming with color coded strings a lot easier task

	it's more or less sorted in order of most frequent appearance to make
	it less slow

	cpos is the cursor position, which is used in <hline> and <center> tags
	for <hline>, the function recurses with Out_text()

	if dev is NULL, it produces no output
*/
int long_color_code(StringIO *dev, User *usr, char *code, int *cpos, int *lines, int max_lines, int dont_auto_color) {
int i, c, color;
char colorbuf[20], buf[PRINT_BUF], *p;

	if (usr == NULL || code == NULL || !*code || cpos == NULL || lines == NULL)
		return 0;

	for(i = 0; i < NUM_COLORS; i++) {
		if (i == HOTKEY)
			continue;

		sprintf(colorbuf, "<%s>", color_table[i].name);

		if (!cstrnicmp(code, colorbuf, strlen(colorbuf))) {
			if (!(usr->flags & USR_ANSI))
				return strlen(colorbuf)-1;

			c = usr->color = color_table[i].key;

			color = Ansi_Color(usr, c);
			if (usr->flags & USR_BOLD)
				sprintf(buf, "\x1b[1;%dm", color);
			else
				sprintf(buf, "\x1b[%dm", color);
			put_StringIO(dev, buf);
			return strlen(colorbuf)-1;
		}
	}
/*
	Blinking is really irritating...

	if (!cstrnicmp(code, "<flash>", 7) || !cstrnicmp(code, "<blink>", 7)) {
		if (!(usr->flags & USR_ANSI))
			return 6;

		usr->color = KEY_CTRL('F');
		color = Ansi_Color(usr, KEY_CTRL('F'));
		if (usr->flags & USR_BOLD)
			sprintf(buf, "\x1b[1;%dm", color);
		else
			sprintf(buf, "\x1b[%dm", color);
		put_StringIO(dev, buf);
		return 6;
	}
*/
	if (!cstrnicmp(code, "<hotkey>", 8)) {
		c = code[8];
		if (!c)
			return 7;

		print_hotkey(usr, c, buf, cpos);
		put_StringIO(dev, buf);
		return 8;
	}
	if (!cstrnicmp(code, "<key>", 5)) {
		c = code[5];
		if (!c)
			return 4;
/*
	Don't do this; the <key> code is used in the Help files to keep this from happening

		if (usr->flags & USR_UPPERCASE_HOTKEYS)
			c = ctoupper(c);
*/
		if (usr->flags & USR_ANSI) {
			if (usr->flags & USR_BOLD)
				sprintf(buf, "\x1b[1;%dm%c\x1b[1;%dm", color_table[usr->colors[HOTKEY]].value, c, Ansi_Color(usr, usr->color));
			else
				sprintf(buf, "\x1b[%dm%c\x1b[%dm", color_table[usr->colors[HOTKEY]].value, c, Ansi_Color(usr, usr->color));

			(*cpos)++;
		} else {
			sprintf(buf, "<%c>", c);
			*cpos += 3;
		}
		put_StringIO(dev, buf);
		return 5;
	}
	if (!cstrnicmp(code, "<beep>", 6)) {
		if (usr->flags & USR_BEEP)
			write_StringIO(dev, "\a", 1);
		return 5;
	}
	if (!cstrnicmp(code, "<normal>", 8)) {
		if (usr->flags & USR_ANSI) {
			sprintf(buf, "\x1b[0;%dm", color_table[usr->colors[BACKGROUND]].value+10);
			put_StringIO(dev, buf);
		} else
			if (usr->flags & USR_BOLD)
				put_StringIO(dev, "\x1b[0m");

		if (usr->flags & USR_BOLD)
			put_StringIO(dev, "\x1b[1m");
		return 7;
	}
	if (!cstrnicmp(code, "<default>", 9)) {
		if (usr->flags & (USR_ANSI | USR_BOLD))
			put_StringIO(dev, "\x1b[0m");
		return 8;
	}
	if (!cstrnicmp(code, "<lt>", 4)) {
		if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
			auto_color(usr, colorbuf);
			put_StringIO(dev, colorbuf);
		}
		write_StringIO(dev, "<", 1);
		(*cpos)++;

		if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
			restore_colorbuf(usr, usr->color, colorbuf);
			put_StringIO(dev, colorbuf);
		}
		return 3;
	}
	if (!cstrnicmp(code, "<gt>", 4)) {
		if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
			auto_color(usr, colorbuf);
			put_StringIO(dev, colorbuf);
		}
		write_StringIO(dev, ">", 1);
		(*cpos)++;

		if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
			restore_colorbuf(usr, usr->color, colorbuf);
			put_StringIO(dev, colorbuf);
		}
		return 3;
	}

/*
	there are two special codes for use in help files and stuff...
	<hline> and <center>

	especially the code for hline is cryptic, but the idea is that
	it fills the line to the width of the terminal
*/
	if (!cstrnicmp(code, "<hline>", 7)) {
		int l;

		code += 7;
		if (!*code)
			return 6;

		c = ((usr->display->term_width-1) > PRINT_BUF) ? PRINT_BUF : (usr->display->term_width-1);
		strncpy(buf, code, c);
		buf[c-1] = 0;
/*
	it stinks, but you have to remove all chars that can reset the cursor pos
*/
		p = buf;
		while(*p) {
			if (*p == KEY_CTRL('X') || *p == '\b')
				memmove(p, p+1, strlen(p+1)+1);
			else {
				if (*p == '\n') {		/* don't go over newlines */
					*p = 0;
					break;
				}
				p++;
			}
		}
		l = strlen(buf);
		i = color_strlen(buf);

		while(*cpos + i < usr->display->term_width-1)
			Out_text(dev, usr, buf, cpos, lines, max_lines, AUTO_COLOR_FORCED);	/* recurse */

		if (*cpos + i >= usr->display->term_width-1) {			/* 'partial put' of the remainder */
			buf[color_index(buf, c - *cpos)] = 0;
			Out_text(dev, usr, buf, cpos, lines, max_lines, AUTO_COLOR_FORCED);
		}
		return 6+l;
	}
	if (!cstrnicmp(code, "<center>", 8)) {
		code += 8;
		if (!*code)
			return 7;

		c = strlen(code);
		c = (c > PRINT_BUF) ? PRINT_BUF : c;
		strncpy(buf, code, c - 1);
		buf[c-1] = 0;

		if ((p = cstrchr(buf, '\n')) != NULL)		/* don't go over newlines */
			*p = 0;

		i = (usr->display->term_width-1)/2 - color_strlen(buf)/2 - *cpos;
		while(i > 0) {
			write_StringIO(dev, " ", 1);
			(*cpos)++;
			i--;
		}
		return 7;
	}
	if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
		auto_color(usr, colorbuf);
		put_StringIO(dev, colorbuf);
	}
	write_StringIO(dev, "<", 1);
	(*cpos)++;

	if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
		restore_colorbuf(usr, usr->color, colorbuf);
		put_StringIO(dev, colorbuf);
	}
	return 0;
}

/*
	construct hotkey string into buf
	buf must be large enough (20 chars should do)

	cpos is the cursor position on the display

	USR_BOLD_HOTKEYS has a complex meaning;
	if USR_BOLD is set, then BOLD_HOTKEYS means we want faint hotkeys,
	if BOLD is NOT set, then BOLD_HOTKEYS means we want bold hotkeys

	(This is mainly due to backward compatibility with existing users;
	their BOLD_HOTKEYS flag will be clear, but you will want them to have
	bold hotkeys if they log in anyway [The other way to solve this is
	to use 2 flags {one for ANSI terminals and one for dumb terminals},
	I use just 1 and link it to BOLD])
*/
void print_hotkey(User *usr, char c, char *buf, int *cpos) {
int len;

	if (usr == NULL)
		return;

	if (usr->flags & USR_UPPERCASE_HOTKEYS)
		c = ctoupper(c);

	buf[0] = 0;
	len = 0;
	if (usr->flags & (USR_ANSI|USR_BOLD_HOTKEYS)) {
		buf[len++] = '\x1b';
		buf[len++] = '[';

		if (usr->flags & USR_BOLD)
			buf[len++] = (usr->flags & USR_BOLD_HOTKEYS) ? '0' : '1';
		else
			buf[len++] = (usr->flags & USR_BOLD_HOTKEYS) ? '1' : '0';

		if (usr->flags & USR_ANSI)
			len += sprintf(buf+len, ";%dm", color_table[usr->colors[HOTKEY]].value);
		else
			buf[len++] = 'm';
	}
	if (usr->flags & USR_HOTKEY_BRACKETS) {
		buf[len++] = '<';
		buf[len++] = c;
		buf[len++] = '>';
		(*cpos) += 3;
	} else {
		buf[len++] = c;
		(*cpos)++;
	}
	if (usr->flags & (USR_ANSI|USR_BOLD_HOTKEYS)) {
		buf[len++] = '\x1b';
		buf[len++] = '[';

		if (usr->flags & USR_BOLD)
			buf[len++] = '1';
		else
			buf[len++] = '0';

		if (usr->flags & USR_ANSI)
			len += sprintf(buf+len, ";%dm", Ansi_Color(usr, usr->color));
		else
			buf[len++] = 'm';
	}
	buf[len] = 0;
}

/*
	expand the <hline> tag into a buffer

	Mind that the line may not fully be expanded, 'remainders' with broken color codes
	are really dreadful
*/
void expand_hline(char *str, char *dest, int bufsize, int width) {
char *p;
int l, n;

	if (str == NULL || dest == NULL || bufsize <= 0)
		return;

	if ((p = cstristr(str, "<hline>")) == NULL) {
		strncpy(dest, str, bufsize);
		dest[bufsize-1] = 0;
		return;
	}
	l = p - str;
	if (l >= bufsize) {
		strncpy(dest, str, bufsize-1);
		dest[bufsize-1] = 0;
		return;
	}
	if (l > 0)
		strncpy(dest, str, l);
	else
		l = 0;
	dest[l] = 0;

	p += 7;

	n = strlen(p);
	if (n > 0) {
		int cl, c;

		cl = color_strlen(dest);
		c = color_strlen(p);

		while(l + n < bufsize && cl < width) {
			strcat(dest, p);
			l += n;
			cl += c;
		}
	}
/*
	remaining part: this is commented out because it has a habit of chopping op
	color codes, giving ugly results

	n = bufsize - l;
	if (n > 0) {
		strncpy(dest+l, p, n);
		dest[bufsize-1] = 0;
	}
*/
	while((p = cstristr(dest, "<hline>")) != NULL)		/* filter out duplicate codes */
		memmove(p, p+7, strlen(p+7)+1);
}

/*
	expand the <center> tag into a buffer
	This tag doesn't go well together with other tags
*/
void expand_center(char *str, char *dest, int bufsize, int width) {
char *p;
int l, n;

	if (str == NULL || dest == NULL || bufsize <= 0)
		return;

	if  ((p = cstristr(str, "<center>")) == NULL) {
		strncpy(dest, str, bufsize);
		dest[bufsize-1] = 0;
		return;
	}
	l = p - str;
	if (l >= bufsize) {
		strncpy(dest, str, bufsize-1);
		dest[bufsize-1] = 0;
		return;
	}
	if (l > 0)
		strncpy(dest, str, l);
	else
		l = 0;
	dest[l] = 0;

	p += 8;

	n = width/2 - color_strlen(p)/2 - l;
	while(n > 0 && l < bufsize-1) {
		dest[l] = ' ';
		l++;
		n--;
	}
	dest[l] = 0;

	n = strlen(p);
	if (l + n >= bufsize) {
		n = bufsize - l;
		if (n > 0) {
			strncpy(dest+l, p, n);
			dest[bufsize-1] = 0;
		}
	} else
		strcpy(dest+l, p);

	while((p = cstristr(dest, "<center>")) != NULL)		/* filter out duplicate codes */
		memmove(p, p+8, strlen(p+8)+1);
}


/*
	this function is kind of lame :)
*/
int skip_long_color_code(char *code) {
int i;
char colorbuf[20];

	if (code == NULL || !*code || *code != '<')
		return 0;

	for(i = 0; i < NUM_COLORS; i++) {
		if (i == HOTKEY)
			continue;

		sprintf(colorbuf, "<%s>", color_table[i].name);
		if (!cstrnicmp(code, colorbuf, strlen(colorbuf)))
			return strlen(colorbuf);
	}
/*
	if (!cstrnicmp(code, "<flash>", 7) || !cstrnicmp(code, "<blink>", 7))
		return 7;
*/
	if (!cstrnicmp(code, "<hotkey>", 8))
		return 8;

	if (!cstrnicmp(code, "<key>", 5))
		return 5;

	if (!cstrnicmp(code, "<beep>", 6))
		return 6;

	if (!cstrnicmp(code, "<normal>", 8))
		return 8;

	if (!cstrnicmp(code, "<default>", 9))
		return 9;

	if (!cstrnicmp(code, "<lt>", 4))
		return 4;

	if (!cstrnicmp(code, "<gt>", 4))
		return 4;

	return 1;
}

int color_strlen(char *str) {
int len = 0, i;

	while(*str) {
		if (*str == '<') {
			i = skip_long_color_code(str);
			if (i == 1)
				len++;
			str += i;
		} else {
			if (*str >= ' ' && *str <= '~')
				len++;
			str++;
		}
	}
	return len;
}

/*
	return the 'wide' index of a 'screen' position within a color coded string
	e.g.
			buf[color_index(buf, 79)] = 0;
*/
int color_index(char *str, int pos) {
int cpos, i;

	if (str == NULL || pos < 0)
		return 0;

	cpos = 0;
	while(*str && pos > 0) {
		if (*str == '<') {
			i = skip_long_color_code(str);
			if (i == 1)
				pos--;

			str += i;
			cpos += i;
		} else {
			if (*str >= ' ' && *str <= '~')
				pos--;

			str++;
			cpos++;
		}
	}
	return cpos;
}

/*
	automatically change color for symbols
*/
void auto_color(User *usr, char *colorbuf) {
int color;

	if (usr == NULL || colorbuf == NULL)
		return;

	*colorbuf = 0;
	if (usr->flags & USR_DONT_AUTO_COLOR)
		return;

	color = color_key_index(usr->color);
	switch(color) {
		case BLACK:
			color = color_table[BLUE].key;
			break;

		case RED:
			color = color_table[YELLOW].key;
			break;

		case GREEN:
			color = color_table[YELLOW].key;
			break;

		case YELLOW:
			color = color_table[WHITE].key;
			break;

		case BLUE:
			color = color_table[CYAN].key;
			break;

		case MAGENTA:
			color = color_table[YELLOW].key;
			break;

		case CYAN:
			color = color_table[WHITE].key;
			break;

		case WHITE:
			color = color_table[WHITE].key;
			break;

		default:
			color = color_table[WHITE].key;
	}
	color = Ansi_Color(usr, color);

	if (usr->flags & USR_BOLD)
		sprintf(colorbuf, "\x1b[1;%dm", color);
	else
		sprintf(colorbuf, "\x1b[0;%dm", color);
}

/*
	only a helper function for Out_text() ... use restore_color() instead
	color should be a CTRL_KEY() value like usr->color
*/
void restore_colorbuf(User *usr, int color, char *colorbuf) {
	if (usr == NULL || colorbuf == NULL)
		return;

	*colorbuf = 0;
	if (usr->flags & USR_ANSI) {
		color = Ansi_Color(usr, color);
		if (usr->flags & USR_BOLD)
			sprintf(colorbuf, "\x1b[1;%dm", color);
		else
			sprintf(colorbuf, "\x1b[0;%dm", color);
	}
}

/*
	restore a previously saved usr->color

	using restore_colorbuf() here doesn't work, because the auto-coloring
	would color the escape sequence string
*/
void restore_color(User *usr, int color) {
char buf[2];

	if (usr == NULL)
		return;

	*buf = color;
	buf[1] = 0;
	Put(usr, buf);
}

int Ansi_Color(User *usr, int c) {
	if (usr == NULL)
		return 0;

	switch(c) {
		case KEY_CTRL('Z'):
			c = BLACK;
			break;

		case KEY_CTRL('R'):
			c = RED;
			break;

		case KEY_CTRL('G'):
			c = GREEN;
			break;

		case KEY_CTRL('Y'):
			c = YELLOW;
			break;

		case KEY_CTRL('B'):
			c = BLUE;
			break;

		case KEY_CTRL('P'):
		case KEY_CTRL('M'):
			c = MAGENTA;
			break;

		case KEY_CTRL('C'):
			c = CYAN;
			break;

		case KEY_CTRL('W'):
			c = WHITE;
			break;

		case KEY_CTRL('K'):
			c = HOTKEY;
			break;

		case KEY_CTRL('D'):
			return 33;

/*
		case KEY_CTRL('F'):
			return 5;
*/
		case KEY_CTRL('N'):
			return 0;

		default:
			c = 0;
	}
	return color_table[usr->colors[c]].value;
}

/*
	the reverse of Ansi_Color()
*/
int Color_Ansi(User *usr, int value) {
int i;

/*
	if (value == 5)
		return KEY_CTRL('F');
*/
	if (!value)
		return KEY_CTRL('N');

	for(i = 0; i < NUM_COLORS; i++) {
		if (color_table[usr->colors[i]].value == value)
			return color_table[usr->colors[i]].key;
	}
	return KEY_CTRL('G');
}

/*
	get the index to the color_table for a given CTRL_KEY() color
*/
int color_key_index(int key) {
int i;

	for(i = 0; i < NUM_COLORS; i++) {
		if (color_table[i].key == key)
			return i;
	}
	return GREEN;		/* just pick something, green or yellow will do */
}

void default_colors(User *usr) {
	if (usr == NULL)
		return;

	usr->colors[BACKGROUND] = BLACK;
	usr->colors[RED] = RED;
	usr->colors[GREEN] = GREEN;
	usr->colors[YELLOW] = YELLOW;
	usr->colors[BLUE] = BLUE;
	usr->colors[MAGENTA] = MAGENTA;
	usr->colors[CYAN] = CYAN;
	usr->colors[WHITE] = WHITE;
	usr->colors[HOTKEY] = YELLOW;
}

void wipe_line(User *usr) {
int i;

	if (usr == NULL)
		return;

	Writechar(usr, '\r');

	for(i = 0; i < usr->display->cpos; i++)
		Writechar(usr, ' ');

	Writechar(usr, '\r');
	usr->display->cpos = 0;
}

int yesno(User *usr, char c, char def) {
	if (usr == NULL)
		return YESNO_UNDEF;

	if (c == KEY_RETURN || c == ' ')
		c = def;

	if (c == 'y' || c == 'Y') {
		Put(usr, "Yes\n");
		return YESNO_YES;
	}
	if (c == 'n' || c == 'N') {
		Put(usr, "No\n");
		return YESNO_NO;
	}
	Put(usr, "\n");
	return YESNO_UNDEF;
}

int user_exists(char *name) {
char buf[MAX_LINE];
struct stat statbuf;

	if (is_guest(name))
		return 1;

	sprintf(buf, "%s/%c/%s/UserData", PARAM_USERDIR, *name, name);
	path_strip(buf);
	if (!stat(buf, &statbuf))
		return 1;

	return 0;
}

/*
	find next helping hand
	returns 0 on not found, 1 on found and usr->question_asked set

	This algorithm is not entirely round robin (since users log in and
	out all the time), but it is good enough
*/
int next_helping_hand(User *usr) {
User *u;

	if (usr == NULL)
		return 0;

	if (usr->question_asked != NULL) {
		Free(usr->question_asked);
		usr->question_asked = NULL;
	}
	if (last_helping_hand[0] && (u = is_online(last_helping_hand)) != NULL) {
		u = u->next;

		for(; u != NULL; u = u->next) {
			if (u == usr)
				continue;

			if ((u->flags & USR_HELPING_HAND)
				&& !(u->runtime_flags & RTF_LOCKED)
				&& (in_StringList(usr->enemies, u->name)) == NULL
				&& (in_StringList(u->enemies, usr->name)) == NULL) {
				strcpy(last_helping_hand, u->name);
				usr->question_asked = cstrdup(u->name);
				return 1;
			}
		}
	}

/* not found; search from beginning */

	for(u = AllUsers; u != NULL; u = u->next) {
		if (u == usr)
			continue;

		if ((u->flags & USR_HELPING_HAND)
			&& !(u->runtime_flags & RTF_LOCKED)
			&& (in_StringList(usr->enemies, u->name)) == NULL
			&& (in_StringList(u->enemies, usr->name)) == NULL) {
			strcpy(last_helping_hand, u->name);
			usr->question_asked = cstrdup(u->name);
			return 1;
		}
	}
	return 0;
}

/*
	make sure the HH asked is still online and still has HH status enabled
	returns NULL if not available, else the recipient user and usr->question_asked set
*/
User *check_helping_hand(User *usr) {
User *u;

	if (usr == NULL)
		return NULL;

	if (usr->question_asked == NULL) {
		if (!next_helping_hand(usr)) {
			Put(usr, "<red>Sorry, but currently there is no one available to help you\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;
			return NULL;
		} else
			Print(usr, "<green>The question goes to <yellow>%s\n", usr->question_asked);
	}
HH_is_online:
	while((u = is_online(usr->question_asked)) == NULL) {		/* HH logged off :P */
		Print(usr, "<yellow>%s<red> is no longer available to help you\n", usr->question_asked);

		if (!next_helping_hand(usr)) {
			Put(usr, "<red>Sorry, but currently there is no one available to help you\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;
			return NULL;
		}
		Print(usr, "<green>The question goes to <yellow>%s<green> instead\n", usr->question_asked);
	}
	if (!(u->flags & USR_HELPING_HAND)
		|| (u->runtime_flags & RTF_LOCKED)
		|| (in_StringList(usr->enemies, u->name)) != NULL
		|| (in_StringList(u->enemies, usr->name)) != NULL) {

		Print(usr, "<yellow>%s<red> is no longer available to help you\n", usr->question_asked);
		if (!next_helping_hand(usr)) {
			Put(usr, "<red>Sorry, but currently there is no one available to help you\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;
			return NULL;
		}
		Print(usr, "<green>The question goes to <yellow>%s<green> instead\n", usr->question_asked);
		goto HH_is_online;
	}
	listdestroy_StringList(usr->recipients);
	usr->recipients = new_StringList(usr->question_asked);
	return u;
}


void system_broadcast(int overrule, char *msg) {
User *u;
void (*func)(User *, char *, ...);
char buf[PRINT_BUF];
struct tm *tm;

	log_info(msg);

	if (overrule)
		func = Print;
	else
		func = Tell;

	for(u = AllUsers; u != NULL; u = u->next) {
		tm = user_time(u, (time_t)0UL);
		if ((u->flags & USR_12HRCLOCK) && tm->tm_hour > 12)
			tm->tm_hour -= 12;

		sprintf(buf, "\n<beep><white>*** <yellow>System message received at %d:%02d <white>***<red>\n"
			"%s\n", tm->tm_hour, tm->tm_min, msg);

		func(u, buf);
	}
}

/*
	WARNING: return value is static
	(because gmtime() returns a static value)
*/
struct tm *tz_time(Timezone *tz, time_t tt) {
struct tm *tm;
time_t the_time;

	if (tt == (time_t)0UL)
		the_time = rtc;
	else
		the_time = tt;

	if (tz != NULL) {
		int tz_type;

		if (tz->transitions != NULL)
			tz_type = tz->transitions[tz->curr_idx].type_idx;
		else
			tz_type = 0;

		the_time += tz->types[tz_type].gmtoff;
	}
	tm = gmtime(&the_time);
	return tm;
}

time_t tz_time_t(Timezone *tz, time_t tt) {
time_t the_time;

	if (tt == (time_t)0UL)
		the_time = rtc;
	else
		the_time = tt;

	if (tz != NULL) {
		int tz_type;

		if (tz->transitions != NULL)
			tz_type = tz->transitions[tz->curr_idx].type_idx;
		else
			tz_type = 0;

		the_time += tz->types[tz_type].gmtoff;
	}
	return the_time;
}

/*
	return 'localtime' for user
	(each user has its own timezone)

	WARNING: return value is static
*/
struct tm *user_time(User *usr, time_t tt) {
	if (usr != NULL)
		return tz_time(usr->tz, tt);

	return tz_time(NULL, tt);
}

/*
	Note: date_str should be large enough (80 bytes will do)
*/
char *print_date(User *usr, time_t tt, char *date_str) {
	if (date_str == NULL)
		return NULL;

	return lc_system->print_date(lc_system, user_time(usr, tt), (usr == NULL) ? 0 : usr->flags & USR_12HRCLOCK, date_str);
}

/*
	Note: buf must be large enough (MAX_LINE bytes should do)
*/
char *print_total_time(unsigned long total, char *buf) {
	if (buf == NULL)
		return NULL;

	return lc_system->print_total_time(lc_system, total, buf);
}


/*
	Note: buf must be large enough (at least 21 bytes)
*/
char *print_number(unsigned long ul, char *buf) {
	if (buf == NULL)
		return NULL;

	return lc_system->print_number(lc_system, ul, buf);
}

/*
	print_number() with '1st', '2nd', '3rd', '4th', ... extension
	Note: buf must be large enough (at least 25 bytes)
*/
char *print_numberth(unsigned long ul, char *buf) {
	if (buf == NULL)
		return NULL;

	return lc_system->print_numberth(lc_system, ul, buf);
}

/*
	Note: buf must be large enough (MAX_LINE bytes in size)
*/
char *possession(char *name, char *obj, char *buf) {
	if (buf == NULL)
		return NULL;

	return lc_system->possession(lc_system, name, obj, buf);
}

unsigned long get_mail_top(char *username) {
char buf[MAX_LINE], *p;
DIR *dirp;
struct dirent *direntp;
unsigned long maxnum = 0UL, n;

	sprintf(buf, "%s/%c/%s/", PARAM_USERDIR, *username, username);
	path_strip(buf);
	if ((dirp = opendir(buf)) == NULL)
		return maxnum;

	while((direntp = readdir(dirp)) != NULL) {
		p = direntp->d_name;
		if (*p >= '0' && *p <= '9') {
			n = strtoul(p, NULL, 10);
			if (n > maxnum)
				maxnum = n;
		}
	}
	closedir(dirp);
	return maxnum;
}

/*
	Note: buf must be large enough (MAX_LINE should do)
*/
char *room_name(User *usr, Room *r, char *buf) {
	if (buf == NULL)
		return NULL;

	*buf = 0;
	if (usr == NULL || r == NULL)
		return buf;

	if (usr->flags & USR_ROOMNUMBERS)
		sprintf(buf, "<white>%u <yellow>%s>", r->number, r->name);
	else
		sprintf(buf, "<yellow>%s>", r->name);
	return buf;
}

char *get_basename(char *path) {
char *p;

	if ((p = strrchr(path, '/')) == NULL)
		return path;
	p++;
	if (!*p)
		return path;
	return p;
}

/*
	rm -rf : first empty directory, then remove directory
*/
int rm_rf_trashdir(char *dirname) {
char buf[MAX_PATHLEN], *bufp;
DIR *dirp;
struct dirent *direntp;

	if (dirname == NULL || !*dirname)
		return -1;

/* safety check */
	sprintf(buf, "%s/", PARAM_TRASHDIR);
	path_strip(buf);
	if (strncmp(buf, dirname, strlen(buf)) || cstrstr(dirname, "..") != NULL)
		return -1;

	strcpy(buf, dirname);
	bufp = buf+strlen(buf)-1;
	if (*bufp != '/') {
		bufp++;
		*bufp = '/';
		bufp++;
		*bufp = 0;
	}
	if ((dirp = opendir(buf)) == NULL)
		return -1;

	while((direntp = readdir(dirp)) != NULL) {
/* check for '.' and '..' directory */
		if (direntp->d_name[0] == '.' && (!direntp->d_name[1]
			|| (direntp->d_name[1] == '.' && !direntp->d_name[2])))
			continue;

		strcpy(bufp, direntp->d_name);
		unlink(buf);		/* note: trash/ is not cached ; it's ok not to use unlink_file() */
	}
	closedir(dirp);
	return rmdir(dirname);
}

int mkdir_p(char *pathname) {
char *p;

	if (pathname == NULL)
		return -1;

	if (!*pathname)
		return 0;

	p = pathname;
	while((p = cstrchr(p, '/')) != NULL) {
		*p = 0;
		if (!*pathname) {
			*p = '/';
			p++;
			continue;
		}
		if (mkdir(pathname, (mode_t)0750) == -1) {
			*p = '/';
			p++;
			if (errno == EEXIST)
				continue;

			log_err("mkdir_p(): failed to create directory %s : %s", pathname, strerror(errno));
			return -1;
		}
		*p = '/';
		p++;
	}
	if (mkdir(pathname, (mode_t)0750) == -1) {
		if (errno == EEXIST)
			return 0;

		log_err("mkdir_p(): failed to create directory %s : %s", pathname, strerror(errno));
		return -1;
	}
	return 0;
}

/*
	Warning: modifies buffer
*/
char *path_strip(char *path) {
char *p;

	if (path == NULL)
		return NULL;

	p = path;
	while((p = cstrchr(p, '/')) != NULL) {
		while(p[1] == '/')
			memmove(p+1, p+2, strlen(p+1));
		p++;
	}
	return path;
}

long fread_int32(FILE *f) {
long l;
int c, i;

	l = 0L;
	for(i = 0; i < 4; i++) {
		if ((c = fgetc(f)) == -1)
			return -1L;

		l <<= 8;
		l |= (c & 255);
	}
	return l;
}

char *print_md5_digest(unsigned char sum[MD5_DIGITS], char *digest) {
int i;

	if (digest == NULL)
		return NULL;

	for(i = 0; i < MD5_DIGITS; i++)
		sprintf(digest+i*2, "%02x", sum[i] & 0xff);

	return digest;
}

StringList *StringIO_to_StringList(StringIO *s) {
StringList *sl;
char buf[PRINT_BUF];
int pos;

	pos = tell_StringIO(s);

	sl = NULL;
	while(gets_StringIO(s, buf, PRINT_BUF) != NULL)
		sl = add_StringList(&sl, new_StringList(buf));

	seek_StringIO(s, pos, STRINGIO_SET);

	sl = rewind_StringList(sl);
	return sl;
}

int StringList_to_StringIO(StringList *sl, StringIO *s) {
	if (s == NULL)
		return -1;

	while(sl != NULL) {
		put_StringIO(s, sl->str);
		write_StringIO(s, "\n", 1);
		sl = sl->next;
	}
	return 0;
}

/*
	writes the entries in raw_list to StringIO s to create the time zone menu
	this routine looks a lot like the one that formats the wide who-list

	- numbered is a flag saying if you want a numbered list or not
*/
void format_menu(StringIO *s, StringList *raw_list, int term_width, int numbered) {
int total, cols, rows, i, j, buflen, len, max_width, idx;
StringList *sl, *sl_cols[16];
char buf[MAX_LINE*4], format[50], filename[MAX_PATHLEN], *p;

	if (term_width >= MAX_LINE*3)
		term_width = MAX_LINE*3;

	total = 0;
	max_width = 10;
	for(sl = raw_list; sl != NULL; sl = sl->next) {
		len = strlen(sl->str);
		if (len > max_width)
			max_width = len;
		total++;
	}
	if (numbered)
		sprintf(format, "%c%%3d %c%%-%ds", (char)color_by_name("green"), (char)color_by_name("yellow"), max_width);
	else
		sprintf(format, "%c%%-%ds", (char)color_by_name("green"), max_width);

	cols = term_width / (max_width+6);
	if (cols < 1)
		cols = 1;
	else
		if (cols > 15)
			cols = 15;

	rows = total / cols;
	if (total % cols)
		rows++;

	memset(sl_cols, 0, sizeof(StringList *) * cols);

/* fill in array of pointers to columns */

	sl = raw_list;
	for(i = 0; i < cols; i++) {
		sl_cols[i] = sl;
		for(j = 0; j < rows; j++) {
			if (sl == NULL)
				break;

			sl = sl->next;
		}
	}

/* make the menu text */

	for(j = 0; j < rows; j++) {
		idx = j + 1;

		buf[0] = 0;
		buflen = 0;

		for(i = 0; i < cols; i++) {
			if (sl_cols[i] == NULL || sl_cols[i]->str == NULL)
				continue;

			strcpy(filename, sl_cols[i]->str);
			p = filename;
			while((p = cstrchr(p, '_')) != NULL)
				*p = ' ';

			if (numbered)
				buflen += sprintf(buf+buflen, format, idx, filename);
			else
				buflen += sprintf(buf+buflen, format, filename);
			idx += rows;

			if ((i+1) < cols) {
				buf[buflen++] = ' ';
				buf[buflen++] = ' ';
				buf[buflen] = 0;
			}
			sl_cols[i] = sl_cols[i]->next;
		}
		buf[buflen++] = '\n';
		buf[buflen] = 0;
		put_StringIO(s, buf);
	}
}

/* EOB */
