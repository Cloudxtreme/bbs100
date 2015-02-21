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
	state_history.c	WJ105
*/

#include "config.h"
#include "debug.h"
#include "state_history.h"
#include "state_room.h"
#include "state_msg.h"
#include "state.h"
#include "edit.h"
#include "util.h"
#include "access.h"
#include "screens.h"
#include "cstring.h"
#include "Param.h"
#include "Memory.h"
#include "helper.h"

static void generic_print_history(User *usr, BufferedMsg *m) {
	if (usr == NULL || m == NULL)
		return;

	print_buffered_msg(usr, m);

	deinit_StringQueue(usr->recipients);
	if (!strcmp(m->from, usr->name))
		(void)concat_StringQueue(usr->recipients, copy_StringList(m->to));
	else
		(void)add_StringQueue(usr->recipients, new_StringList(m->from));
}


int history_prompt(User *usr) {
BufferedMsg *m;

	if (usr == NULL)
		return -1;

	Enter(history_prompt);
	
	usr->history_p = unwind_BufferedMsg(usr->history);

	while(usr->history_p != NULL) {
		m = (BufferedMsg *)usr->history_p->p;
		if ((m->flags & BUFMSG_TYPE) != BUFMSG_ONESHOT)
			break;

		usr->history_p = usr->history_p->prev;
	}
	if (usr->history_p == NULL) {
		Put(usr, "<red>No messages have been sent yet\n");
		Return 0;
	}
	CALLX(usr, STATE_HISTORY_PROMPT, INIT_PROMPT);
	Return 1;
}

