/*
    bbs100 2.0 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	CallStack.c	WJ99
*/

#include "config.h"
#include "CallStack.h"
#include "debug.h"
#include "User.h"
#include "util.h"
#include "state.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

CallStack *new_CallStack(void (*func)(User *, char)) {
CallStack *cs;

	if ((cs = (CallStack *)Malloc(sizeof(CallStack), TYPE_CALLSTACK)) == NULL)
		return NULL;

	cs->ip = func;
	return cs;
}

void destroy_CallStack(CallStack *cs) {
	Free(cs);
}


/*
	Push() : Go to a new level, but do not call yet (so it doesn't print a prompt)
*/
CallStack *Push(User *usr, void (*state)(User *, char)) {
CallStack *cs;

	if (usr == NULL || state == NULL)
		return NULL;

	if ((cs = new_CallStack(state)) == NULL) {
		Perror(usr, "Out of memory");
		return NULL;
	}
	usr->callstack = add_CallStack(&usr->callstack, cs);
	return cs;
}

/*
	Call() : Go to a new level; it is immediately initialized
	         (for printing prompts, initializing vars, etc)
*/
void Call(User *usr, void (*state)(User *, char)) {
	if (usr == NULL || state == NULL || Push(usr, state) == NULL)
		return;

	state(usr, INIT_STATE);
}

/*
	Move() : Replace current level by a new level, don't call init
*/
void Move(User *usr, void (*state)(User *, char)) {
	if (usr == NULL || state == NULL)
		return;

	if (usr->callstack == NULL)
		usr->callstack = new_CallStack(state);
	else
		usr->callstack->ip = state;
}

/*
	Jump() : Replace current level by a new level, and call init
*/
void Jump(User *usr, void (*state)(User *, char)) {
	if (usr == NULL || state == NULL)
		return;

	Move(usr, state);
	if (usr->callstack != NULL && usr->callstack->ip != NULL)
		usr->callstack->ip(usr, INIT_STATE);
}

/*
	Ret() : Pops off the level and calls INIT in the previous level
	        (for reprinting prompts, initializing vars, etc)
*/
void Ret(User *usr) {
	if (usr == NULL)
		return;

	Pop(usr);
	if (usr->callstack != NULL && usr->callstack->ip != NULL)
		usr->callstack->ip(usr, INIT_STATE);
}

/* EOB */
