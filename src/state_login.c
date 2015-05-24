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
	state_login.c	WJ99
*/

#include "config.h"
#include "defines.h"
#include "debug.h"
#include "state_login.h"
#include "state_room.h"
#include "state.h"
#include "state_msg.h"
#include "state_sysop.h"
#include "edit.h"
#include "inet.h"
#include "util.h"
#include "log.h"
#include "main.h"
#include "passwd.h"
#include "Stats.h"
#include "timeout.h"
#include "CallStack.h"
#include "screens.h"
#include "cstring.h"
#include "make_dir.h"
#include "Param.h"
#include "copyright.h"
#include "access.h"
#include "Memory.h"
#include "OnlineUser.h"
#include "SU_Passwd.h"
#include "Timezone.h"
#include "Wrapper.h"
#include "sys_time.h"
#include "bufprintf.h"
#include "helper.h"
#include "NewUserLog.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

StringList *banished = NULL;

char *Str_Really_Logout[] = {
	"Really logout",
	"Are you sure",
	"Are you sure you are sure",
	"Are you sure you want to logout",
	"Do you really wish to logout",
	"Really logout from the BBS"
};


void state_login_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_login_prompt);

	if (c == INIT_STATE) {
		usr->login_time++;
		if (usr->login_time > MAX_LOGIN_ATTEMPTS) {
			Put(usr, "Bye! Come back when you've figured it out..!\n");
			usr->name[0] = 0;
			close_connection(usr, "too many attempts");
			Return;
		}
		Put(usr, "Enter your name: ");

		Free(usr->tmpbuf[TMP_NAME]);
		usr->tmpbuf[TMP_NAME] = NULL;

		Free(usr->tmpbuf[TMP_PASSWD]);
		usr->tmpbuf[TMP_PASSWD] = NULL;

		usr->online_timer = (unsigned long)rtc;
		usr->total_time = 0UL;
	}
	r = edit_name(usr, c);

	if (r == EDIT_BREAK) {
		Print(usr, "\nBye, and have a nice life!\n");
		close_connection(usr, "user hit break on the login prompt");
		Return;
	}
	if (r == EDIT_RETURN) {
		Free(usr->tmpbuf[TMP_NAME]);
		if ((usr->tmpbuf[TMP_NAME] = (char *)Malloc(MAX_NAME, TYPE_CHAR)) == NULL) {
			Perror(usr, "Out of memory");
			close_connection(usr, "out of memory");
			Return;
		}
		strncpy(usr->tmpbuf[TMP_NAME], usr->edit_buf, MAX_NAME);
		usr->tmpbuf[TMP_NAME][MAX_NAME-1] = 0;

		usr->edit_buf[0] = 0;
		usr->edit_pos = 0;

		if (!usr->tmpbuf[TMP_NAME][0]) {
			Put(usr, "\nPress Ctrl-D to exit\n");
			Put(usr, "Enter your name: ");
			Return;
		}
/*
	check for PARAM_NAME_SYSOP as well, by Richard of MatrixBBS
*/
		if (!strcmp(usr->tmpbuf[TMP_NAME], "Sysop") || !strcmp(usr->tmpbuf[TMP_NAME], PARAM_NAME_SYSOP)) {
			Print(usr, "You are not a Sysop, nor a %s. Go away!\n", PARAM_NAME_SYSOP);
			close_connection(usr, "attempt to login as Sysop");
			Return;
		}
/*
	nologin is active and user is not a sysop
*/
		if (nologin_active && !is_sysop(usr->tmpbuf[TMP_NAME])) {
			Put(usr, "\n");
			display_screen(usr, PARAM_NOLOGIN_SCREEN);
			close_connection(usr, "connection closed by nologin");
			Return;
		}
/*
	Note: it's possible to banish 'New', so no new users can be added
*/
		if (in_StringList(banished, usr->tmpbuf[TMP_NAME]) != NULL) {
			Put(usr, "\nYou have been denied access to this BBS\n");
			close_connection(usr, "user %s has been banished", usr->tmpbuf[TMP_NAME]);
			Return;
		}
		if (!strcmp(usr->tmpbuf[TMP_NAME], "New")) {
			JMP(usr, STATE_NEW_LOGIN_PROMPT);
			Return;
		}
		if (is_guest(usr->tmpbuf[TMP_NAME])) {
/* give Guest an appropriate login name */
			if (is_online(PARAM_NAME_GUEST) == NULL)
				cstrcpy(usr->name, PARAM_NAME_GUEST, MAX_NAME);
			else {
				for(r = 2; r < MAX_GUESTS; r++) {
					bufprintf(usr->tmpbuf[TMP_NAME], MAX_NAME, "%s %d", PARAM_NAME_GUEST, r);
					if (is_online(usr->tmpbuf[TMP_NAME]) == NULL)
						break;
				}
				if (r >= MAX_GUESTS) {
					Print(usr, "There are too many %s users online, please try again later\n", PARAM_NAME_GUEST);
					close_connection(usr, "too many guest users online");
					Return;
				}
				cstrcpy(usr->name, usr->tmpbuf[TMP_NAME], MAX_NAME);
			}
			log_auth("LOGIN %s (%s)", usr->name, usr->conn->hostname);

			usr->doing = cstrdup("is just looking around");
			usr->flags |= USR_X_DISABLED;
			usr->login_time = usr->online_timer = (unsigned long)rtc;

/* give Guest a default timezone */
			if (usr->timezone == NULL)
				usr->timezone = cstrdup(PARAM_DEFAULT_TIMEZONE);
			if (usr->tz == NULL)
				usr->tz = load_Timezone(usr->timezone);

			JMP(usr, STATE_ANSI_PROMPT);
			Return;
		}
		if (!user_exists(usr->tmpbuf[TMP_NAME])) {
			Put(usr, "No such user. ");

/* I said, it's possible to banish 'New', so no new users can be added */
			if (in_StringList(banished, "New") != NULL) {
				Put(usr, "\n\n");
				CURRENT_STATE(usr);
				Return;
			}
			JMP(usr, STATE_NEW_ACCOUNT_YESNO);		/* unknown user; create new account? */
		} else {
			if (load_User(usr, usr->tmpbuf[TMP_NAME], LOAD_USER_PASSWORD)) {
				Perror(usr, "Failed to load user file");
				CURRENT_STATE(usr);
				Return;
			}
			JMP(usr, STATE_PASSWORD_PROMPT);
		}
	}
	Return;
}

