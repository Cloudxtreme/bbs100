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
	state_config.c	WJ99

	The Config Menu
*/

#include "config.h"
#include "debug.h"
#include "state_config.h"
#include "state_friendlist.h"
#include "state_msg.h"
#include "state.h"
#include "edit.h"
#include "util.h"
#include "cstring.h"
#include "passwd.h"
#include "screens.h"
#include "Memory.h"
#include "Timezone.h"
#include "Param.h"
#include "bufprintf.h"
#include "access.h"
#include "DirList.h"

#include <stdio.h>
#include <stdlib.h>


#define CONFIG_OPTION(x, y)							\
	do {											\
		usr->flags ^= (x);							\
		usr->runtime_flags |= RTF_CONFIG_EDITED;	\
		Print(usr, "%s\n", (y));					\
		CURRENT_STATE(usr);							\
		Return;										\
	} while(0)

#define CONFIG_OPTION2(x, y)						\
	do {											\
		usr->flags2 ^= (x);							\
		usr->runtime_flags |= RTF_CONFIG_EDITED;	\
		Print(usr, "%s\n", (y));					\
		CURRENT_STATE(usr);							\
		Return;										\
	} while(0)


void state_config_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_config_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Put(usr, "<magenta>\n"
				"<hotkey>Address                      <hotkey>Help\n"
				"Profile <hotkey>info                 <hotkey>Terminal settings\n"
				"<hotkey>Doing                        Customize <hotkey>Who list\n"
				"<hotkey>Reminder                     <hotkey>Options\n"
			);
			if (PARAM_HAVE_XMSG_HDR)
				Put(usr, "e<hotkey>Xpress Message header       ");

			if (PARAM_HAVE_QUICK_X)
				Put(usr, "<hotkey>Quicklist\n");
			else
				if (PARAM_HAVE_XMSG_HDR)
					Put(usr, "\n");

			Put(usr,
				"Anon<hotkey>ymous alias              <hotkey>Friends and <hotkey>Enemies\n"
				"<hotkey>Password                     Time <hotkey>zone\n"
			);
			read_menu(usr);
			Return;

		case '$':
			if (usr->runtime_flags & RTF_SYSOP)
				drop_sysop_privs(usr);
			else
				break;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "\n");
			if (usr->runtime_flags & RTF_CONFIG_EDITED) {
				if (save_User(usr)) {
					Perror(usr, "failed to save user file");
				}
				usr->runtime_flags &= ~RTF_CONFIG_EDITED;
			}
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Address\n");
			CALL(usr, STATE_CONFIG_ADDRESS);
			Return;

		case 'i':
		case 'I':
			Put(usr, "Profile info\n");
			CALL(usr, STATE_CONFIG_PROFILE);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Doing\n");
			CALL(usr, STATE_CONFIG_DOING);
			Return;

		case 'x':
		case 'X':
			if (PARAM_HAVE_XMSG_HDR) {
				Put(usr, "eXpress Message header\n");
				CALL(usr, STATE_CONFIG_XMSG_HEADER);
				Return;
			}
			break;

		case 'r':
		case 'R':
			Put(usr, "Reminder\n");
			CALL(usr, STATE_CONFIG_REMINDER);
			Return;

		case 'y':
		case 'Y':
			Put(usr, "Default anonymous alias\n");
			CALL(usr, STATE_CONFIG_ANON);
			Return;

		case 'f':
		case 'F':
		case '>':
			Put(usr, "Friends\n");
			CALL(usr, STATE_FRIENDLIST_PROMPT);
			Return;

		case 'e':
		case 'E':
		case '<':
			Put(usr, "Enemies\n");
			CALL(usr, STATE_ENEMYLIST_PROMPT);
			Return;

		case 'p':
		case 'P':
			Put(usr, "Password\n");
			CALL(usr, STATE_CONFIG_PASSWORD);
			Return;

		case 't':
		case 'T':
			Put(usr, "Terminal settings\n");
			CALL(usr, STATE_CONFIG_TERMINAL);
			Return;

		case 'w':
		case 'W':
		case KEY_CTRL('W'):
			Put(usr, "Who list\n");
			CALL(usr, STATE_CONFIG_WHO);
			Return;

		case 'o':
		case 'O':
		case KEY_CTRL('O'):
			Put(usr, "Options\n");
			CALL(usr, STATE_CONFIG_OPTIONS);
			Return;

		case 'q':
		case 'Q':
		case KEY_CTRL('Q'):
			if (PARAM_HAVE_QUICK_X) {
				CALL(usr, STATE_QUICKLIST_PROMPT);
				Return;
			}
			break;

		case 'z':
		case 'Z':
			Put(usr, "Time Zone\n");
			CALL(usr, STATE_CONFIG_TIMEZONE);
			Return;

		case 'h':
		case 'H':
		case '?':
			if (help_config(usr))
				break;

			Return;
	}
	Print(usr, "<yellow>\n[Config] %c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

int help_config(User *usr) {
char filename[MAX_PATHLEN];

	Put(usr, "Help\n");

	bufprintf(filename, sizeof(filename), "%s/config", PARAM_HELPDIR);

	if (load_screen(usr->text, filename) < 0) {
		Put(usr, "<red>No help available\n");
		return -1;
	}
	PUSH(usr, STATE_PRESS_ANY_KEY);
	read_text(usr);
	return 0;
}


static char *print_address(char *value, char *alt, char *buf, int buflen) {
	if (value != NULL && *value)
		bufprintf(buf, buflen, "<yellow> %s", value);
	else
		bufprintf(buf, buflen, "<white> <unknown%s>", alt);
	return buf;
}

void state_config_address(User *usr, char c) {
char buf[MAX_LONGLINE];

	if (usr == NULL)
		return;

	Enter(state_config_address);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			buffer_text(usr);

			Print(usr, "<magenta>\n"
				"<hotkey>Real name :%s<magenta>\n", print_address(usr->real_name, "", buf, MAX_LONGLINE));

			Print(usr, "<hotkey>Address   :%s\n", print_address(usr->street, " street", buf, MAX_LONGLINE));

			Print(usr, "           %s",		print_address(usr->city, " city", buf, MAX_LONGLINE));
			Print(usr, " %s\n",				print_address(usr->zipcode, " ZIP code", buf, MAX_LONGLINE));
			Print(usr, "           %s,",	print_address(usr->state, " state", buf, MAX_LONGLINE));
			Print(usr, "%s<magenta>\n",		print_address(usr->country, " country", buf, MAX_LONGLINE));

			Print(usr, "<hotkey>Phone     :%s<magenta>\n",	print_address(usr->phone, " phone number", buf, MAX_LONGLINE));
			Print(usr, "\n"
				"<hotkey>E-mail    :%s<magenta>\n",			print_address(usr->email, " e-mail address", buf, MAX_LONGLINE));
			Print(usr, "<hotkey>WWW       :%s<magenta>\n",	print_address(usr->www, " WWW address", buf, MAX_LONGLINE));

			Print(usr, "\n"
				"<hotkey>Hide address from non-friends     <white>%s<magenta>\n",

				(usr->flags & USR_HIDE_ADDRESS) ? "Yes" : "No"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "Config menu\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Real name\n");
			CALL(usr, STATE_CHANGE_REALNAME);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Address\n");
			CALL(usr, STATE_CHANGE_ADDRESS);
			Return;

		case 'p':
		case 'P':
			Put(usr, "Phone number\n");
			CALL(usr, STATE_CHANGE_PHONE);
			Return;

		case 'e':
		case 'E':
			Put(usr, "E-mail address\n");
			CALL(usr, STATE_CHANGE_EMAIL);
			Return;

		case 'w':
		case 'W':
			Put(usr, "WWW address\n");
			CALL(usr, STATE_CHANGE_WWW);
			Return;

		case 'h':
		case 'H':
			CONFIG_OPTION(USR_HIDE_ADDRESS, "Hide address information");
	}
	Print(usr, "<yellow>\n[Config] Address%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_change_realname(User *usr, char c) {
	Enter(state_change_realname);
	change_config(usr, c, &usr->real_name, "<green>Enter your real name: <yellow>");
	Return;
}

void state_change_address(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_change_address);

	POP(usr);
	PUSH(usr, STATE_CHANGE_COUNTRY);
	PUSH(usr, STATE_CHANGE_STATE);
	PUSH(usr, STATE_CHANGE_ZIPCODE);
	PUSH(usr, STATE_CHANGE_CITY);
	CALL(usr, STATE_CHANGE_STREET);

	Return;
}


