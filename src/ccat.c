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
	ccat for bbs100	WJ99

	Note: ccat has gotten a bit old. The Out() function in util.c of bbs100
	is much more sophisticated than the Put() presented here in ccat
	Still, ccat serves its purpose, but it's just that you know ...
*/

#include "config.h"
#include "copyright.h"
#include "cstring.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TERM_WIDTH		80

#define KEY_CTRL(x)		((x) - 'A' + 1)

#define KEY_ESC			0x1b
#define KEY_RETURN		'\r'
#define KEY_BS			0x7f
#define KEY_BEEP		7
#define KEY_TAB			'\t'
#define KEY_BACKTAB		'\\'

#define Writechar(x)	c = (x); write(fileno(stdout), &c, 1)
#define Write(x)		write(fileno(stdout), (x), strlen(x))

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


ColorTable color_table[] = {
	{ "Black",		30,	KEY_CTRL('Z')	},
	{ "Red",		31,	KEY_CTRL('R')	},
	{ "Green",		32,	KEY_CTRL('G')	},
	{ "Yellow",		33,	KEY_CTRL('Y')	},
	{ "Blue",		34,	KEY_CTRL('B')	},
	{ "Magenta",	35,	KEY_CTRL('M')	},
	{ "Cyan",		36,	KEY_CTRL('C')	},
	{ "White",		37,	KEY_CTRL('W')	},
	{ "Hotkeys",	33,	KEY_CTRL('K')	}
};

int usr_color = 37;
int usr_term_width = TERM_WIDTH;

int long_color_code(char *);
int color_strlen(char *);


int Ansi_Color(int c) {
	switch(c) {
		case KEY_CTRL('Z'): c = BLACK;		break;
		case KEY_CTRL('R'):	c = RED;		break;
		case KEY_CTRL('G'):	c = GREEN;		break;
		case KEY_CTRL('Y'):	c = YELLOW;		break;
		case KEY_CTRL('B'):	c = BLUE;		break;
		case KEY_CTRL('P'):
		case KEY_CTRL('M'):	c = MAGENTA;	break;
		case KEY_CTRL('C'):	c = CYAN;		break;
		case KEY_CTRL('W'):	c = WHITE;		break;

		case KEY_CTRL('K'): c = HOTKEY;		break;

		case KEY_CTRL('D'):	return 33;
		case KEY_CTRL('F'):	return 5;
		case KEY_CTRL('N'):	return 0;

		default:
			c = 0;
	}
	return color_table[c].value;
}

void Put(char *str) {
char buf[MAX_COLORBUF], c;

	if (str == NULL)
		return;

	while(*str) {
		c = *str;

		switch(c) {
			case '\n':
				Writechar('\r');
				Writechar('\n');
				break;

			case KEY_CTRL('X'):
				Writechar('\r');
				break;

			case KEY_CTRL('A'):
				Writechar('\a');
				break;

			case KEY_CTRL('Z'):
			case KEY_CTRL('R'):
			case KEY_CTRL('G'):
			case KEY_CTRL('Y'):
			case KEY_CTRL('B'):
			case KEY_CTRL('M'):
			case KEY_CTRL('C'):
			case KEY_CTRL('W'):
			case KEY_CTRL('F'):
				usr_color = Ansi_Color(c);
				bufprintf(buf, sizeof(buf), "\x1b[1;%dm", usr_color);
				Write(buf);
				break;

			case KEY_CTRL('K'):
				str++;
				if (!*str)
					break;

				bufprintf(buf, sizeof(buf), "\x1b[1;%dm%c\x1b[1;%dm", color_table[HOTKEY].value, *str, usr_color);
				Write(buf);
				break;

			case KEY_CTRL('N'):
				bufprintf(buf, sizeof(buf), "\x1b[0;%dm", color_table[BACKGROUND].value+10);
				Write(buf);
				Write("\x1b[1m");
				break;

			case KEY_CTRL('D'):
				Write("\x1b[0m");
				usr_color = 0;
				break;

/* long codes are specified as '<yellow>', '<beep>', etc. */

			case '<':
				str += long_color_code(str);
				break;

			default:
				Writechar(c);
		}
		if (*str)
			str++;
	}
}