void state_new_account_yesno(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_new_account_yesno);

	if (c == INIT_STATE) {
		Put(usr, "Do you wish to create a new user? (y/N): ");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			if (!allow_Wrapper(usr->conn->ipnum, WRAPPER_NEW_USER)) {
				Put(usr, "\nSorry, but you're connecting from a site that has been "
					"locked out of the BBS.\n");
				close_connection(usr, "new user login closed by wrapper");
				Return;
			}
			cstrcpy(usr->edit_buf, usr->tmpbuf[TMP_NAME], MAX_LINE);
			usr->edit_pos = strlen(usr->edit_buf);

			MOV(usr, STATE_NEW_LOGIN_PROMPT);
			state_new_login_prompt(usr, KEY_RETURN);
			break;

		case YESNO_NO:
			Put(usr, "\n");
			JMP(usr, STATE_LOGIN_PROMPT);
			break;

		case YESNO_UNDEF:
			Put(usr, "Create a new user, <hotkey>yes or <hotkey>no? (y/N): ");
	}
	Return;
}

void state_password_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_password_prompt);

	if (c == INIT_STATE) {
		usr->name[0] = 0;				/* this keeps close_connection() from saving the user file (found by Georbit) */
		Put(usr, "Enter password: ");
	}
	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
		Print(usr, "\nBye, and have a nice life!\n");
		close_connection(usr, "user hit break on the login prompt");
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!verify_phrase(usr->edit_buf, usr->passwd)) {
			User *u;
			int load_flags;

			clear_password_buffer(usr);

			load_flags = LOAD_USER_ALL;
			if (!PARAM_HAVE_RESIDENT_INFO)				/* deferred loading of profile */
				load_flags &= ~LOAD_USER_INFO;

			if (load_User(usr, usr->tmpbuf[TMP_NAME], load_flags)) {
				Perror(usr, "failed to load user data");
				close_connection(usr, "failed to load user file");
				Return;
			}
			Put(usr, "<normal>");

			if ((u = is_online(usr->tmpbuf[TMP_NAME])) != NULL) {
				Print(u, "\n<red>Connection closed by another login from %s\n", usr->conn->hostname);
				close_connection(u, "connection closed by another login");
			}
			cstrcpy(usr->name, usr->tmpbuf[TMP_NAME], MAX_NAME);
			log_auth("LOGIN %s (%s)", usr->name, usr->conn->hostname);

			if (u == NULL)				/* if (u != NULL) killed by another login */
				notify_login(usr);		/* tell friends we're here */
			else
				if (u->runtime_flags & RTF_LOCKED)
					notify_unlocked(usr);

			JMP(usr, STATE_DISPLAY_MOTD);
		} else {
			clear_password_buffer(usr);
			Put(usr, "Wrong password!\n\n");
			log_auth("WRONGPASSWD %s (%s)", usr->tmpbuf[TMP_NAME], usr->conn->hostname);
			JMP(usr, STATE_LOGIN_PROMPT);
		}
	}
	Return;
}