void state_change_street(User *usr, char c) {
	Enter(state_change_street);
	change_config(usr, c, &usr->street,		"<green>Enter your street and number  :<yellow> ");
	Return;
}

void state_change_zipcode(User *usr, char c) {
	Enter(state_change_zipcode);
	change_config(usr, c, &usr->zipcode,	"<green>Enter your ZIP or postal code :<yellow> ");
	Return;
}

void state_change_city(User *usr, char c) {
	Enter(state_change_city);
	change_config(usr, c, &usr->city,		"<green>Enter the city you live in    :<yellow> ");
	Return;
}

void state_change_state(User *usr, char c) {
	Enter(state_change_state);
	change_config(usr, c, &usr->state,		"<green>Enter the state you are from  :<yellow> ");
	Return;
}

void state_change_country(User *usr, char c) {
	Enter(state_change_country);
	change_config(usr, c, &usr->country,	"<green>Enter your country            :<yellow> ");
	Return;
}

void state_change_phone(User *usr, char c) {
	Enter(state_change_phone);
	change_config(usr, c, &usr->phone, "<green>Enter your phone number:<yellow> ");
	Return;
}

void state_change_email(User *usr, char c) {
	Enter(state_change_email);
	change_config(usr, c, &usr->email, "<green>Enter your e-mail address:<yellow> ");
	Return;
}

void state_change_www(User *usr, char c) {
	Enter(state_change_www);
	change_config(usr, c, &usr->www, "<green>Enter your WWW address:<yellow> ");
	Return;
}


void state_config_profile(User *usr, char c) {
	Enter(state_config_profile);

	switch(c) {
		case INIT_STATE:
			Put(usr, "<magenta>\n"
				"<hotkey>View current                 <hotkey>Upload\n"
				"<hotkey>Enter new                    <hotkey>Download\n"
			);
			if (PARAM_HAVE_VANITY) {
				Put(usr, "\n"
					"Vanit<hotkey>y"
				);
				if (usr->vanity != NULL && usr->vanity[0])
					Print(usr, "                       <magenta>*<white> %s <magenta>*\n", usr->vanity);
				else
					Put(usr, "                       <white>None\n");
			}
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "Config menu\n");

			if (!PARAM_HAVE_RESIDENT_INFO) {
				destroy_StringIO(usr->info);
				usr->info = NULL;
			}
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'v':
		case 'V':
			Put(usr, "View\n");
			load_profile_info(usr);
			if (usr->info == NULL || usr->info->buf == NULL) {
				Put(usr, "<cyan>Your current profile info is empty\n");
				CURRENT_STATE(usr);
				Return;
			}
			copy_StringIO(usr->text, usr->info);
			PUSH(usr, STATE_PRESS_ANY_KEY);
			Put(usr, "<green>");
			read_text(usr);
			Return;

		case 'e':
		case 'E':
			Put(usr, "Edit\n"
				"<green>\n"
				"Enter new profile info, press<yellow> <return><green> twice or press<yellow> <Ctrl-C><green> to end\n"
			);
			edit_text(usr, save_profile, abort_profile);
			Return;

		case 'u':
		case 'U':
			Put(usr, "Upload\n"
				"<green>\n"
				"Upload new profile info, press<yellow> <Ctrl-C><green> to end\n"
			);
			usr->runtime_flags |= RTF_UPLOAD;
			edit_text(usr, save_profile, abort_profile);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Download\n");
			load_profile_info(usr);
			if (usr->info == NULL || usr->info->buf == NULL) {
				Put(usr, "<cyan>Your current profile info is empty\n");
				CURRENT_STATE(usr);
				Return;
			}
			copy_StringIO(usr->text, usr->info);
			CALL(usr, STATE_DOWNLOAD_TEXT);
			Return;

		case 'y':
		case 'Y':
			if (PARAM_HAVE_VANITY) {
				Put(usr, "Vanity\n");
				CALL(usr, STATE_CONFIG_VANITY);
				Return;
			}
			break;
	}
	Print(usr, "<yellow>\n[Config] Profile%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void save_profile(User *usr, char c) {
StringIO *tmp;

	if (usr == NULL)
		return;

	Enter(save_profile);

	destroy_StringIO(usr->info);
	usr->info = NULL;

	if ((tmp = new_StringIO()) == NULL) {
		Perror(usr, "Failed to save profile");
		RET(usr);
		Return;
	}
	usr->info = usr->text;
	usr->text = tmp;
/*
	save it now, or else have problems with PARAM_HAVE_RESIDENT_INFO
*/
	if (save_User(usr)) {
		Perror(usr, "failed to save user file");
	}
	usr->runtime_flags &= ~RTF_CONFIG_EDITED;

	RET(usr);
	Return;
}

void abort_profile(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(abort_profile);

	free_StringIO(usr->text);
	RET(usr);
	Return;
}

void state_config_vanity(User *usr, char c) {
	Enter(state_config_vanity);

	if (c == INIT_STATE && usr->vanity != NULL && usr->vanity[0])
		Print(usr, "<green>Your current vanity flag:<cyan> %s\n", usr->vanity);

	change_config(usr, c, &usr->vanity, "<green>Enter new vanity flag:<yellow> ");
	Return;
}

void state_config_doing(User *usr, char c) {
	Enter(state_config_doing);

	if (c == INIT_STATE && usr->doing != NULL && usr->doing[0])
		Print(usr, "<green>You are currently doing:<cyan> %s\n", usr->doing);

	change_config(usr, c, &usr->doing, "<green>Enter new Doing:<yellow> ");
	Return;
}

void state_config_xmsg_header(User *usr, char c) {
	Enter(state_config_xmsg_header);

	if (c == INIT_STATE && usr->xmsg_header != NULL && usr->xmsg_header[0])
		Print(usr, "<green>Your current eXpress Message header:<cyan> %s\n", usr->xmsg_header);

	change_config(usr, c, &usr->xmsg_header, "<green>Enter new eXpress Message header:<yellow> ");
	Return;
}

void state_config_reminder(User *usr, char c) {
	Enter(state_config_reminder);

	if (c == INIT_STATE && usr->reminder != NULL && usr->reminder[0])
		Print(usr, "<green>Current reminder:<cyan> %s\n", usr->reminder);

	change_config(usr, c, &usr->reminder, "<green>Enter new reminder:<yellow> ");
	Return;
}

