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
	CallStack.c	WJ99
*/

#include "config.h"
#include "CallStack.h"
#include "debug.h"
#include "Conn.h"
#include "util.h"
#include "state.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

CallStack *new_CallStack(void) {
	return (CallStack *)Malloc(sizeof(CallStack), TYPE_CALLSTACK);
}

void destroy_CallStack(CallStack *cs) {
	Free(cs);
}


/*
	Push() : Go to a new level, but do not call yet (so it doesn't print a prompt)
*/
void Push(Conn *conn, void (*state)(void *, char)) {
CallStack *cs;

	if (conn == NULL || state == NULL)
		return;

	if ((cs = new_CallStack()) == NULL)
		return;

	cs->ip = state;
	conn->callstack = push_CallStack(&conn->callstack, cs);
}

/*
	Call() : Go to a new level; it is immediately initialized
	         (for printing prompts, initializing vars, etc)
*/
void Call(Conn *conn, void (*state)(void *, char)) {
	if (conn == NULL || state == NULL)
		return;

	Push(conn, state);
	state(conn->data, INIT_STATE);
}

/*
	Move() : Replace current level by a new level, don't call init
*/
void Move(Conn *conn, void (*state)(void *, char)) {
	if (conn == NULL || state == NULL)
		return;

	if (conn->callstack == NULL)
		conn->callstack = new_CallStack();

	if (conn->callstack != NULL)
		conn->callstack->ip = state;
}

/*
	Jump() : Replace current level by a new level, and call init
*/
void Jump(Conn *conn, void (*state)(void *, char)) {
	if (conn == NULL || state == NULL)
		return;

	Move(conn, state);
	if (conn->callstack != NULL && conn->callstack->ip != NULL)
		conn->callstack->ip(conn->data, INIT_STATE);
}

/*
	Ret() : Pops off the level and calls INIT in the previous level
	        (for reprinting prompts, initializing vars, etc)
*/
void Ret(Conn *conn) {
	if (conn == NULL)
		return;

	destroy_CallStack(pop_CallStack(&conn->callstack));

	if (conn->callstack != NULL && conn->callstack->ip != NULL)
		conn->callstack->ip(conn->data, INIT_STATE);
}

/* EOB */