/*
	long color codes look like '<yellow>', '<white>', '<hotkey>', etc.

	parsing the long color code is a slow function...
	but it makes programming with color coded strings a lot easier task

	it's more or less sorted in order of most frequent appearance to make
	it less slow
*/
int long_color_code(char *code) {
int i, c, colors;
char colorbuf[MAX_COLORBUF], buf[MAX_COLORBUF];

	colors = sizeof(color_table)/sizeof(ColorTable);
	for(i = 0; i < colors; i++) {
		if (i == HOTKEY)
			continue;

		bufprintf(colorbuf, sizeof(colorbuf), "<%s>", color_table[i].name);

		if (!cstrnicmp(code, colorbuf, strlen(colorbuf))) {
			c = color_table[i].key;

			usr_color = Ansi_Color(c);
			bufprintf(buf, sizeof(buf), "\x1b[1;%dm", usr_color);
			Write(buf);
			return strlen(colorbuf)-1;
		}
	}
/*
	Blinking is really irritating...

	if (!cstrnicmp(code, "<flash>", 7) || !cstrnicmp(code, "<blink>", 7)) {
		color = Ansi_Color(KEY_CTRL('F'));
		bufprintf(buf, sizeof(buf), "\x1b[1;%dm", color);
		Write(buf);
		return 6;
	}
*/
	if (!cstrnicmp(code, "<hotkey>", 8)) {
		c = code[8];
		if (!c)
			return 7;

		bufprintf(buf, sizeof(buf), "\x1b[1;%dm%c\x1b[1;%dm", color_table[HOTKEY].value, c, usr_color);
		Write(buf);
		return 8;
	}
	if (!cstrnicmp(code, "<beep>", 6)) {
		Writechar(KEY_BEEP);
		return 5;
	}
	if (!cstrnicmp(code, "<normal>", 8)) {
		bufprintf(buf, sizeof(buf), "\x1b[0;%dm", color_table[BACKGROUND].value+10);
		Write(buf);
		Write("\x1b[1m");
		return 7;
	}
	if (!cstrnicmp(code, "<default>", 9)) {
		Write("\x1b[0m");
		usr_color = 0;
		return 8;
	}
	if (!cstrnicmp(code, "<lt>", 4)) {
		Writechar('<');
		return 3;
	}
	if (!cstrnicmp(code, "<gt>", 4)) {
		Writechar('>');
		return 3;
	}

/*
	there are two special codes for use in help files and stuff...
	<hline> and <center>
*/
	if (!cstrnicmp(code, "<hline>", 7)) {
		char *p, *base;

		p = base = code + 7;
		if (*p) {
			while(*p && (*p < ' ' || *p > '~'))
				p++;
			base = p;

/*
	this is merely a check that it contains a valid line
	otherwise we might run into an endless loop later
*/
			while(*p) {
				if (*p >= ' ' && *p <= '~')
					break;
				p++;
			}
			if (!*p)
				return strlen(code)-1;

			p = base;
			if (*p) {
				int n;
/*
	here...
	this loop would be endless if we hadn't checked the string for
	valid characters before
	Note that you cannot use color codes after a <hline> tag
	because of the way this has been implemented
*/
				n = 1;
				while(n < usr_term_width) {
					if (*p >= ' ' && *p <= '~') {
						Writechar(*p);
						n++;
					}
					p++;
					if (!*p)
						p = base;
				}
				Writechar('\r');
				Writechar('\n');
			}
		}
		return strlen(code)-1;
	}
	if (!cstrnicmp(code, "<center>", 8)) {
		i = (usr_term_width-1)/2 - color_strlen(code+8)/2;
		while(i > 0) {
			Writechar(' ');
			i--;
		}
		return 7;
	}
	Writechar('<');
	return 0;
}

int skip_long_color_code(char *code) {
int colors, i;
char colorbuf[MAX_COLORBUF];

	if (code == NULL || !*code || *code != '<')
		return 0;

	colors = sizeof(color_table)/sizeof(ColorTable);
	for(i = 0; i < colors; i++) {
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

/*
	color_strlen() doesn't count the color codes
*/
int color_strlen(char *str) {
int len = 0;

	while(*str) {
		if (*str == '<')
			str += skip_long_color_code(str);
		else {
			if (*str >= ' ' && *str <= '~')
				len++;
			str++;
		}
	}
	return len;
}


int main(void) {
char buf[256];

	printf("%s", print_copyright(SHORT, "ccat", buf, 256));

	while(fgets(buf, 256, stdin) != NULL)
		Put(buf);

	Write("\x1b[0m\n");
	return 0;
}

/* EOB */