void state_config_anon(User *usr, char c) {
int r;

	Enter(state_config_anon);

	if (c == INIT_STATE) {
		if (usr->default_anon != NULL && usr->default_anon[0])
			Print(usr, "<green>Your current default anonymous alias is:<cyan> %s\n", usr->default_anon);
		Put(usr, "<green>Enter new alias:<yellow> ");
	}
	r = edit_name(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			cstrip_line(usr->edit_buf);

			if (!usr->edit_buf[0]) {
				Free(usr->default_anon);
				usr->default_anon = NULL;
			} else {
				char *s;

				if ((s = cstrdup(usr->edit_buf)) == NULL) {
					Perror(usr, "Out of memory");
					RET(usr);
					Return;
				}
				Free(usr->default_anon);
				usr->default_anon = s;
				cstrlwr(usr->default_anon);
				Print(usr, "<green>Default anonymous alias set to:<cyan> %s\n", usr->default_anon);
			}
			usr->runtime_flags |= RTF_CONFIG_EDITED;
		} else
			if (usr->default_anon != NULL && usr->default_anon[0])
				Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}


void state_config_password(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_config_password);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter old password:<yellow> ");

	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
		Put(usr, "<red>Password not changed\n");
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if (!verify_phrase(usr->edit_buf, usr->passwd)) {
			clear_password_buffer(usr);
			JMP(usr, STATE_CHANGE_PASSWORD);
		} else {
			clear_password_buffer(usr);
			Put(usr, "<red>Wrong password\n");
			RET(usr);
		}
	}
	Return;
}

void state_change_password(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_change_password);

	if (c == INIT_STATE) {
		Put(usr, "<green>Enter new password:<yellow> ");

		Free(usr->tmpbuf[TMP_PASSWD]);
		usr->tmpbuf[TMP_PASSWD] = NULL;
	}
	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
		Put(usr, "<red>Password not changed\n");
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if (usr->tmpbuf[TMP_PASSWD] == NULL) {
			if (strlen(usr->edit_buf) < 5) {
				clear_password_buffer(usr);
				Put(usr, "<red>That password is too short\n");
				CURRENT_STATE(usr);
				Return;
			}
			Put(usr, "<green>Enter it again (for verification):<yellow> ");

			if ((usr->tmpbuf[TMP_PASSWD] = cstrdup(usr->edit_buf)) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			clear_password_buffer(usr);
			usr->edit_buf[0] = 0;
			usr->edit_pos = 0;
		} else {
			if (!strcmp(usr->edit_buf, usr->tmpbuf[TMP_PASSWD])) {
				char crypted[MAX_CRYPTED];

				crypt_phrase(usr->edit_buf, crypted);
				crypted[MAX_CRYPTED_PASSWD-1] = 0;

				if (verify_phrase(usr->edit_buf, crypted)) {
					clear_password_buffer(usr);
					Perror(usr, "bug in password encryption -- please choose an other password");
					CURRENT_STATE(usr);
					Return;
				}
				clear_password_buffer(usr);
				cstrcpy(usr->passwd, crypted, MAX_CRYPTED_PASSWD);
				Put(usr, "Password changed\n");
				usr->runtime_flags |= RTF_CONFIG_EDITED;
			} else {
				clear_password_buffer(usr);
				Put(usr, "<red>Passwords didn't match; password NOT changed\n");
			}
			Free(usr->tmpbuf[TMP_PASSWD]);
			usr->tmpbuf[TMP_PASSWD] = NULL;
			RET(usr);
		}
	}
	Return;
}

void state_quicklist_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_quicklist_prompt);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			Put(usr, "<white>Quicklist\n\n");
			buffer_text(usr);
			print_quicklist(usr);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "<white>Config menu\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			Print(usr, "%c\n", c);
			if (c == '0')
				c = '9'+1;
			r = c - '1';
			PUSH_ARG(usr, &r, sizeof(int));
			enter_name(usr, STATE_EDIT_QUICKLIST);
			Return;
	}
	Put(usr, "\n<green>Enter number: <yellow>");
	Return;
}

/*
	Note: the index to the quicklist entry is on the stack
*/
void state_edit_quicklist(User *usr, char c) {
int r;

	if (usr == NULL || c == INIT_STATE)
		return;

	Enter(state_edit_quicklist);

	r = edit_tabname(usr, c);

	if (r == EDIT_BREAK) {
		deinit_StringQueue(usr->recipients);
		POP_ARG(usr, &r, sizeof(int));
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		POP_ARG(usr, &r, sizeof(int));
		if (r < 0 || r >= NUM_QUICK) {
			Perror(usr, "Invalid quicklist entry");
			RET(usr);
			Return;
		}
		if (!usr->edit_buf[0]) {
			Free(usr->quick[r]);
			usr->quick[r] = NULL;

			usr->runtime_flags |= RTF_CONFIG_EDITED;
		} else {
			if (!user_exists(usr->edit_buf))
				Put(usr, "<red>No such user\n");
			else {
				if ((usr->quick[r] = cstrdup(usr->edit_buf)) == NULL) {
					Perror(usr, "Out of memory");
				}
				usr->runtime_flags |= RTF_CONFIG_EDITED;
			}
		}
		deinit_StringQueue(usr->recipients);
		RET(usr);
		Return;
	}
	Return;
}

void state_config_terminal(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_config_terminal);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Print(usr, "<magenta>\n"
				"<hotkey>Terminal emulation                   <white>%s<magenta>\n"
				"Make use of bold/bright <hotkey>attribute    <white>%s<magenta>\n",

				(usr->flags & USR_ANSI) ? "ANSI" : "dumb",
				(usr->flags & USR_BOLD) ? "Yes"  : "No"
			);
/*
	USR_BOLD_HOTKEYS has a complex meaning;
	if USR_BOLD is set, then BOLD_HOTKEYS means we want faint hotkeys,
	if BOLD is NOT set, then BOLD_HOTKEYS means we want bold hotkeys

	see also print_hotkey() in util.c
*/
			Print(usr,
				"Show hotkeys in <hotkey>bold/bright          <white>%s<magenta>\n",

				((usr->flags & (USR_BOLD|USR_BOLD_HOTKEYS)) == (USR_BOLD|USR_BOLD_HOTKEYS)
					|| (usr->flags & (USR_BOLD|USR_BOLD_HOTKEYS)) == 0) ? "No" : "Yes"
			);