void state_logout_prompt(User *usr, char c) {
char buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(state_logout_prompt);

	if (c == INIT_STATE) {
		if ((usr->runtime_flags & RTF_HOLD) && usr->held_msgs != NULL)
			Print(usr, "<green>You have unread messages held\n");
		else {
			User *u;

			for(u = AllUsers; u != NULL; u = u->next) {
				if (u->runtime_flags & RTF_BUSY) {
					if ((u->runtime_flags & RTF_BUSY_SENDING)
						&& in_StringQueue(u->recipients, usr->name) != NULL) {
/*
	warn follow-up mode by Richard of MatrixBBS
*/
						if (u->flags & USR_FOLLOWUP)
							Print(usr, "<yellow>%s<green> is busy sending you a message in follow-up mode\n", u->name);
						else
							Print(usr, "<yellow>%s<green> is busy sending you a message\n", u->name);
					} else {
						if ((u->runtime_flags & RTF_BUSY_MAILING)
							&& u->new_message != NULL
							&& in_MailToQueue(u->new_message->to, usr->name) != NULL)
							Print(usr, "<yellow>%s<green> is busy mailing you a message\n", u->name);
					}
				}
			}
		}
		bufprintf(buf, sizeof(buf), "<cyan>%s? ", RND_STR(Str_Really_Logout));
		Put(usr, buf);
		Put(usr, "(y/N): <white>");
		usr->runtime_flags |= RTF_BUSY;
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			notify_logout(usr);

			Put(usr, "\n");
/*
	mail any X messages that were received while we were pressing 'Y'
*/
			if (usr->held_msgs != NULL) {
				PList *pl;
				BufferedMsg *m;
				User *u;

				for(pl = usr->held_msgs; pl != NULL; pl = pl->next) {
					m = (BufferedMsg *)pl->p;

					if ((u = is_online(m->from)) != NULL)
						mail_lost_msg(u, m, usr);
				}
			}
			display_screen(usr, PARAM_LOGOUT_SCREEN);
			close_connection(usr, "LOGOUT %s (%s)", usr->name, usr->conn->hostname);
			Return;

		case YESNO_NO:
			RET(usr);
			break;

		default:
			bufprintf(buf, sizeof(buf), "<cyan>%s? ", RND_STR(Str_Really_Logout));
			Put(usr, buf);
			Put(usr, "(y/N): <white>");
	}
	Return;
}