void state_history_prompt(User *usr, char c) {
BufferedMsg *m = NULL;
StringList *sl;
PList *pl;
int n, remaining;
char num_buf1[MAX_NUMBER], num_buf2[MAX_NUMBER];

	if (usr == NULL)
		return;

	Enter(state_history_prompt);

	switch(c) {
/*
	it's confusing, but I've reversed INIT_PROMPT and INIT_STATE here to make
	things look better; it will only reprint the message when explicitly asked
	for, and just the plain prompt on INIT_STATE
*/
		case INIT_PROMPT:
			if (usr->history_p == NULL)
				break;

			generic_print_history(usr, (BufferedMsg *)usr->history_p->p);
			usr->runtime_flags |= RTF_BUSY;
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			break;

		case 'b':
		case 'B':
		case KEY_BS:
			Put(usr, "Back\n");

			if (usr->history_p == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			usr->history_p = usr->history_p->prev;
			if (usr->history_p == NULL)
				break;

			generic_print_history(usr, (BufferedMsg *)usr->history_p->p);
			break;

		case 'n':
		case 'N':
		case ' ':
			Put(usr, "Next\n");

			if (usr->history_p == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			usr->history_p = usr->history_p->next;
			if (usr->history_p == NULL) {
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			generic_print_history(usr, (BufferedMsg *)usr->history_p->p);
			break;

		case 'a':
		case 'A':
			Put(usr, "Again");
		case KEY_CTRL('L'):
			Put(usr, "\n");
			if (usr->history_p == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			generic_print_history(usr, (BufferedMsg *)usr->history_p->p);
			break;

		case '-':
		case '_':
			Put(usr, "<white>Read last\n");
			CALL(usr, STATE_HIST_MINUS);
			Return;

		case '+':
		case '=':
			Put(usr, "<white>Skip forward\n");
			CALL(usr, STATE_HIST_PLUS);
			Return;

		case '#':
			Put(usr, "<white>Message number\n");
			CALL(usr, STATE_HIST_MSG_NUMBER);
			Return;

		case 'r':
		case 'v':
			Put(usr, "Reply\n");
			goto History_Reply_Code;

		case 'R':
		case KEY_CTRL('R'):
		case 'V':
			Put(usr, "Reply to All\n");

History_Reply_Code:
			if (usr->runtime_flags & RTF_HOLD) {
				Put(usr, "<magenta>You have put messages on hold\n");
				break;
			}
			if (usr->history_p == NULL) {
				Put(usr, "<red>No message to reply to\n");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			deinit_StringQueue(usr->recipients);

			m = (BufferedMsg *)usr->history_p->p;

			if (c == 'R' || c == 'V' || c == 'A' || c == 'a') {
				if (concat_StringQueue(usr->recipients, copy_StringList(m->to)) == NULL) {
					Perror(usr, "Out of memory");
					deinit_StringQueue(usr->recipients);
				}
			}
			if ((sl = in_StringQueue(usr->recipients, m->from)) == NULL) {
				if ((sl = new_StringList(m->from)) == NULL) {
					Perror(usr, "Out of memory");
				} else
					(void)add_StringQueue(usr->recipients, sl);
			}
			check_recipients(usr);
			if (count_Queue(usr->recipients) <= 0)
				break;

			do_reply_x(usr, m->flags);
			Return;

		case 'l':
		case 'L':
			Put(usr, "List recipients\n");

			if (usr->history_p == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			m = (BufferedMsg *)usr->history_p->p;
			if (m->to != NULL && m->to->next != NULL)
				Put(usr, "<magenta>Recipients are <yellow>");
			else
				Put(usr, "<magenta>The recipient is <yellow>");

			for(sl = m->to; sl != NULL; sl = sl->next) {
				if (sl->next == NULL)
					Print(usr, "%s\n", sl->str);
				else
					Print(usr, "%s, ", sl->str);
			}
			break;

		case 's':
		case 'S':
		case 'q':
		case 'Q':
		case KEY_ESC:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			if (c == 'q' || c == 'Q')
				Put(usr, "Quit\n");
			else
				Put(usr, "Stop\n");
			usr->runtime_flags &= ~RTF_BUSY;
			RET(usr);
			Return;

		default:
			if (fun_common(usr, c)) {				/* Note: may reset RTF_BUSY flag(!) */
				Return;
			}
	}
	if (usr->history_p == NULL) {
		usr->runtime_flags &= ~RTF_BUSY;
		RET(usr);
		Return;
	}
	usr->runtime_flags |= RTF_BUSY;

	n = 1;
	for(pl = rewind_BufferedMsg(usr->history); pl != NULL; pl = pl->next) {
		if (pl == usr->history_p)
			break;

		n++;
	}
/*
	this is not very efficient for large histories ...
*/
	remaining = 0;
	while(pl != NULL) {
		pl = pl->next;
		remaining++;
	}
	if (remaining > 0)
		remaining--;

	Print(usr, "<yellow>\n[X History]<magenta> msg #%s (%s remaining) %c<white> ",
		print_number(n, num_buf1, sizeof(num_buf1)),
		print_number(remaining, num_buf2, sizeof(num_buf2)),
		(usr->runtime_flags & RTF_SYSOP) ? '#' : '>'
	);
	Return;
}

/*
	here some more generic static functions for the functions
	'#'  Enter message number
	'-'  Read back
	'+'  Skip messages

	because the codes are all identical, only work on different set of pointers
*/

static void generic_history_msg_number(User *usr, char c, PList *root, PList **pointer) {
int r;

	if (usr == NULL || pointer == NULL)
		return;

	Enter(state_hist_msg_number);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter message number: <yellow>");

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		int num;
		PList *pl;

		num = atoi(usr->edit_buf);
		if (num <= 0) {
			*pointer = root;
			RET(usr);
			Return;
		}
		num--;
		for(pl = root; pl != NULL && pl->next != NULL && num > 0; pl = pl->next)
			num--;

		if (pl == NULL)
			Put(usr, "<red>No such message\n");
		else
			*pointer = pl;

		RETX(usr, INIT_PROMPT);
	}
	Return;
}

static void generic_history_minus(User *usr, char c, PList **pointer) {
int r;

	if (usr == NULL || pointer == NULL)
		return;

	Enter(state_enter_minus_msg);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter number of messages to read back: <yellow>");

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		int i;
		PList *pl;

		i = atoi(usr->edit_buf);
		if (i <= 0) {
			RET(usr);
			Return;
		}
		for(pl = *pointer; pl != NULL && pl->prev != NULL && i > 0; pl = pl->prev)
			i--;

		if (pl == NULL)
			Put(usr, "<red>No messages\n");
		else
			*pointer = pl;

		RETX(usr, INIT_PROMPT);
	}
	Return;
}

static void generic_history_plus(User *usr, char c, PList **pointer) {
int r;

	if (usr == NULL || pointer == NULL)
		return;

	Enter(state_enter_plus_msg);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter number of messages to skip: <yellow>");

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		int i;
		PList *pl;

		i = atoi(usr->edit_buf);
		if (i <= 0) {
			RET(usr);
			Return;
		}
		for(pl = *pointer; pl != NULL && pl->next != NULL && i > 0; pl = pl->next)
			i--;

		if (pl == NULL)
			Put(usr, "<red>No messages\n");
		else
			*pointer = pl;

		RETX(usr, INIT_PROMPT);
	}
	Return;
}


void state_hist_msg_number(User *usr, char c) {
	generic_history_msg_number(usr, c, usr->history, &usr->history_p);
}

void state_hist_minus(User *usr, char c) {
	generic_history_minus(usr, c, &usr->history_p);
}

void state_hist_plus(User *usr, char c) {
	generic_history_plus(usr, c, &usr->history_p);
}


/*
	for messages that were held
	pretty much the same function as above, but now for held_msgp
	instead of history_p

	Mind that there are subtle differences between the functions for held_history
	and plain history
*/
int held_history_prompt(User *usr) {
BufferedMsg *m;

	if (usr == NULL)
		return -1;

	Enter(held_history_prompt);

	if (usr->held_msgp == NULL)
		usr->held_msgp = usr->held_msgs;

	while(usr->held_msgp != NULL) {
		m = (BufferedMsg *)usr->held_msgp->p;
		if ((m->flags & BUFMSG_TYPE) != BUFMSG_ONESHOT)
			break;

		usr->held_msgp = usr->held_msgp->prev;
	}
	CALLX(usr, STATE_HELD_HISTORY_PROMPT, INIT_PROMPT);
	Return 1;
}

void state_held_history_prompt(User *usr, char c) {
StringList *sl;
PList *pl, *pl_next;
BufferedMsg *m;
int printed, n, remaining;
char num_buf1[MAX_NUMBER], num_buf2[MAX_NUMBER];

	if (usr == NULL)
		return;

	Enter(state_held_history_prompt);

/*
	spit out any received one-shot messages
	it is not as elegant, but you have to deal with them some time
*/
	printed = 0;
	for(pl = usr->held_msgs; pl != NULL; pl = pl_next) {
		pl_next = pl->next;

		m = (BufferedMsg *)pl->p;
		if ((m->flags & BUFMSG_TYPE) == BUFMSG_ONESHOT) {		/* one shot message */
			display_text(usr, m->msg);
			printed++;
			remove_BufferedMsg(&usr->held_msgs, m);
			destroy_BufferedMsg(m);				/* one-shots are not remembered */
		}
	}
	if (printed)
		Put(usr, "<white>");					/* do color correction */

	switch(c) {
		case INIT_PROMPT:
			if (usr->held_msgp == NULL) {
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			generic_print_history(usr, (BufferedMsg *)usr->held_msgp->p);
			usr->runtime_flags |= RTF_BUSY;
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			break;

		case 'b':
		case 'B':
		case KEY_BS:
			Put(usr, "Back\n");

			if (usr->held_msgp == NULL) {
				Perror(usr, "Your held messages buffer is gone");
				goto Exit_Held_History;
			}
			usr->held_msgp = usr->held_msgp->prev;
			if (usr->held_msgp == NULL)
				goto Exit_Held_History;

			generic_print_history(usr, (BufferedMsg *)usr->held_msgp->p);
			break;

		case 'n':
		case 'N':
		case ' ':
			Put(usr, "Next\n");

			if (usr->held_msgp == NULL) {
				Perror(usr, "Your held messages buffer is gone");
				goto Exit_Held_History;
			}
			usr->held_msgp = usr->held_msgp->next;
			if (usr->held_msgp == NULL)
				goto Exit_Held_History;

			generic_print_history(usr, (BufferedMsg *)usr->held_msgp->p);
			break;

		case 'a':
		case 'A':
			Put(usr, "Again");
		case KEY_CTRL('L'):
			Put(usr, "\n");
			if (usr->held_msgp == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			generic_print_history(usr, (BufferedMsg *)usr->held_msgp->p);
			break;

		case '-':
		case '_':
			Put(usr, "<white>Read last\n");
			CALL(usr, STATE_HELD_MINUS);
			Return;

		case '+':
		case '=':
			Put(usr, "<white>Skip forward\n");
			CALL(usr, STATE_HELD_PLUS);
			Return;

		case '#':
			Put(usr, "<white>Message number\n");
			CALL(usr, STATE_HELD_MSG_NUMBER);
			Return;

		case 'r':
		case 'v':
			Put(usr, "Reply\n");
			goto Held_History_Reply;

		case 'R':
		case KEY_CTRL('R'):
		case 'V':
			Put(usr, "Reply to All\n");

Held_History_Reply:
			if (usr->runtime_flags & RTF_HOLD) {			/* this would normally never happen ... */
				Put(usr, "<magenta>You have put messages on hold\n");
				break;
			}
			if (usr->held_msgp == NULL) {
				Put(usr, "<red>No message to reply to\n");
				goto Exit_Held_History;
			}
			m = (BufferedMsg *)usr->held_msgp->p;

			deinit_StringQueue(usr->recipients);

			if (c == 'R' || c == 'V' || c == 'A' || c == 'a') {
				if (concat_StringQueue(usr->recipients, copy_StringList(m->to)) == NULL) {
					Perror(usr, "Out of memory");
					deinit_StringQueue(usr->recipients);
				}
			}
			if ((sl = in_StringQueue(usr->recipients, m->from)) == NULL) {
				if ((sl = new_StringList(m->from)) == NULL) {
					Perror(usr, "Out of memory");
				} else
					(void)add_StringQueue(usr->recipients, sl);
			}
			check_recipients(usr);
			if (count_Queue(usr->recipients) <= 0)
				break;

			do_reply_x(usr, m->flags);
			Return;

		case 'l':
		case 'L':
			Put(usr, "List recipients\n");

			if (usr->held_msgp == NULL) {
				Perror(usr, "Your held messages buffer is gone");
				goto Exit_Held_History;
			}
			m = (BufferedMsg *)usr->held_msgp->p;
			if (m->to != NULL && m->to->next != NULL)
				Put(usr, "<magenta>Recipients are <yellow>");
			else
				Put(usr, "<magenta>The recipient is <yellow>");

			for(sl = m->to; sl != NULL; sl = sl->next) {
				if (sl->next == NULL)
					Print(usr, "%s\n", sl->str);
				else
					Print(usr, "%s, ", sl->str);
			}
			break;

		case 's':
		case 'S':
		case 'q':
		case 'Q':
		case KEY_ESC:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			if (c == 'q' || c == 'Q')
				Put(usr, "Quit\n");
			else
				Put(usr, "Stop\n");

Exit_Held_History:
			(void)rewind_PList(usr->held_msgs);
			usr->msg_seq_recv += count_List(usr->held_msgs);
			concat_BufferedMsg(&usr->history, usr->held_msgs);
			usr->held_msgs = usr->held_msgp = NULL;

/*
	hold when busy doesn't set RTF_HOLD
	explicit hold with Ctrl-B does set RTF_HOLD
*/
			if (usr->runtime_flags & RTF_HOLD)
				notify_unhold(usr);				/* wake up friends */

			usr->runtime_flags &= ~(RTF_BUSY|RTF_HOLD);
			Free(usr->away);
			usr->away = NULL;

			if (usr->runtime_flags & RTF_WAS_HH) {
				usr->runtime_flags &= ~RTF_WAS_HH;
				usr->flags |= USR_HELPING_HAND;
				add_helper(usr);
			}

			RET(usr);
			Return;

		default:
			if (fun_common(usr, c)) {			/* Note: may reset RTF_BUSY flag(!) */
				Return;
			}
	}
	if (usr->held_msgp == NULL) {
		usr->runtime_flags &= ~RTF_BUSY;
		RET(usr);
		Return;
	}
	usr->runtime_flags |= RTF_BUSY;

	n = 1;
	for(pl = rewind_BufferedMsg(usr->held_msgs); pl != NULL; pl = pl->next) {
		if (pl == usr->held_msgp)
			break;

		n++;
	}
	remaining = 0;
	while(pl != NULL) {
		pl = pl->next;
		remaining++;
	}
	if (remaining > 0)
		remaining--;

	Print(usr, "<yellow>\n[Held Messages]<magenta> msg #%s (%s remaining) %c<white> ",
		print_number(n, num_buf1, sizeof(num_buf1)),
		print_number(remaining, num_buf2, sizeof(num_buf2)),
		(usr->runtime_flags & RTF_SYSOP) ? '#' : '>'
	);
	Return;
}

void state_held_msg_number(User *usr, char c) {
	generic_history_msg_number(usr, c, usr->held_msgs, &usr->held_msgp);
}

void state_held_minus(User *usr, char c) {
	generic_history_minus(usr, c, &usr->held_msgp);
}

void state_held_plus(User *usr, char c) {
	generic_history_plus(usr, c, &usr->held_msgp);
}

/*
	there is a maximum number of remembered messages
	call directly after adding a BufferedMsg to the history

	it is inefficient because it has to count the list every time
	maybe it should be a Queue instead
*/
void expire_history(User *usr) {
	if (usr == NULL)
		return;

	if (usr->history_p != usr->history && count_List(usr->history) > PARAM_MAX_HISTORY) {
		PList *pl;

		if ((pl = pop_PList(&usr->history)) != NULL) {
			destroy_BufferedMsg((BufferedMsg *)pl->p);
			pl->p = NULL;
			destroy_PList(pl);
		}
	}
}

/* EOB */