/*
	USR_HOTKEY_BRACKETS has a weird inverted meaning;
	if USR_ANSI is set, HOTKEY_BRACKETS means we want brackets
	if ANSI is not set, HOTKEY_BRACKETS means we don't want them

	see also print_hotkey() in util.c
*/
			Print(usr,
				"Always show hotkeys in <hotkey>uppercase     <white>%s<magenta>\n"
				"Show angle brac<hotkey>kets around hotkeys   <white>%s<magenta>%s\n",

				(usr->flags & USR_UPPERCASE_HOTKEYS) ? "Yes" : "No",
				(((usr->flags & (USR_ANSI|USR_HOTKEY_BRACKETS)) == (USR_ANSI|USR_HOTKEY_BRACKETS))
					|| (usr->flags & (USR_ANSI|USR_HOTKEY_BRACKETS)) == 0) ? "Yes" : "No",
				((usr->flags & (USR_ANSI|USR_HOTKEY_BRACKETS)) == USR_HOTKEY_BRACKETS) ? "  (Hit <key>k to enable)" : ""
			);
			Print(usr, "\n"
				"<hotkey>Force screen width and height        <white>%s<magenta>\n"
				"Screen <hotkey>dimensions                    <white>%dx%d<magenta>\n",

				(usr->flags & USR_FORCE_TERM) ? "Yes" : "No",
				usr->display->term_width, usr->display->term_height
			);
			if (usr->flags & USR_ANSI) {
				Print(usr, "\n"
					"Color <hotkey>scheme                         <white>%s<magenta>\n"
					"Customize <hotkey>colors\n",

					(usr->flags & USR_DONT_AUTO_COLOR) ? "Classic" : "Modern"
				);
				if (!(usr->flags & USR_DONT_AUTO_COLOR))
					Print(usr, "Customize colors of s<hotkey>ymbols\n");
			}
			Print(usr, "\n"
				"Hackerz m<hotkey>0de                         <white>%s<magenta>\n",

				(usr->flags & USR_HACKERZ) ? "Oh Yeah" : "Off"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "Config menu\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 't':
		case 'T':
			Put(usr, "Terminal emulation<default>\n");

			usr->flags ^= USR_ANSI;

			if (usr->flags & USR_ANSI) {			/* assume bold/non-bold */
				usr->flags |= USR_BOLD;
				usr->flags &= ~(USR_HOTKEY_BRACKETS|USR_BOLD_HOTKEYS);
			} else
				usr->flags &= ~(USR_BOLD|USR_BOLD_HOTKEYS|USR_HOTKEY_BRACKETS);

			Put(usr, "<normal>");
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			CURRENT_STATE(usr);
			Return;

		case 'a':
		case 'A':
			Put(usr, "<default>");

			usr->flags ^= USR_BOLD;
			usr->flags &= ~USR_BOLD_HOTKEYS;
			
			Put(usr, "<normal>Attribute bold/bright\n");
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			CURRENT_STATE(usr);
			Return;

		case 'b':
		case 'B':
			CONFIG_OPTION(USR_BOLD_HOTKEYS, "Bold/bright hotkeys");

		case 'u':
		case 'U':
			CONFIG_OPTION(USR_UPPERCASE_HOTKEYS, "Uppercase hotkeys");

		case 'k':
		case 'K':
			CONFIG_OPTION(USR_HOTKEY_BRACKETS, "Angle brackets around hotkeys");

		case 's':
		case 'S':
			if (!(usr->flags & USR_ANSI))
				break;

			CONFIG_OPTION(USR_DONT_AUTO_COLOR, "Color scheme");

		case 'f':
		case 'F':
			usr->flags ^= USR_FORCE_TERM;
			Print(usr, "%s screen width and height\n", (usr->flags & USR_FORCE_TERM) ? "Force" : "Don't force");

			if (!(usr->flags & USR_FORCE_TERM) && usr->telnet != NULL) {
				usr->display->term_width = usr->telnet->term_width;
				usr->display->term_height = usr->telnet->term_height;
			}
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			CURRENT_STATE(usr);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Screen dimensions\n");
			CALL(usr, STATE_CONFIG_WIDTH);
			Return;

		case 'c':
		case 'C':
			if (usr->flags & USR_ANSI) {
				Put(usr, "Customize colors\n");
				CALL(usr, STATE_CONFIG_COLORS);
				Return;
			}
			break;

		case 'y':
		case 'Y':
			if ((usr->flags & (USR_ANSI|USR_DONT_AUTO_COLOR)) == USR_ANSI) {
				Put(usr, "Customize colors of symbols\n");
				CALL(usr, STATE_CONFIG_SYMBOLS);
				Return;
			}
			break;

		case '0':
			CONFIG_OPTION(USR_HACKERZ, "Hackerz mode");
	}
	Print(usr, "<yellow>\n[Config] Terminal%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_config_width(User *usr, char c) {
	if (usr == NULL)
		return;

	if (c == INIT_STATE)
		Print(usr, "<green>Enter screen width <white>[<yellow>%d<white>]: ", usr->display->term_width);

	config_dimensions(usr, c, &usr->display->term_width, STATE_CONFIG_HEIGHT);
}

void state_config_height(User *usr, char c) {
	if (usr == NULL)
		return;

	if (c == INIT_STATE)
		Print(usr, "<green>Enter screen height <white>[<yellow>%d<white>]: ", usr->display->term_height);

	config_dimensions(usr, c, &usr->display->term_height, NULL);
}

void config_dimensions(User *usr, char c, int *var, void (*next_state)(User *, char)) {
int r;

	if (usr == NULL)
		return;

	if (var == NULL) {
		log_err("state_config_integer(): BUG !");
		RET(usr);
		return;
	}
	Enter(config_dimensions);

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			if (next_state != NULL) {
				JMP(usr, next_state);
				Return;
			}
			RET(usr);
			Return;
		}
		if (is_numeric(usr->edit_buf))
			r = (int)cstrtoul(usr->edit_buf, 10);
		else
			r = -1;

		if (r < 1) {
			Put(usr, "<red>Invalid value; not changed\n");
			RET(usr);
			Return;
		}
		if (r > MAX_TERM) {
			Put(usr, "<red>Too large, not changed\n");
			RET(usr);
			Return;
		}
		*var = r;
		usr->flags |= USR_FORCE_TERM;
		usr->runtime_flags |= RTF_CONFIG_EDITED;

		if (next_state != NULL) {
			JMP(usr, next_state);
			Return;
		}
		RET(usr);
	}
	Return;
}


#define CUSTOM_COLOR(x)										\
	do {													\
		r = (x);											\
		Print(usr, "Customize %s\n", color_table[r].name);	\
		PUSH_ARG(usr, &r, sizeof(int));						\
		CALL(usr, STATE_CUSTOM_COLORS);						\
		Return;												\
	} while(0)

void state_config_colors(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_config_colors);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			buffer_text(usr);