/*
	this function looks a lot like state.c:mail_msg(), but it is not
	quite the same ... (that's what you get with a state machine)

	It saves the message as Mail>, but does not create a sender's copy
*/
void mail_lost_msg(User *from, BufferedMsg *msg, User *to) {
Message *new_msg;
int flags;
char buf[PRINT_BUF], filename[MAX_PATHLEN];
MailTo *mailto;

	if (from == NULL || msg == NULL || to == NULL)
		return;

	Enter(mail_lost_msg);

	if ((new_msg = new_Message()) == NULL) {
		Perror(to, "Out of memory");
		Return;
	}
	cstrcpy(new_msg->from, from->name, MAX_NAME);

	if ((new_msg->to = new_MailToQueue()) == NULL) {
		Perror(to, "Out of memory");
		destroy_Message(new_msg);
		Return;
	}
	if ((mailto = new_MailTo()) == NULL) {
		Perror(to, "Out of memory");
		Return;
	}
	mailto->name = to->name;
	(void)add_MailToQueue(new_msg->to, mailto);

	new_msg->subject = cstrdup("<lost message>");

	if ((new_msg->msg = new_StringIO()) == NULL) {
		Perror(to, "Out of memory");
		destroy_Message(new_msg);
		Return;
	}
	put_StringIO(new_msg->msg, "<cyan>Delivery of this message was impossible. You do get it this way.\n \n");

	flags = to->flags;
	to->flags &= ~USR_XMSG_NUM;
	buffered_msg_header(to, msg, buf, PRINT_BUF);
	to->flags = flags;

	put_StringIO(new_msg->msg, buf);
	concat_StringIO(new_msg->msg, msg->msg);

	new_msg->number = room_top(to->mail)+1;
	bufprintf(filename, sizeof(filename), "%s/%c/%s/%lu", PARAM_USERDIR, to->name[0], to->name, new_msg->number);
	path_strip(filename);

	if (save_Message(new_msg, filename, 0))
		Perror(to, "Error saving mail");
	else
		newMsg(to->mail, to);

	destroy_Message(new_msg);
	Return;
}

void state_ansi_prompt(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_ansi_prompt);

	if (c == INIT_STATE) {
		Put(usr, "Are you on an ANSI terminal? (Y/n): ");
		usr->runtime_flags |= RTF_BUSY;
		Return;
	}
	switch(yesno(usr, c, 'Y')) {
		case YESNO_YES:
			usr->flags |= (USR_ANSI | USR_BOLD);		/* assume bold */
			Put(usr, "<normal>");
			break;

		case YESNO_NO:
			usr->flags &= ~(USR_ANSI | USR_BOLD);
			break;

		default:
			Put(usr, "\nAre you on an ANSI terminal, <hotkey>yes or <hotkey>no? (Y/n): ");
			Return;
	}
	if (load_screen(usr->text, PARAM_FIRST_LOGIN) >= 0) {
/*
	for the new users, we reset the timeout timer here so they have some
	time to read the displayed text
	and re-insert it into the sorted timerq
*/
		if (usr->idle_timer != NULL) {
			usr->idle_timer->maxtime = PARAM_IDLE_TIMEOUT * SECS_IN_MIN;
			usr->idle_timer->restart = TIMEOUT_USER;
			usr->idle_timer->action = user_timeout;
			set_Timer(&usr->timerq, usr->idle_timer, usr->idle_timer->maxtime);
		}
		MOV(usr, STATE_DISPLAY_MOTD);
		PUSH(usr, STATE_PRESS_ANY_KEY);
		read_text(usr);
		Return;
	}
	JMP(usr, STATE_DISPLAY_MOTD);
	Return;
}

void state_display_motd(User *usr, char c) {
File *f;

	Enter(state_display_motd);

	if (usr->idle_timer != NULL) {			/* reset the 'timeout timer' */
		usr->idle_timer->maxtime = PARAM_IDLE_TIMEOUT * SECS_IN_MIN;
		usr->idle_timer->restart = TIMEOUT_USER;
		usr->idle_timer->action = user_timeout;
		set_Timer(&usr->timerq, usr->idle_timer, usr->idle_timer->maxtime);
	}
	if ((f = Fopen(PARAM_MOTD_SCREEN)) != NULL) {
		Fget_StringIO(f, usr->text);
		Fclose(f);

		POP(usr);							/* remove this call, it's not needed anymore */
		PUSH(usr, STATE_GO_ONLINE);
		Put(usr, "<green>");
		read_text(usr);						/* read with --More-- prompt */
		Return;
	}
	JMP(usr, STATE_GO_ONLINE);
	Return;
}

