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
	util.c	WJ99
*/

#include "config.h"
#include "util.h"
#include "debug.h"
#include "edit.h"
#include "cstring.h"
#include "mydirentry.h"
#include "cstrerror.h"
#include "CachedFile.h"
#include "Param.h"
#include "access.h"
#include "Memory.h"
#include "memset.h"
#include "OnlineUser.h"
#include "locale_system.h"
#include "make_dir.h"
#include "bufprintf.h"
#include "state_room.h"

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

char *Default_Symbols = DEFAULT_SYMBOLS;


int Out(User *usr, char *str) {
	if (usr == NULL || str == NULL || !*str)
		return 0;

	if (usr->display == NULL && (usr->display = new_Display()) == NULL)
		return 0;

	if (usr->text == NULL && (usr->text = new_StringIO()) == NULL)
		return 0;

	if (usr->runtime_flags & RTF_BUFFER_TEXT)
		return put_StringIO(usr->text, str);

	return Out_text(usr->conn->output, usr, str, &usr->display->cpos, &usr->display->line, -1, 0);
}

/*
	- it takes the cursor position into account for <hline> and <center> tags
	- when max_lines > -1, can display a limited number of lines
	  (used in --More-- prompt reading)
	- returns position in the string where it stopped
	- if dev is NULL, it produces no output, but does run the function and update the cursor pos
	- force_auto_color_off is used for hline tags, that really don't like auto-coloring

	- is_symbol says if it's a symbol that needs auto-coloring
	- dont_auto_color is for controlling exceptions to the rule of auto-coloring
*/
int Out_text(StringIO *dev, User *usr, char *str, int *cpos, int *lines, int max_lines, int force_auto_color_off) {
char buf[MAX_COLORBUF], c;
int pos, n, dont_auto_color, color, is_symbol;

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

/* user-defined auto-coloring symbols */

		if ((usr->flags & (USR_ANSI|USR_DONT_AUTO_COLOR)) == USR_ANSI) {
			char *symbol_str;

			symbol_str = (usr->symbols == NULL) ? Default_Symbols : usr->symbols;
			if (cstrchr(symbol_str, c) != NULL) {
				is_symbol = 1;
/*
	auto-coloring a single dot is ugly, but multiple ones is fun
*/
				if (c == '.' && str[1] != '.' && (pos > 0 && str[-1] != '.'))
					dont_auto_color = AUTO_COLOR_FORCED;
			}
		}
/*
	word-wrap in display function
	Charset1 contains characters that break the line
*/
		if (cstrchr(Wrap_Charset1, c) != NULL) {
			if (*cpos + word_len(str+1) >= usr->display->term_width) {
				if (c != ' ') {
					if (is_symbol && !dont_auto_color) {
						auto_color(usr, buf, MAX_COLORBUF);
						put_StringIO(dev, buf);
					}
					write_StringIO(dev, str, 1);

					if (is_symbol && !dont_auto_color) {
						restore_colorbuf(usr, usr->color, buf, MAX_COLORBUF);
						put_StringIO(dev, buf);
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
	pretty word-wrap: Charset2 contains characters that wrap along with the line
	mind that the < character is also used for long color codes
*/
			if (c != '<' && cstrchr(Wrap_Charset2, c) != NULL) {
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

			case KEY_CTRL('L'):				/* clear screen */
				if (usr->flags & (USR_ANSI|USR_BOLD))
					write_StringIO(dev, "\x1b[1;1H\x1b[2J", 10);
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
						bufprintf(buf, sizeof(buf), "\x1b[1;%dm", color);
					else
						bufprintf(buf, sizeof(buf), "\x1b[%dm", color);
					put_StringIO(dev, buf);
					dont_auto_color = AUTO_COLOR_FORCED;
				}
				break;

			case KEY_CTRL('K'):
				str++;
				if (!*str)
					break;

				print_hotkey(usr, *str, buf, sizeof(buf), cpos);
				put_StringIO(dev, buf);

				dont_auto_color = force_auto_color_off;
				break;

			case KEY_CTRL('N'):
				if (usr->flags & USR_ANSI) {
					bufprintf(buf, sizeof(buf), "\x1b[0;%dm", color_table[usr->colors[BACKGROUND]].value+10);
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
				if (n > 0)
					dont_auto_color = AUTO_COLOR_FORCED;
				str += n;
				pos += n;
				break;

			default:
				if (is_symbol && !dont_auto_color) {
					auto_color(usr, buf, MAX_COLORBUF);
					put_StringIO(dev, buf);
				}
				write_StringIO(dev, &c, 1);
				(*cpos)++;

				if (is_symbol && !dont_auto_color) {
					restore_colorbuf(usr, usr->color, buf, MAX_COLORBUF);
					put_StringIO(dev, buf);
				}
				if (!is_symbol)
					dont_auto_color = force_auto_color_off;
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
				str += skip_long_color_code(str)-1;
				break;

			case KEY_CTRL('X'):
			case '\r':
				return -10000;		/* fool him */

			case ' ':
			case '\n':
			case '\t':
				return len;

			default:
				if (cstrchr(Wrap_Charset1, *str) != NULL) {
					len++;
					return len;
				}
				if (cstrchr(Wrap_Charset2, *str) != NULL)
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
char colorbuf[MAX_COLORBUF], buf[PRINT_BUF], *p;

	if (usr == NULL || code == NULL || !*code || cpos == NULL || lines == NULL)
		return 0;

	for(i = 0; i < NUM_COLORS; i++) {
		if (i == HOTKEY)
			continue;

		bufprintf(colorbuf, sizeof(colorbuf), "<%s>", color_table[i].name);

		if (!cstrnicmp(code, colorbuf, strlen(colorbuf))) {
			if (!(usr->flags & USR_ANSI))
				return strlen(colorbuf)-1;

			c = usr->color = color_table[i].key;

			color = Ansi_Color(usr, c);
			if (usr->flags & USR_BOLD)
				bufprintf(buf, sizeof(buf), "\x1b[1;%dm", color);
			else
				bufprintf(buf, sizeof(buf), "\x1b[%dm", color);
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
			bufprintf(buf, sizeof(buf), "\x1b[1;%dm", color);
		else
			bufprintf(buf, sizeof(buf), "\x1b[%dm", color);
		put_StringIO(dev, buf);
		return 6;
	}
*/
	if (!cstrnicmp(code, "<hotkey>", 8)) {
		c = code[8];
		if (!c)
			return 7;

		print_hotkey(usr, c, buf, sizeof(buf), cpos);
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
				bufprintf(buf, sizeof(buf), "\x1b[1;%dm%c\x1b[1;%dm", color_table[usr->colors[HOTKEY]].value, c, Ansi_Color(usr, usr->color));
			else
				bufprintf(buf, sizeof(buf), "\x1b[%dm%c\x1b[%dm", color_table[usr->colors[HOTKEY]].value, c, Ansi_Color(usr, usr->color));

			(*cpos)++;
		} else {
			bufprintf(buf, sizeof(buf), "<%c>", c);
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
			bufprintf(buf, sizeof(buf), "\x1b[0;%dm", color_table[usr->colors[BACKGROUND]].value+10);
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
			auto_color(usr, colorbuf, MAX_COLORBUF);
			put_StringIO(dev, colorbuf);
		}
		write_StringIO(dev, "<", 1);
		(*cpos)++;

		if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
			restore_colorbuf(usr, usr->color, colorbuf, MAX_COLORBUF);
			put_StringIO(dev, colorbuf);
		}
		return 3;
	}
	if (!cstrnicmp(code, "<gt>", 4)) {
		if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
			auto_color(usr, colorbuf, MAX_COLORBUF);
			put_StringIO(dev, colorbuf);
		}
		write_StringIO(dev, ">", 1);
		(*cpos)++;

		if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
			restore_colorbuf(usr, usr->color, colorbuf, MAX_COLORBUF);
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
		cstrncpy(buf, code, c);
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
		cstrncpy(buf, code, c);
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
		auto_color(usr, colorbuf, MAX_COLORBUF);
		put_StringIO(dev, colorbuf);
	}
	write_StringIO(dev, "<", 1);
	(*cpos)++;

	if ((usr->flags & USR_ANSI) && !(usr->flags & USR_DONT_AUTO_COLOR) && !dont_auto_color) {
		restore_colorbuf(usr, usr->color, colorbuf, MAX_COLORBUF);
		put_StringIO(dev, colorbuf);
	}
	return 0;
}

/*
	construct hotkey string into buf
	buf must be large enough (MAX_COLORBUF should do)

	cpos is the cursor position on the display

	USR_BOLD_HOTKEYS has a complex meaning;
	if USR_BOLD is set, then BOLD_HOTKEYS means we want faint hotkeys,
	if BOLD is NOT set, then BOLD_HOTKEYS means we want bold hotkeys

	Same goes for USR_HOTKEY_BRACKETS;
	if USR_ANSI is set, then HOTKEY_BRACKETS means we want em
	if ANSI is not set, then HOTKEY_BRACKETS means we don't want em

	(This is mainly due to backward compatibility with existing users;
	their BOLD_HOTKEYS flag will be clear, but you will want them to have
	bold hotkeys if they log in anyway [The other way to solve this is
	to use 2 flags {one for ANSI terminals and one for dumb terminals},
	I use just 1 and link it to BOLD])
*/
void print_hotkey(User *usr, char c, char *buf, int buflen, int *cpos) {
int len;

	if (usr == NULL || buf == NULL || buflen <= 0 || cpos == NULL)
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
			len += bufprintf(buf+len, buflen - len, ";%d;%dm", color_table[usr->colors[BACKGROUND]].value+10, color_table[usr->colors[HOTKEY]].value);
		else
			buf[len++] = 'm';
	}
	if (((usr->flags & (USR_ANSI|USR_HOTKEY_BRACKETS)) == (USR_ANSI|USR_HOTKEY_BRACKETS))
		|| ((usr->flags & (USR_ANSI|USR_HOTKEY_BRACKETS)) == 0)) {
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
			len += bufprintf(buf+len, buflen - len, ";%d;%dm", color_table[usr->colors[BACKGROUND]].value+10, Ansi_Color(usr, usr->color));
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
		cstrncpy(dest, str, bufsize);
		return;
	}
	l = p - str;
	if (l >= bufsize) {
		cstrncpy(dest, str, bufsize);
		return;
	}
	if (l > 0)
		cstrncpy(dest, str, l+1);
	else
		*dest = 0;

	p += 7;

	n = strlen(p);
	if (n > 0) {
		int cl, c;

		cl = color_strlen(dest);
		c = color_strlen(p);

		while(l + n < bufsize && cl < width) {
			cstrcat(dest, p, bufsize);
			l += n;
			cl += c;
		}
	}
/*
	remaining part: this is commented out because it has a habit of chopping op
	color codes, giving ugly results

	n = bufsize - l;
	if (n > 0)
		cstrncpy(dest+l, p, n);
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
		cstrncpy(dest, str, bufsize);
		return;
	}
	l = p - str;
	if (l >= bufsize) {
		cstrncpy(dest, str, bufsize);
		return;
	}
	if (l > 0)
		cstrncpy(dest, str, l);
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
		if (n > 0)
			cstrncpy(dest+l, p, n);
	} else
		cstrcpy(dest+l, p, bufsize - l);

	while((p = cstristr(dest, "<center>")) != NULL)		/* filter out duplicate codes */
		memmove(p, p+8, strlen(p+8)+1);
}


int skip_long_color_code(char *code) {
int i;
char colorbuf[MAX_COLORBUF];

	if (code == NULL || !*code || *code != '<')
		return 0;

	for(i = 0; i < NUM_COLORS; i++) {
		if (i == HOTKEY)
			continue;

		bufprintf(colorbuf, sizeof(colorbuf), "<%s>", color_table[i].name);
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
	convert key code to long color code
	some key codes do not have a long equivalent

	flags can be USR_SHORT_DL_COLORS, which means that the color codes should be
	represented in short format
*/
int short_color_to_long(char c, char *buf, int max_len, int flags) {
int i;

	if (buf == NULL || max_len <= 0)
		return -1;

	buf[0] = 0;
	switch(c) {
		case KEY_CTRL('Q'):				/* don't auto-color this string */
			break;

		case KEY_CTRL('X'):
			break;

		case KEY_CTRL('A'):
			cstrcpy(buf, "<beep>", max_len);
			break;

		case KEY_CTRL('L'):				/* clear screen */
			break;

		case KEY_CTRL('Z'):
		case KEY_CTRL('R'):
		case KEY_CTRL('G'):
		case KEY_CTRL('Y'):
		case KEY_CTRL('B'):
		case KEY_CTRL('M'):
		case KEY_CTRL('C'):
		case KEY_CTRL('W'):
			for(i = 0; i < NUM_COLORS; i++) {
				if (color_table[i].key == c) {
					if (flags & USR_SHORT_DL_COLORS) {
/*
	black is Ctrl-Z, but actually has to entered as Ctrl-K
	see also edit_color() in edit.c

	(this is confusing, but black is blacK, while the K is for hotkeys)
*/
						if (c == KEY_CTRL('Z'))
							c = KEY_CTRL('K');

						bufprintf(buf, max_len, "^%c", c + 'A' - 1);
					} else {
						bufprintf(buf, max_len, "<%s>", color_table[i].name);
						cstrlwr(buf);
					}
					break;
				}
			}
			break;

		case KEY_CTRL('K'):
			cstrcpy(buf, "<hotkey>", max_len);
			break;

		case KEY_CTRL('N'):
			cstrcpy(buf, "<normal>", max_len);
			break;

		case KEY_CTRL('D'):
			cstrcpy(buf, "<default>", max_len);
			break;

		default:
			if (c >= ' ' && c <= '~' && max_len >= 2) {
				buf[0] = c;
				buf[1] = 0;
			}
	}
	return 0;
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
void auto_color(User *usr, char *colorbuf, int buflen) {
int color;

	if (usr == NULL || colorbuf == NULL || !(usr->flags & USR_ANSI))
		return;

	*colorbuf = 0;
	if (usr->flags & USR_DONT_AUTO_COLOR)
		return;

	color = color_key_index(usr->color);
	switch(color) {
		case BLACK:
		case RED:
		case GREEN:
		case YELLOW:
		case BLUE:
		case MAGENTA:
		case CYAN:
		case WHITE:
			color = color_table[usr->symbol_colors[color]].value;
			break;

		default:
			color = color_table[usr->symbol_colors[WHITE]].value;
	}
	if (usr->flags & USR_BOLD)
		bufprintf(colorbuf, buflen, "\x1b[1;%d;%dm", color_table[usr->colors[BACKGROUND]].value+10, color);
	else
		bufprintf(colorbuf, buflen, "\x1b[0;%d;%dm", color_table[usr->colors[BACKGROUND]].value+10, color);
}

/*
	only a helper function for Out_text() ... use restore_color() instead
	color should be a CTRL_KEY() value like usr->color
*/
void restore_colorbuf(User *usr, int color, char *colorbuf, int buflen) {
	if (usr == NULL || colorbuf == NULL)
		return;

	*colorbuf = 0;
	if (usr->flags & USR_ANSI) {
		color = Ansi_Color(usr, color);
		if (usr->flags & USR_BOLD)
			bufprintf(colorbuf, buflen, "\x1b[1;%d;%dm", color_table[usr->colors[BACKGROUND]].value+10, color);
		else
			bufprintf(colorbuf, buflen, "\x1b[0;%d;%dm", color_table[usr->colors[BACKGROUND]].value+10, color);
	}
}

/*
	restore a previously saved usr->color (which is a KEY_CTRL() color character)

	using restore_colorbuf() here doesn't work, because the auto-coloring
	would color the escape sequence string
*/
void restore_color(User *usr, int color) {
char buf[2];

	if (usr == NULL)
		return;

	*buf = (char)color;
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

void default_symbol_colors(User *usr) {
	if (usr == NULL)
		return;

	usr->symbol_colors[BLACK] = BLUE;
	usr->symbol_colors[RED] = YELLOW;
	usr->symbol_colors[GREEN] = YELLOW;
	usr->symbol_colors[YELLOW] = WHITE;
	usr->symbol_colors[BLUE] = CYAN;
	usr->symbol_colors[MAGENTA] = YELLOW;
	usr->symbol_colors[CYAN] = WHITE;
	usr->symbol_colors[WHITE] = YELLOW;
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

int file_exists(char *filename) {
struct stat statbuf;

	if (filename == NULL || !*filename)
		return 0;

	if (!stat(filename, &statbuf))
		return 1;

	return 0;
}

int user_exists(char *name) {
char filename[MAX_PATHLEN];

	if (is_guest(name))
		return 1;

	bufprintf(filename, sizeof(filename), "%s/%c/%s/UserData", PARAM_USERDIR, *name, name);
	path_strip(filename);
	return file_exists(filename);
}

void system_broadcast(int overrule, char *msg) {
User *u;
void (*say)(User *, char *, ...);
char buf[PRINT_BUF];
struct tm *tm;

	log_info(msg);

	if (overrule)
		say = Print;
	else
		say = Tell;

	for(u = AllUsers; u != NULL; u = u->next) {
		tm = user_time(u, (time_t)0UL);
		if ((u->flags & USR_12HRCLOCK) && tm->tm_hour > 12)
			tm->tm_hour -= 12;

		bufprintf(buf, sizeof(buf), "\n<beep><white>*** <yellow>System message received at %d:%02d <white>***<red>\n"
			"%s\n", tm->tm_hour, tm->tm_min, msg);

		if (u->curr_room != NULL && (u->curr_room->flags & ROOM_CHATROOM) && !(u->runtime_flags & RTF_BUSY))
			chatroom_tell_user(u, buf);
		else
			say(u, buf);
	}
}

/*
	WARNING: return value is static
	(because gmtime() returns a static value)
*/
struct tm *tz_time(Timezone *tz, time_t tt) {
struct tm *tm;
time_t the_time;

	the_time = tz_time_t(tz, tt);
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

		if (tz->transitions != NULL && tz->num_trans > 0) {
/* shift into the next transition if needed; daylight savings or not */
			while((tz->curr_idx+1) < tz->num_trans && tz->transitions[tz->curr_idx+1].when <= rtc)
				tz->curr_idx++;

			tz_type = tz->transitions[tz->curr_idx].type_idx;
			if (tz_type < 0 || tz_type >= tz->num_types)
				tz_type = 0;
		} else
			tz_type = 0;

		if (tz->types != NULL && tz->num_types > 0)
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
	Note: date_str should be large enough (MAX_LINE will do)
*/
char *print_date(User *usr, time_t tt, char *date_str, int buflen) {
	if (date_str == NULL || buflen <= 0)
		return NULL;

	return lc_system->print_date(lc_system, user_time(usr, tt), (usr == NULL) ? 0 : usr->flags & USR_12HRCLOCK, date_str, buflen);
}

/*
	Note: buf must be large enough (MAX_LINE bytes should do)
*/
char *print_total_time(unsigned long total, char *buf, int buflen) {
	if (buf == NULL || buflen <= 0)
		return NULL;

	return lc_system->print_total_time(lc_system, total, buf, buflen);
}


/*
	Note: buf must be large enough (at least MAX_NUMBER)
*/
char *print_number(unsigned long ul, char *buf, int buflen) {
	if (buf == NULL || buflen <= 0)
		return NULL;

	return lc_system->print_number(lc_system, ul, buf, buflen);
}

/*
	print_number() with '1st', '2nd', '3rd', '4th', ... extension
	Note: buf must be large enough (at least 25 bytes)
*/
char *print_numberth(unsigned long ul, char *buf, int buflen) {
	if (buf == NULL || buflen <= 0)
		return NULL;

	return lc_system->print_numberth(lc_system, ul, buf, buflen);
}

/*
	Note: buf must be large enough (MAX_NAME + 3 + strlen(obj) + some ...)
	obj may be NULL
*/
char *possession(char *name, char *obj, char *buf, int buflen) {
	if (buf == NULL || buflen <= 0)
		return NULL;

	return lc_system->possession(lc_system, name, obj, buf, buflen);
}

unsigned long get_mail_top(char *username) {
char buf[MAX_PATHLEN], *p;
DIR *dirp;
struct dirent *direntp;
unsigned long maxnum = 0UL, n;

	bufprintf(buf, sizeof(buf), "%s/%c/%s/", PARAM_USERDIR, *username, username);
	path_strip(buf);
	if ((dirp = opendir(buf)) == NULL)
		return maxnum;

	while((direntp = readdir(dirp)) != NULL) {
		p = direntp->d_name;
		if (*p >= '0' && *p <= '9') {
			n = cstrtoul(p, 10);
			if (n > maxnum)
				maxnum = n;
		}
	}
	closedir(dirp);
	return maxnum;
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
	bufprintf(buf, sizeof(buf), "%s/", PARAM_TRASHDIR);
	path_strip(buf);
	if (strncmp(buf, dirname, strlen(buf)) || cstrstr(dirname, "..") != NULL)
		return -1;

	cstrcpy(buf, dirname, MAX_PATHLEN);
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

		cstrcpy(bufp, direntp->d_name, MAX_PATHLEN);
		unlink(buf);		/* note: trash/ is not cached ; it's ok not to use unlink_file() */
	}
	closedir(dirp);
	return remove_dir(dirname);
}

int mkdir_p(char *pathname) {
char *p, errbuf[MAX_LINE];

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
		if (make_dir(pathname, (mode_t)0750) == -1) {
			*p = '/';
			p++;
			if (errno == EEXIST)
				continue;

			log_err("mkdir_p(): failed to create directory %s : %s", pathname, cstrerror(errno, errbuf, MAX_LINE));
			return -1;
		}
		*p = '/';
		p++;
	}
	if (make_dir(pathname, (mode_t)0750) == -1) {
		if (errno == EEXIST)
			return 0;

		log_err("mkdir_p(): failed to create directory %s : %s", pathname, cstrerror(errno, errbuf, MAX_LINE));
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
	prints the entries in raw_list to create a menu in columns
	this routine looks a lot like the one that formats the wide who-list

	flags is FORMAT_NUMBERED|FORMAT_NO_UNDERSCORES
*/
void print_columns(User *usr, StringList *raw_list, int flags) {
int term_width, total, cols, rows, i, j, buflen, len, max_width, idx;
StringList *sl, *sl_cols[16];
char buf[MAX_LINE*4], format[MAX_LINE], filename[MAX_PATHLEN], *p;

	if (usr == NULL || raw_list == NULL)
		return;

	term_width = usr->display->term_width;
	if (term_width >= MAX_LONGLINE)
		term_width = MAX_LONGLINE;

	total = 0;
	max_width = 10;
	for(sl = raw_list; sl != NULL; sl = sl->next) {
		len = strlen(sl->str);
		if (len > max_width)
			max_width = len;
		total++;
	}
	if (flags & FORMAT_NUMBERED)
		bufprintf(format, sizeof(format), "%c%%3d %c%%-%ds", (char)color_by_name("green"), (char)color_by_name("yellow"), max_width);
	else
		bufprintf(format, sizeof(format), "%c%%-%ds", (char)color_by_name("yellow"), max_width);

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

			cstrcpy(filename, sl_cols[i]->str, MAX_PATHLEN);

			if (flags & FORMAT_NO_UNDERSCORES) {
				p = filename;
				while((p = cstrchr(p, '_')) != NULL)
					*p = ' ';
			}
			if (flags & FORMAT_NUMBERED)
				buflen += bufprintf(buf+buflen, sizeof(buf) - buflen, format, idx, filename);
			else
				buflen += bufprintf(buf+buflen, sizeof(buf) - buflen, format, filename);
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
		Put(usr, buf);
	}
}

/*
	set a flag that tells Out() to write output to usr->text rather than
	to the connection device

	this is convenient for setting up read_text() and read_menu()
*/
void buffer_text(User *usr) {
	if (usr == NULL)
		return;

	if (usr->runtime_flags & RTF_BUFFER_TEXT)
		return;

	free_StringIO(usr->text);
	usr->runtime_flags |= RTF_BUFFER_TEXT;
}

void clear_buffer(User *usr) {
	if (usr == NULL)
		return;

	free_StringIO(usr->text);
	usr->runtime_flags &= ~RTF_BUFFER_TEXT;
}

void clear_screen(User *usr) {
char cls[2];

	if (usr == NULL)
		return;

	if (usr->runtime_flags & RTF_BUFFER_TEXT)
		return;

	if (!(usr->flags & (USR_ANSI|USR_BOLD)))
		return;

	cls[0] = KEY_CTRL('L');
	cls[1] = 0;

	Put(usr, cls);
}

/*
	return an allocated list to names we've talked with

	the caller must listdestroy_StringList(talked_to)
*/
StringList *make_talked_to(User *usr) {
PList *pl;
StringList *talked_to, *sl;
BufferedMsg *m;

	if (usr == NULL || usr->history == NULL)
		return NULL;

	talked_to = NULL;

	for(pl = rewind_PList(usr->history); pl != NULL; pl = pl->next) {
		m = (BufferedMsg *)pl->p;
		if ((m->flags & BUFMSG_TYPE) == BUFMSG_ONESHOT)
			continue;

		if (m->from[0] && strcmp(usr->name, m->from) && !in_StringList(usr->enemies, m->from)
			&& !in_StringList(talked_to, m->from))
			(void)add_StringList(&talked_to, new_StringList(m->from));

		for(sl = m->to; sl != NULL; sl = sl->next)
			if (sl->str != NULL && sl->str[0] && strcmp(usr->name, sl->str)
				&& !in_StringList(usr->enemies, sl->str)
				&& !in_StringList(talked_to, sl->str))
				(void)add_StringList(&talked_to, new_StringList(sl->str));
	}
	return talked_to;
}


/* EOB */