/* the <normal> tag here resets the background color, if needed */

			Print(usr, "<normal><magenta>\n"
				"<hotkey>White      <white>[%c%-7s<white>]<magenta>         <hotkey>Cyan       <white>[%c%-7s<white>]<magenta>\n"
				"<hotkey>Yellow     <white>[%c%-7s<white>]<magenta>         <hotkey>Blue       <white>[%c%-7s<white>]<magenta>\n",

				color_table[usr->colors[WHITE]].key,	color_table[usr->colors[WHITE]].name,
				color_table[usr->colors[CYAN]].key,		color_table[usr->colors[CYAN]].name,
				color_table[usr->colors[YELLOW]].key,	color_table[usr->colors[YELLOW]].name,
				color_table[usr->colors[BLUE]].key,		color_table[usr->colors[BLUE]].name
			);
			Print(usr,
				"<hotkey>Red        <white>[%c%-7s<white>]<magenta>         <hotkey>Magenta    <white>[%c%-7s<white>]<magenta>\n"
				"<hotkey>Green      <white>[%c%-7s<white>]<magenta>         H<hotkey>otkey     <white>[%c%-7s<white>]<magenta>\n"
				"\n"
				"<magenta>Bac<hotkey>kground <white>[%c%-7s<white>]<magenta>         <hotkey>Defaults for all colors\n",

				color_table[usr->colors[RED]].key,		color_table[usr->colors[RED]].name,
				color_table[usr->colors[MAGENTA]].key,	color_table[usr->colors[MAGENTA]].name,
				color_table[usr->colors[GREEN]].key,	color_table[usr->colors[GREEN]].name,
				color_table[usr->colors[HOTKEY]].key,	color_table[usr->colors[HOTKEY]].name,
				color_table[usr->colors[BACKGROUND]].key, color_table[usr->colors[BACKGROUND]].name
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "Config Terminal\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'w':
		case 'W':
			CUSTOM_COLOR(WHITE);

		case 'y':
		case 'Y':
			CUSTOM_COLOR(YELLOW);

		case 'r':
		case 'R':
			CUSTOM_COLOR(RED);

		case 'g':
		case 'G':
			CUSTOM_COLOR(GREEN);

		case 'c':
		case 'C':
			CUSTOM_COLOR(CYAN);

		case 'b':
		case 'B':
			CUSTOM_COLOR(BLUE);

		case 'm':
		case 'M':
			CUSTOM_COLOR(MAGENTA);

		case 'o':
		case 'O':
			CUSTOM_COLOR(HOTKEY);

		case 'k':
		case 'K':
			CUSTOM_COLOR(BACKGROUND);

		case 'd':
		case 'D':
			if (usr->flags & USR_ANSI) {
				default_colors(usr);
				Put(usr, "Defaults<normal>\n");
			}
			CURRENT_STATE(usr);
			Return;
	}
	Print(usr, "<yellow>\n[Config] Colors%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

/*
	Customize colors
	Note: index of color to change is on the stack
*/
void state_custom_colors(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_custom_colors);

	switch(c) {
		case INIT_STATE:
			Put(usr, "\n<green>Colors are: <hotkey>R<red>ed <hotkey>G<green>reen <hotkey>Y<yellow>ellow <hotkey>B<blue>lue <hotkey>M<magenta>agenta <hotkey>C<cyan>yan <hotkey>W<white>hite <black>Blac<hotkey>k <hotkey>D<green>efault");
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			Put(usr, "\n");
			POP_ARG(usr, &r, sizeof(int));
			RET(usr);
			Return;

		case 'r':
		case 'R':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = RED;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<red>Red\n");
			RET(usr);
			Return;

		case 'g':
		case 'G':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = GREEN;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<green>Green\n");
			RET(usr);
			Return;

		case 'y':
		case 'Y':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = YELLOW;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<yellow>Yellow\n");
			RET(usr);
			Return;

		case 'b':
		case 'B':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = BLUE;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<blue>Blue\n");
			RET(usr);
			Return;

		case 'm':
		case 'M':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = MAGENTA;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<magenta>Magenta\n");
			RET(usr);
			Return;

		case 'c':
		case 'C':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = CYAN;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<cyan>Cyan\n");
			RET(usr);
			Return;

		case 'w':
		case 'W':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = WHITE;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<white>White\n");
			RET(usr);
			Return;

		case 'k':
		case 'K':
			POP_ARG(usr, &r, sizeof(int));
			usr->colors[r] = BLACK;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<white>Black\n");
			RET(usr);
			Return;

		case 'd':
		case 'D':
			POP_ARG(usr, &r, sizeof(int));
			if (r == HOTKEY)
				usr->colors[r] = YELLOW;
			else
				usr->colors[r] = r;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<white>Default\n");
			RET(usr);
			Return;
	}
	PEEK_ARG(usr, &r, sizeof(int));
	if (r == BACKGROUND)
		Put(usr, "\n<cyan>Change the background color to: ");
	else
		Print(usr, "\n<cyan>Change the color for %c%s<cyan> to: ", 
			color_table[r].key,
			color_table[r].name);
	Return;
}


#define CUSTOM_SYMBOL_COLOR(x)						\
	do {											\
		r = (x);									\
		Put(usr, "Symbol color\n");					\
		PUSH_ARG(usr, &r, sizeof(int));				\
		CALL(usr, STATE_CUSTOM_SYMBOL_COLORS);		\
		Return;										\
	} while(0)

void state_config_symbols(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_config_symbols);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			buffer_text(usr);

			Print(usr, "<magenta>\n"
				"Symbols in <hotkey>White text appear in ...   %c%-16s<white> [sample..!]<magenta>\n"
				"Symbols in <hotkey>Yellow text appear in ...  %c%-16s<yellow> [sample..!]<magenta>\n"
				"Symbols in <hotkey>Red text appear in ...     %c%-16s<red> [sample..!]<magenta>\n"
				"Symbols in <hotkey>Green text appear in ...   %c%-16s<green> [sample..!]<magenta>\n",

				color_table[usr->symbol_colors[WHITE]].key, color_table[usr->symbol_colors[WHITE]].name,
				color_table[usr->symbol_colors[YELLOW]].key, color_table[usr->symbol_colors[YELLOW]].name,
				color_table[usr->symbol_colors[RED]].key, color_table[usr->symbol_colors[RED]].name,
				color_table[usr->symbol_colors[GREEN]].key, color_table[usr->symbol_colors[GREEN]].name
			);
			Print(usr,
				"Symbols in <hotkey>Cyan text appear in ...    %c%-16s<cyan> [sample..!]<magenta>\n"
				"Symbols in <hotkey>Blue text appear in ...    %c%-16s<blue> [sample..!]<magenta>\n"
				"Symbols in <hotkey>Magenta text appear in ... %c%-16s<magenta> [sample..!]<magenta>\n",

				color_table[usr->symbol_colors[CYAN]].key, color_table[usr->symbol_colors[CYAN]].name,
				color_table[usr->symbol_colors[BLUE]].key, color_table[usr->symbol_colors[BLUE]].name,
				color_table[usr->symbol_colors[MAGENTA]].key, color_table[usr->symbol_colors[MAGENTA]].name
			);
/* print the symbol string in white */
			usr->flags |= USR_DONT_AUTO_COLOR;

			Print(usr,
				"\n"
				"<hotkey>Defaults for all colors\n"
				"\n"
				"Automatically color these <hotkey>symbols     <white>%s<magenta>\n",

				(usr->symbols == NULL) ? Default_Symbols : usr->symbols);

/* we can safely do this, because you can't have this flag set and be in this menu */
			usr->flags &= ~USR_DONT_AUTO_COLOR;

			if (usr->symbols != NULL && strcmp(usr->symbols, Default_Symbols))
				Print(usr, "<hotkey>Use default symbol string\n");

			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "Config Terminal\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'w':
		case 'W':
			CUSTOM_SYMBOL_COLOR(WHITE);

		case 'y':
		case 'Y':
			CUSTOM_SYMBOL_COLOR(YELLOW);

		case 'r':
		case 'R':
			CUSTOM_SYMBOL_COLOR(RED);

		case 'g':
		case 'G':
			CUSTOM_SYMBOL_COLOR(GREEN);

		case 'c':
		case 'C':
			CUSTOM_SYMBOL_COLOR(CYAN);

		case 'b':
		case 'B':
			CUSTOM_SYMBOL_COLOR(BLUE);

		case 'm':
		case 'M':
			CUSTOM_SYMBOL_COLOR(MAGENTA);

		case 'd':
		case 'D':
			Put(usr, "Default colors\n");
			default_symbol_colors(usr);

			CURRENT_STATE(usr);
			Return;

		case 's':
		case 'S':
			Put(usr, "Symbols\n");
			CALL(usr, STATE_CHANGE_SYMBOLS);
			Return;

		case 'u':
		case 'U':
			if (usr->symbols != NULL && strcmp(usr->symbols, Default_Symbols)) {
				Put(usr, "Use default symbols\n");
				Free(usr->symbols);
				usr->symbols = NULL;

				CURRENT_STATE(usr);
				Return;
			}
			break;
	}
	Print(usr, "<yellow>\n[Config] Symbols%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

/*
	Customize symbol colors
	almost the same as state_custom_colors(), but with small changes ...

	Note: index of color to change is on the stack
*/
void state_custom_symbol_colors(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_custom_symbol_colors);

	switch(c) {
		case INIT_STATE:
			Put(usr, "\n<green>Colors are: <hotkey>R<red>ed <hotkey>G<green>reen <hotkey>Y<yellow>ellow <hotkey>B<blue>lue <hotkey>M<magenta>agenta <hotkey>C<cyan>yan <hotkey>W<white>hite <black>Blac<hotkey>k");
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			Put(usr, "\n");
			POP_ARG(usr, &r, sizeof(int));
			RET(usr);
			Return;

		case 'r':
		case 'R':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = RED;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<red>Red\n");
			RET(usr);
			Return;

		case 'g':
		case 'G':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = GREEN;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<green>Green\n");
			RET(usr);
			Return;

		case 'y':
		case 'Y':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = YELLOW;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<yellow>Yellow\n");
			RET(usr);
			Return;

		case 'b':
		case 'B':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = BLUE;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<blue>Blue\n");
			RET(usr);
			Return;

		case 'm':
		case 'M':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = MAGENTA;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<magenta>Magenta\n");
			RET(usr);
			Return;

		case 'c':
		case 'C':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = CYAN;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<cyan>Cyan\n");
			RET(usr);
			Return;

		case 'w':
		case 'W':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = WHITE;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<white>White\n");
			RET(usr);
			Return;

		case 'k':
		case 'K':
			POP_ARG(usr, &r, sizeof(int));
			usr->symbol_colors[r] = BLACK;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Put(usr, "<white>Black\n");
			RET(usr);
			Return;
	}
	PEEK_ARG(usr, &r, sizeof(int));
	Print(usr, "\n<cyan>Have symbols on %c%s<cyan> text appear in: ",
		color_table[r].key,
		color_table[r].name);
	Return;
}