void state_go_online(User *usr, char c) {
Joined *j;
Room *r;
char num_buf[MAX_NUMBER];
int i, new_mail;

	if (usr == NULL)
		return;

	Enter(state_go_online);

	if (!usr->birth)
		usr->birth = rtc;

/* do periodic saving of user files */
	add_Timer(&usr->timerq, new_Timer(PARAM_SAVE_TIMEOUT * SECS_IN_MIN, save_timeout, TIMER_RESTART));
/*
	give the user a Mail> room
	this is already done by load_User(), but Guests and New users don't have one yet
*/
	if (usr->mail == NULL) {
		int load_room_flags = LOAD_ROOM_ALL;

		if (!PARAM_HAVE_RESIDENT_INFO)
			load_room_flags &= ~LOAD_ROOM_INFO;

		if ((usr->mail = load_Mail(usr->name, load_room_flags)) == NULL) {
			Perror(usr, "Out of memory");
			close_connection(usr, "out of memory");
			Return;
		}
	}
/*
	fix last_read field if too large (fix screwed up mail rooms)
*/
	if ((j = in_Joined(usr->rooms, MAIL_ROOM)) != NULL) {
		if (usr->mail->head_msg <= 0L)
			j->last_read = 0UL;
		else {
			if (j->last_read > usr->mail->head_msg)
				j->last_read = usr->mail->head_msg;
			if (j->last_read < 0L)
				j->last_read = 0L;
		}
	} else {
		if ((j = new_Joined()) != NULL) {
			j->number = MAIL_ROOM;
			j->generation = usr->mail->generation;
			(void)prepend_Joined(&usr->rooms, j);
		}
	}
	usr->runtime_flags &= ~RTF_BUSY;
	usr->edit_buf[0] = 0;
	usr->login_time = usr->online_timer = (unsigned long)rtc;
	usr->logins++;

	stats.num_logins++;
	update_stats(usr);

	Put(usr, "\n");
	if (usr->logins <= 1) {
		Put(usr, "<green>This is your <yellow>1st<green> login\n");

		if (usr->doing == NULL) {
			char buf[MAX_LINE];

			bufprintf(buf, sizeof(buf), "is new to <white>%s", PARAM_BBS_NAME);
			usr->doing = cstrdup(buf);
		}
	} else {
		char exclaim[4];

/* yell out on a hundredth login */
		if (!(usr->logins % 100))
			cstrcpy(exclaim, "!!!", 4);
		else
			exclaim[0] = 0;

		Print(usr, "<green>Welcome back, <yellow>%s! <green>"
			"This is your <yellow>%s<green> login%s\n", usr->name, print_numberth(usr->logins, num_buf, sizeof(num_buf)), exclaim);
	}
/*
	note that the last IP was stored in tmpbuf[TMP_FROM_HOST] by load_User() in User.c
	note that new users do not have a last_logout time
*/
	if (usr->last_logout > (time_t)0UL) {
		char date_buf[MAX_LINE], online_for[MAX_LINE+10];

		if (usr->last_online_time > 0UL) {
			int l;

			l = bufprintf(online_for, sizeof(online_for), "%c, for %c", color_by_name("green"), color_by_name("yellow"));
			print_total_time(usr->last_online_time, online_for+l, sizeof(online_for) - l);
		} else
			online_for[0] = 0;

		if (usr->tmpbuf[TMP_FROM_HOST] != NULL && usr->tmpbuf[TMP_FROM_HOST][0])
			Print(usr, "\n<green>Last login was on <cyan>%s%s<green> from<yellow> %s\n", print_date(usr, usr->last_logout, date_buf, MAX_LINE), online_for, usr->tmpbuf[TMP_FROM_HOST]);
		else
			Print(usr, "\n<green>Last login was on <cyan>%s%s\n", print_date(usr, usr->last_logout, date_buf, MAX_LINE), online_for);
	}
/* free the tmp buffers as they won't be used anymore for a long time */
	for(i = 0; i < NUM_TMP; i++) {
		Free(usr->tmpbuf[i]);
		usr->tmpbuf[i] = NULL;
	}
	new_mail = print_user_status(usr);

	if (usr->flags & USR_HELPING_HAND)
		add_helper(usr);

	MOV(usr, STATE_ROOM_PROMPT);

/*
	if there are new Lobby posts, go to the Lobby> first regardless
	of whether you have new mail or not. New Mail> will be read right
	after having read the Lobby> anyway
*/
	if ((j = in_Joined(usr->rooms, LOBBY_ROOM)) != NULL && newMsgs(Lobby_room, j->last_read) >= 0)
		r = Lobby_room;
	else {
		if (PARAM_HAVE_MAILROOM && new_mail)
			r = usr->mail;
		else {
			if ((r = find_Roombynumber(usr, usr->default_room)) == NULL
				|| joined_room(usr, r) == NULL
				|| room_access(r, usr->name) < 0) {
				usr->default_room = LOBBY_ROOM;
				r = Lobby_room;
			}
		}
	}
	goto_room(usr, r);

	add_OnlineUser(usr);		/* add user to hash of online users */
	Return;
}


void state_new_login_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_new_login_prompt);

	if (c == INIT_STATE) {
		if (!allow_Wrapper(usr->conn->ipnum, WRAPPER_NEW_USER)) {
			Put(usr, "\nSorry, but you're connecting from a site that has been "
				"locked out of the BBS.\n");
			close_connection(usr, "new user login closed by wrapper");
			Return;
		}
		log_auth("NEWLOGIN (%s)", usr->conn->hostname);

		Put(usr, "\nHello there, new user! You may choose a name that suits you well.\n");
		Put(usr, "This name will be your alias for the rest of your BBS life.\n");
		Put(usr, "Enter your login name: ");

		Free(usr->tmpbuf[TMP_NAME]);
		usr->tmpbuf[TMP_NAME] = NULL;
	}
	r = edit_name(usr, c);

	if (r == EDIT_BREAK) {
		Print(usr, "\nBye, and have a nice life!\n");
		close_connection(usr, "user hit break on the login prompt");
		Return;
	}
	if (r == EDIT_RETURN) {
		char *name;

		name = usr->edit_buf;
		usr->edit_pos = 0;

		Free(usr->tmpbuf[TMP_NAME]);
		usr->tmpbuf[TMP_NAME] = NULL;

		if (!name[0]) {
			Put(usr, "\nPress Ctrl-D to exit\n");
			JMP(usr, STATE_LOGIN_PROMPT);
			Return;
		}
		if (!name[1]) {
			Put(usr, "\nThat name is too short\n");
			Put(usr, "Enter your login name: ");
			usr->edit_buf[0] = 0;
			Return;
		}
		if (in_StringList(banished, name) != NULL) {
			Put(usr, "\nYou have been denied access to this BBS.\n");
			close_connection(usr, "user has been banished");
			Return;
		}
		if (!strcmp(name, "New")) {
			Print(usr, "\nYou cannot use '%s' as login name, choose another login name\n", "New");
			Put(usr, "Enter your login name: ");
			usr->edit_buf[0] = 0;
			Return;
		}
		if (!strcmp(name, "Sysop") || !strcmp(name, PARAM_NAME_SYSOP) || is_guest(name)) {
			Print(usr, "\nYou cannot use '%s' as login name, choose another login name\n", name);
			Put(usr, "Enter your login name: ");
			usr->edit_buf[0] = 0;
			Return;
		}
		if (user_exists(name)) {
			Put(usr, "\nThat name already is in use, please choose an other login name\n"
				"Enter your login name: ");
			usr->edit_buf[0] = 0;
			Return;
		}
		if ((usr->tmpbuf[TMP_NAME] = cstrdup(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
			close_connection(usr, "out of memory");
			Return;
		}
		usr->edit_buf[0] = 0;

		Put(usr, "Now to choose a password. Passwords can be 79 characters long and can contain\n"
			"spaces and punctuation characters. Be sure not to use a password that can be\n"
			"guessed easily by anyone. Also be sure not to forget your own password..!\n");
		JMP(usr, STATE_NEW_PASSWORD_PROMPT);
	}
	Return;
}

void state_new_password_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_new_password_prompt);

	if (c == INIT_STATE)
		Put(usr, "Enter new password: ");

	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
		JMP(usr, STATE_LOGIN_PROMPT);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			JMP(usr, STATE_LOGIN_PROMPT);
			Return;
		}
		if (strlen(usr->edit_buf) < 5) {
			clear_password_buffer(usr);
			Put(usr, "\nThat password is too short\n");
			JMP(usr, STATE_NEW_PASSWORD_PROMPT);
			Return;
		}