void state_change_symbols(User *usr, char c) {
	Enter(state_change_symbols);

	if (c == INIT_STATE)
		usr->flags |= USR_DONT_AUTO_COLOR;
	else
		if (c == (char)EDIT_BREAK || c == EDIT_RETURN)
			usr->flags &= ~USR_DONT_AUTO_COLOR;

	change_config(usr, c, &usr->symbols, "<green>Enter symbols that will be automatically colored: <yellow>");
	Return;
}

void state_config_who(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_config_who);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Print(usr, "\n<magenta>"
				"Default who list <hotkey>format      <white>%s<magenta>\n"
				"Sort <hotkey>by...                   <white>%s<magenta>\n"
				"Sort <hotkey>order                   <white>%s<magenta>\n",
				(usr->flags & USR_SHORT_WHO)       ? "Short"      : "Long",
				(usr->flags & USR_SORT_BYNAME)     ? "Name"       : "Online time",
				(usr->flags & USR_SORT_DESCENDING) ? "Descending" : "Ascending"
			);
			if (PARAM_HAVE_CHATROOMS)
				Print(usr,
					"When in a <hotkey>chat room...       <white>%s<magenta>\n",
					(usr->flags & USR_SHOW_ALL)        ? "Show All"   : "Show Inside"
				);

			Print(usr,
				"Show online <hotkey>enemies          <white>%s<magenta>\n",
				(usr->flags & USR_SHOW_ENEMIES)    ? "Yes"        : "No"
			);
			read_menu(usr);
			Return;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_ESC:
		case ' ':
		case KEY_BS:
		case KEY_RETURN:
			Put(usr, "Exit\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'f':
		case 'F':
			CONFIG_OPTION(USR_SHORT_WHO, "Format");

		case 'b':
		case 'B':
			CONFIG_OPTION(USR_SORT_BYNAME, "Sort by");

		case 'o':
		case 'O':
			CONFIG_OPTION(USR_SORT_DESCENDING, "Sort order");

		case 'c':
		case 'C':
			if (PARAM_HAVE_CHATROOMS)
				CONFIG_OPTION(USR_SHOW_ALL, "In a chat room...");

			break;

		case 'e':
		case 'E':
			usr->flags ^= USR_SHOW_ENEMIES;
			Print(usr, "%s enemies\n", (usr->flags & USR_SHOW_ENEMIES) ? "Show" : "Don't show");
			CURRENT_STATE(usr);
			Return;
	}
	Print(usr, "<yellow>\n[Config] Who%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_config_who_sysop(User *usr, char c) {
	POP(usr);
	Print(usr, "<yellow>\n[Config] Who%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
}


void state_config_options(User *usr, char c) {
Room *r;
char buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(state_config_options);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Print(usr, "\n<magenta>"
				"Beep on e<hotkey>Xpress message arrival      <white>%s<magenta>\n"
				"Messages have <hotkey>sequence numbers       <white>%s<magenta>\n"
				"<hotkey>Message reception is ...             <white>%s<magenta>\n"
				"<hotkey>Accept Xes from Friends regardless   <white>%s<magenta>\n",

				(usr->flags & USR_BEEP) ? "Yes" : "No",
				(usr->flags & USR_XMSG_NUM) ? "Yes" : "No",
				(usr->flags & USR_X_DISABLED) ? "Disabled" : "Enabled",
				(usr->flags & USR_BLOCK_FRIENDS) ? "No" : "Yes"
			);
			Print(usr,
				"M<hotkey>ulti message reception is ...       <white>%s<magenta>\n"
				"<hotkey>Follow up mode (auto reply)          <white>%s<magenta>\n"
				"<hotkey>Hold message mode when busy          <white>%s<magenta>\n"
				"Ask for a <hotkey>reason when going away     <white>%s<magenta>\n",

				(usr->flags & USR_DENY_MULTI) ? "Disabled" : "Enabled",
				(usr->flags & USR_FOLLOWUP) ? "On" : "Off",
				(usr->flags & USR_HOLD_BUSY) ? "Yes" : "No",
				(usr->flags & USR_DONT_ASK_REASON) ? "No" : "Yes"
			);
			Print(usr, "\n"
				"Rooms <hotkey>beep on new posts              <white>%s<magenta>\n"
				"<hotkey>Enter message defaults to Upload     <white>%s<magenta>\n"
				"C<hotkey>ycle rooms on no new messages       <white>%s<magenta>\n"
				"Show room <hotkey>number in prompt           <white>%s<magenta>\n",

				(usr->flags & USR_ROOMBEEP) ? "Yes" : "No",
				(usr->flags2 & USR2_ENTER_UPLOAD) ? "Yes" : "No",
				(usr->flags & USR_CYCLE_ROOMS) ? "Yes" : "No",
				(usr->flags & USR_ROOMNUMBERS) ? "Yes" : "No"
			);
			if ((r = find_Roombynumber(usr, usr->default_room)) == NULL || room_access(r, usr->name) < ACCESS_OK) {
				usr->default_room = LOBBY_ROOM;
				r = Lobby_room;
			}
			Print(usr,
				"Default r<hotkey>oom is set to ...           <yellow>%s<magenta>\n",

				room_name(usr, r, buf, MAX_LINE)
			);
			Print(usr, "\n"
				"<hotkey>Downloads pause every page           <white>%s<magenta>\n"
				"<hotkey>Color codes in downloads are ...     <white>%s<magenta>\n",

				(usr->flags & USR_NOPAGE_DOWNLOADS) ? "No" : "Yes",
				(usr->flags & USR_SHORT_DL_COLORS) ? "Short" : "Long"
			);
			Print(usr, "\n"
				"<hotkey>Verbose friend notifications         <white>%s<magenta>\n"
				"Default <hotkey>profile info is ...          <white>%s<magenta>\n"
				"Hide profile <hotkey>info from enemies       <white>%s<magenta>\n",

				(usr->flags & USR_FRIEND_NOTIFY) ? "On" : "Off",
				(usr->flags & USR_SHORT_PROFILE) ? "Short" : "Long",
				(usr->flags & USR_HIDE_INFO) ? "Yes" : "No"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "Config menu\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'x':
		case 'X':
			CONFIG_OPTION(USR_BEEP, "Beep");

		case 'm':
		case 'M':
			usr->flags ^= USR_X_DISABLED;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Print(usr, "%s message reception\n", (usr->flags & USR_X_DISABLED) ? "Disable" : "Enable");
			CURRENT_STATE(usr);
			Return;

		case 'a':
		case 'A':
			usr->flags ^= USR_BLOCK_FRIENDS;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Print(usr, "%s friend messages\n", (usr->flags & USR_BLOCK_FRIENDS) ? "Block" : "Accept");

/* if blocking friends, remove them from the override list */
			if ((usr->flags & USR_BLOCK_FRIENDS) && usr->override != NULL) {
				StringList *sl, *sl2;

				for(sl = usr->friends; sl != NULL; sl = sl->next) {
					if ((sl2 = in_StringList(usr->override, sl->str)) != NULL) {
						(void)remove_StringList(&usr->override, sl2);
						destroy_StringList(sl2);
					}
				}
			}
			CURRENT_STATE(usr);
			Return;

		case 'u':
		case 'U':
			usr->flags ^= USR_DENY_MULTI;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Print(usr, "%s multi message reception\n", (usr->flags & USR_DENY_MULTI) ? "Disable" : "Enable");
			CURRENT_STATE(usr);
			Return;

		case 'f':
		case 'F':
			usr->flags ^= USR_FOLLOWUP;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Print(usr, "%s follow up mode\n", (usr->flags & USR_FOLLOWUP) ? "Enable" : "Disable");
			CURRENT_STATE(usr);
			Return;

		case 's':
		case 'S':
			CONFIG_OPTION(USR_XMSG_NUM, "Sequence numbers");

		case 'h':
		case 'H':
			CONFIG_OPTION(USR_HOLD_BUSY, "Hold message mode when busy");

		case 'r':
		case 'R':
			usr->flags ^= USR_DONT_ASK_REASON;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Print(usr, "%s for a reason\n", (usr->flags & USR_DONT_ASK_REASON) ? "Don't ask" : "Ask");
			CURRENT_STATE(usr);
			Return;

		case 'd':
		case 'D':
			CONFIG_OPTION(USR_NOPAGE_DOWNLOADS, "Downloads page");

		case 'c':
		case 'C':
			CONFIG_OPTION(USR_SHORT_DL_COLORS, "Color codes in downloads");

		case 'v':
		case 'V':
			CONFIG_OPTION(USR_FRIEND_NOTIFY, "Verbose friend notifications");

		case 'b':
		case 'B':
			CONFIG_OPTION(USR_ROOMBEEP, "Beep on new posts");

		case 'e':
		case 'E':
			usr->flags2 ^= USR2_ENTER_UPLOAD;
			usr->runtime_flags |= RTF_CONFIG_EDITED;
			Print(usr, "%s message\n", (usr->flags2 & USR2_ENTER_UPLOAD) ? "Upload" : "Enter");
			CURRENT_STATE(usr);
			Return;

		case 'y':
		case 'Y':
			CONFIG_OPTION(USR_CYCLE_ROOMS, "Cycle rooms");

		case 'n':
		case 'N':
			CONFIG_OPTION(USR_ROOMNUMBERS, "Show room number");

		case 'o':
		case 'O':
			CALL(usr, STATE_DEFAULT_ROOM);
			Return;

		case 'p':
		case 'P':
			CONFIG_OPTION(USR_SHORT_PROFILE, "Profile");

		case 'i':
		case 'I':
			CONFIG_OPTION(USR_HIDE_INFO, "Hide profile information");
	}
	Print(usr, "<yellow>\n[Config] Options%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_default_room(User *usr, char c) {
int r;
Room *rm;

	if (usr == NULL)
		return;

	Enter(state_default_room);

	if (c == INIT_STATE) {
		char buf[MAX_LINE];

		if ((rm = find_Roombynumber(usr, usr->default_room)) == NULL || room_access(rm, usr->name) < ACCESS_OK) {
			usr->default_room = LOBBY_ROOM;
			rm = Lobby_room;
		}
		Print(usr, "<cyan>Your default room is <yellow>%s<cyan>\n"
			"Enter new default room: <yellow>", room_name(usr, rm, buf, MAX_LINE));
	}
	r = edit_roomname(usr, c);
	switch(r) {
		case EDIT_BREAK:
			Put(usr, "<red>Not changed\n");
			RET(usr);
			Return;

		case EDIT_RETURN:
			if (!usr->edit_buf[0]) {
				Put(usr, "<red>Not changed\n");
				RET(usr);
				Return;
			}
			if ((rm = find_abbrevRoom(usr, usr->edit_buf)) == NULL) {
				Put(usr, "<red>No such room\n");
				RET(usr);
				Return;
			}
			if (rm->number == HOME_ROOM) {
				char buf[MAX_LINE];

				possession(usr->name, "Home", buf, MAX_LINE);
				if (strcmp(rm->name, buf))
					Put(usr, "<white>Note: <red>This only works for your own <yellow>Home><red> room\n");
			}
			switch(room_access(rm, usr->name)) {
				case ACCESS_HIDDEN:
					Put(usr, "<red>No such room\n");
					unload_Room(rm);
					RET(usr);
					Return;

				case ACCESS_INVITE_ONLY:
					Put(usr, "<red>That room is invite-only, and you have not been invited\n");
					unload_Room(rm);
					RET(usr);
					Return;

				case ACCESS_KICKED:
					Put(usr, "<red>You have been kicked from that room\n");
					unload_Room(rm);
					RET(usr);
					Return;

				case ACCESS_INVITED:
				case ACCESS_OK:
					break;

				default:
					Perror(usr, "failed to check whether you have access or not");
					unload_Room(rm);
					RET(usr);
					Return;
			}
			usr->default_room = rm->number;
			usr->runtime_flags |= RTF_CONFIG_EDITED;

			unload_Room(rm);
			RET(usr);
			Return;
	}
	Return;
}

void state_config_timezone(User *usr, char c) {
char buf[MAX_LINE], *p;
DirList *dl;

	if (usr == NULL)
		return;

	Enter(state_config_timezone);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			print_calendar(usr);

/* filter underscores */
			if (usr->timezone == NULL)
				cstrcpy(buf, "GMT", MAX_LINE);
			else {
				cstrncpy(buf, usr->timezone, MAX_LINE);
				p = buf;
				while((p = cstrchr(p, '_')) != NULL)
					*p = ' ';
			}
			Print(usr, "<magenta>\n"
				"<hotkey>Display time as          <white>%s<magenta>\n"
				"<hotkey>Select time zone         <white>%s (%s)<magenta>\n",
	
				(usr->flags & USR_12HRCLOCK) ? "12 hour clock (AM/PM)" : "24 hour clock",
				buf, name_Timezone(usr->tz)
			);
			read_menu(usr);
			Return;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_ESC:
		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Config menu\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'd':
		case 'D':
			CONFIG_OPTION(USR_12HRCLOCK, "Display time");

		case 's':
		case 'S':
			Put(usr, "Select time zone\n");

			if ((dl = list_DirList(PARAM_ZONEINFODIR, IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_SLASHES)) == NULL) {
				log_err("state_config_timezone(): list_DirList(%s) failed", PARAM_ZONEINFODIR);
				Put(usr, "<red>Sorry, the time zone system appears to be offline\n\n");
				CURRENT_STATE(usr);
				Return;
			}
			if (count_Queue(dl->list) <= 0) {
				log_warn("state_config_timezone(): zoneinfo files are missing from %s", PARAM_ZONEINFODIR);
				Put(usr, "<red>Sorry, the time zone system appears to be offline\n\n");
				destroy_DirList(dl);
				CURRENT_STATE(usr);
				Return;
			}
			PUSH_ARG(usr, &dl, sizeof(DirList *));
			CALL(usr, STATE_TZ_CONTINENT);
			Return;
	}
	Print(usr, "<yellow>\n[Config] Time Zone%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_tz_continent(User *usr, char c) {
DirList *dl;
int r;

	Enter(state_tz_continent);

	PEEK_ARG(usr, &dl, sizeof(DirList *));
	if (dl == NULL || dl->list == NULL) {
		Perror(usr, "The zoneinfo listing has disappeared");
		POP_ARG(usr, &dl, sizeof(DirList *));
		destroy_DirList(dl);
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			edit_number(usr, EDIT_INIT);

			buffer_text(usr);
			Put(usr, "\n<magenta>Time zone regions\n");
			print_columns(usr, (StringList *)dl->list->tail, FORMAT_NUMBERED|FORMAT_NO_UNDERSCORES);
			read_menu(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		default:
			r = edit_number(usr, c);
			if (r == EDIT_RETURN) {
				if (!usr->edit_buf[0])
					r = EDIT_BREAK;
				else {
					char dirname[MAX_PATHLEN];
					StringList *sl;
					DirList *dl2;
					int n;

					n = atoi(usr->edit_buf);
					if (n <= 0) {
						Put(usr, "<red>Invalid region\n");
						CURRENT_STATE(usr);
						Return;
					}
					n--;
					for(sl = (StringList *)dl->list->tail; n != 0 && sl != NULL; sl = sl->next)
						n--;

					if (sl == NULL) {
						Put(usr, "<red>Invalid region\n");
						CURRENT_STATE(usr);
						Return;
					}
					bufprintf(dirname, sizeof(dirname), "%s/%s", PARAM_ZONEINFODIR, sl->str);
					if ((dl2 = list_DirList(dirname, IGNORE_HIDDEN|NO_DIRS)) == NULL) {
						log_err("state_tz_continent(): list_DirList(%s) failed", dirname);
						Put(usr, "<red>Sorry, the time zone system appears to be offline\n\n");
						POP_ARG(usr, &dl, sizeof(DirList *));
						destroy_DirList(dl);
						RET(usr);
						Return;
					}
					if (count_Queue(dl2->list) <= 0) {
						log_warn("state_tz_continent(): zoneinfo files are missing from %s", dirname);
						Put(usr, "<red>Sorry, the time zone system appears to be offline\n\n");
						destroy_DirList(dl2);
						POP_ARG(usr, &dl, sizeof(DirList *));
						destroy_DirList(dl);
						RET(usr);
						Return;
					}
					Free(usr->tmpbuf[TMP_NAME]);
					if ((usr->tmpbuf[TMP_NAME] = cstrdup(sl->str)) == NULL) {
						Perror(usr, "Out of memory");
						destroy_DirList(dl2);
						POP_ARG(usr, &dl, sizeof(DirList *));
						destroy_DirList(dl);
						RET(usr);
						Return;
					}
					PUSH_ARG(usr, &dl2, sizeof(DirList *));
					CALL(usr, STATE_TZ_CITY);
					Return;
				}
			}
			if (r == EDIT_BREAK) {
				POP_ARG(usr, &dl, sizeof(DirList *));
				destroy_DirList(dl);
				RET(usr);
				Return;
			}
			Return;
	}
	Put(usr, "\n<green>What part of the world are you in? <yellow>");
	Return;
}

/*
	usr->tmpbuf[0] is the previously selected region
*/
void state_tz_city(User *usr, char c) {
DirList *dl;
int r;
char zonename[MAX_PATHLEN], *p;

	Enter(state_tz_city);

	PEEK_ARG(usr, &dl, sizeof(DirList *));
	if (dl == NULL || dl->list == NULL) {
		Perror(usr, "The zoneinfo listing has disappeared");
		POP_ARG(usr, &dl, sizeof(DirList *));
		destroy_DirList(dl);
		Free(usr->tmpbuf[0]);
		usr->tmpbuf[0] = NULL;
		RET(usr);
		Return;
	}
	if (usr->tmpbuf[0] == NULL) {
		Perror(usr, "Can't remember what your previous choice was!");
		POP_ARG(usr, &dl, sizeof(DirList *));
		destroy_DirList(dl);
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			edit_number(usr, EDIT_INIT);

			cstrcpy(zonename, usr->tmpbuf[0], MAX_PATHLEN);
			p = zonename;
			while((p = cstrchr(p, '_')) != NULL)
				*p = ' ';

			buffer_text(usr);
			Print(usr, "\n<green>Cities, countries, regions, and zones in category <yellow>%s:\n\n", zonename);
			print_columns(usr, (StringList *)dl->list->tail, FORMAT_NUMBERED|FORMAT_NO_UNDERSCORES);
			read_menu(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		default:
			r = edit_number(usr, c);
			if (r == EDIT_RETURN) {
				if (!usr->edit_buf[0])
					r = EDIT_BREAK;
				else {
					StringList *sl;
					int n;
					Timezone *tz;

					n = atoi(usr->edit_buf);
					if (n <= 0) {
						Put(usr, "<red>Invalid choice\n");
						CURRENT_STATE(usr);
						Return;
					}
					n--;
					for(sl = (StringList *)dl->list->tail; n != 0 && sl != NULL; sl = sl->next)
						n--;

					if (sl == NULL) {
						Put(usr, "<red>Invalid choice\n");
						CURRENT_STATE(usr);
						Return;
					}
					bufprintf(zonename, sizeof(zonename), "%s/%s", usr->tmpbuf[0], sl->str);
					path_strip(zonename);
					if ((tz = load_Timezone(zonename)) == NULL) {
						Put(usr, "<red>That time zone cannot be used right now. Please try again later\n");
						state_tz_return(usr);
						Return;
					}
					unload_Timezone(usr->timezone);
					usr->tz = NULL;
					Free(usr->timezone);
					if ((usr->timezone = cstrdup(zonename)) == NULL) {
						log_err("state_tz_city(): out of memory setting usr->timezone to %s for user %s", zonename, usr->name);
						Print(usr, "<red>Failed to set new time zone to <yellow>%s\n", zonename);
						state_tz_return(usr);
						Return;
					}
					usr->tz = tz;
					state_tz_return(usr);
					Return;
				}
			}
			if (r == EDIT_BREAK) {
				Free(usr->tmpbuf[0]);
				usr->tmpbuf[0] = NULL;
				POP_ARG(usr, &dl, sizeof(DirList *));
				destroy_DirList(dl);
				RET(usr);
				Return;
			}
			Return;
	}
	Put(usr, "\n<green>Enter city, country, or region near you: <yellow>");
	Return;
}

void state_tz_return(User *usr) {
DirList *dl;

	if (usr == NULL)
		return;

	Free(usr->tmpbuf[0]);
	usr->tmpbuf[0] = NULL;

	POP_ARG(usr, &dl, sizeof(DirList *));	/* argument to state_tz_city */
	destroy_DirList(dl);
	POP(usr);								/* state_tz_city */
	POP_ARG(usr, &dl, sizeof(DirList *));	/* argument to state_tz_continent */
	RET(usr);								/* pops state_tz_continent, returning to state_config_timezone */
}


/*
	set a configurable string
*/
void change_config(User *usr, char c, char **var, char *prompt) {
int r;

	if (usr == NULL || var == NULL)
		return;

	Enter(change_config);

	if (c == INIT_STATE && prompt != NULL)
		Put(usr, prompt);

	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			cstrip_line(usr->edit_buf);

			if (!usr->edit_buf[0]) {
				Free(*var);
				*var = NULL;
			} else {
				char *s;

				if ((s = cstrdup(usr->edit_buf)) == NULL) {
					Perror(usr, "Out of memory");
					RET(usr);
					Return;
				}
				Free(*var);
				*var = s;
			}
			usr->runtime_flags |= RTF_CONFIG_EDITED;
		} else
			if (var != NULL && *var != NULL && **var)
				Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}

/* EOB */