/* check passwd same as username */
		if (!cstricmp(usr->edit_buf, usr->tmpbuf[TMP_NAME])) {
			clear_password_buffer(usr);
			Put(usr, "\nThat password is not good enough\n");
			JMP(usr, STATE_NEW_PASSWORD_PROMPT);
			Return;
		}
		Free(usr->tmpbuf[TMP_PASSWD]);
		if ((usr->tmpbuf[TMP_PASSWD] = cstrdup(usr->edit_buf)) == NULL) {
			clear_password_buffer(usr);
			Perror(usr, "Out of memory");
			close_connection(usr, "out of memory");
			Return;
		}
		clear_password_buffer(usr);
		JMP(usr, STATE_NEW_PASSWORD_AGAIN);
	}
	Return;
}

void state_new_password_again(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_new_password_again);

	if (c == INIT_STATE)
		Put(usr, "Enter it again (for verification): ");

	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
		Print(usr, "\nBye, and have a nice life!\n");
		close_connection(usr, "user hit break on the login prompt");
		Return;
	}
	if (r == EDIT_RETURN) {
		char crypted[MAX_CRYPTED], dirname[MAX_PATHLEN];
		int i;

		if (!usr->edit_buf[0]) {
			Put(usr, "Cancelled\n\n");
			JMP(usr, STATE_LOGIN_PROMPT);
			Return;
		}
		Put(usr, "\n");

		if (strcmp(usr->edit_buf, usr->tmpbuf[TMP_PASSWD])) {
			clear_password_buffer(usr);
			Put(usr, "Passwords didn't match!\n");
			JMP(usr, STATE_NEW_PASSWORD_PROMPT);
			Return;
		}
/*
	from here we have a name -- from here on others can see the new user online
*/
		cstrcpy(usr->name, usr->tmpbuf[TMP_NAME], MAX_NAME);
		i = strlen(usr->name)-1;
		if (usr->name[i] == ' ')
			usr->name[i] = 0;

		stats.youngest_birth = usr->birth = usr->login_time = usr->online_timer = rtc;
		cstrcpy(stats.youngest, usr->name, MAX_NAME);

		crypt_phrase(usr->edit_buf, crypted);
		crypted[MAX_CRYPTED_PASSWD-1] = 0;

		if (verify_phrase(usr->edit_buf, crypted)) {
			clear_password_buffer(usr);
			Perror(usr, "bug in password encryption -- please choose an other password");
			JMP(usr, STATE_NEW_PASSWORD_PROMPT);
			Return;
		}
		clear_password_buffer(usr);
		cstrcpy(usr->passwd, crypted, MAX_CRYPTED_PASSWD);

		bufprintf(dirname, sizeof(dirname), "%s/%c/%s", PARAM_USERDIR, usr->name[0], usr->name);
		path_strip(dirname);
		if (make_dir(dirname, (mode_t)0750))
			Perror(usr, "Failed to create user directory");

		log_auth("NEWUSER %s (%s)", usr->name, usr->conn->hostname);
		log_newuser(usr->name);

/* new user gets default timezone */
		if (usr->timezone == NULL)
			usr->timezone = cstrdup(PARAM_DEFAULT_TIMEZONE);
		if (usr->tz == NULL)
			usr->tz = load_Timezone(usr->timezone);

/* save user here, or we're not able to profile him yet! */
		if (save_User(usr)) {
			Perror(usr, "failed to save userfile");
		}
		JMP(usr, STATE_ANSI_PROMPT);
	}
	Return;
}

int print_user_status(User *usr) {
int num_users, num_friends, all_users, new_mail;
Joined *j;
User *u;

/* count number of users online */
	num_users = num_friends = all_users = 0;
	for(u = AllUsers; u != NULL; u = u->next) {
		if (u->name[0])
			all_users++;

		if (u == usr)
			continue;

		if (u->name[0]) {
			if (!(usr->flags & USR_SHOW_ENEMIES) && in_StringList(usr->enemies, u->name) != NULL)
				continue;

			num_users++;
		}
		if (in_StringList(usr->friends, u->name) != NULL)
			num_friends++;
	}
	if (!num_users)
		Put(usr, "<green>You are the one and only user online right now...\n");
	else {
		if (num_users == 1) {
			if (num_friends == 1)
				Put(usr, "<green>There is one friend online\n");
			else
				Put(usr, "<green>There is one other user online\n");
		} else {
			if (num_friends > 0) {
				num_users -= num_friends;
				Print(usr, "<green>There are <yellow>%d<green> friend%s and <yellow>%d<green> other user%s online\n",
					num_friends, (num_friends == 1) ? "" : "s",
					num_users, (num_users == 1) ? "" : "s");
			} else
				Print(usr, "<green>There are <yellow>%d<green> other users online\n", num_users);
		}
	}
	log_info("%d users online", all_users);

	if (!PARAM_HAVE_QUESTIONS)
		usr->flags &= ~USR_HELPING_HAND;

	if (usr->flags & USR_HELPING_HAND) {
		if (!is_sysop(usr->name) && usr->total_time / SECS_IN_DAY < PARAM_HELPER_AGE)
			usr->flags &= ~USR_HELPING_HAND;
		else {
			if (usr->flags & USR_X_DISABLED)
				usr->flags &= ~USR_HELPING_HAND;
			else
				Put(usr, "<magenta>You are available to help others\n");
		}
	}
	if (usr->flags & USR_X_DISABLED)
		Print(usr, "<magenta>Message reception is turned off%s\n", (usr->flags & USR_BLOCK_FRIENDS) ? ", and you are blocking Friends" : "");

	if (usr->runtime_flags & RTF_HOLD)
		Put(usr, "<magenta>You have put messages on hold\n");

	if (usr->reminder != NULL && usr->reminder[0])
		Print(usr, "\n<magenta>Reminder: <yellow>%s\n", usr->reminder);

/* bbs birthday */
	if (usr->logins > 1) {
		struct tm *tm;
		int bday_day, bday_mon, bday_year;
		char num_buf[MAX_NUMBER];

		tm = user_time(usr, usr->birth);
		bday_day = tm->tm_mday;
		bday_mon = tm->tm_mon;
		bday_year = tm->tm_year;

		tm = user_time(usr, (time_t)0UL);

		if (tm->tm_mday == bday_day && tm->tm_mon == bday_mon && tm->tm_year > bday_year)
			Print(usr, "\n<magenta>Today is your <yellow>%s<magenta> BBS birthday!\n",
				print_numberth(tm->tm_year - bday_year, num_buf, sizeof(num_buf)));
	}
	print_reboot_status(usr);

	if (nologin_active)
		Put(usr, "\n<white>NOTE: <red>nologin is active\n");

	new_mail = 0;
	if (PARAM_HAVE_MAILROOM && usr->mail != NULL && (j = in_Joined(usr->rooms, MAIL_ROOM)) != NULL
		&& newMsgs(usr->mail, j->last_read) > j->last_read) {
		new_mail = 1;
		Put(usr, "\n<beep><cyan>You have new mail\n");
	}
	return new_mail;
}

/* EOB */
