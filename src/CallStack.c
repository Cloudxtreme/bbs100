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
	CallStack.c	WJ99

	The CallStack acts much like a CPU's stack; it points to a function where
	the user is currently at
	You can return to the previous function where the user has been
	It is also possible to pass arguments or temporary variables between
	states by pushing the arguments onto the CallStack
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

/*
	NOTE: The CallStack is not a very complex type, but mind that there
	can be CallStack argument objects on the list as well ... so listdestroy_CallStack()
	must be able to correctly destroy all types on the list
	(This is important to know when changing this code)
*/
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
	(void)push_CallStack(&conn->callstack, cs);
}

/*
	Call directly into a specific substate of state
*/
void Callx(Conn *conn, void (*state)(void *, char), char arg) {
	if (conn == NULL || state == NULL)
		return;

	Push(conn, state);
	state(conn->data, arg);
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
	Pops off the level and calls cmd in the previous level
	(for reprinting prompts, initializing vars, etc)

	cmd is usually INIT_STATE, but Retx() can be used to immediately
	activate other functions too
*/
void Retx(Conn *conn, char cmd) {
	if (conn == NULL)
		return;

	destroy_CallStack(pop_CallStack(&conn->callstack));

	if (conn->callstack != NULL && conn->callstack->ip != NULL)
		conn->callstack->ip(conn->data, cmd);
}

/*
	arguments are allocated, copied, and pushed onto the CallStack
	The reason that this does not give problems with listdestroy_CallStack()
	is that destroy_CallStack() is also only a Free()
*/
void PushArg(Conn *conn, void *arg, int size) {
char *cs;

	if (conn == NULL || arg == NULL || (cs = (char *)Malloc(sizeof(ListType)+size, TYPE_CHAR)) == NULL)
		return;

	memcpy(cs + sizeof(ListType), arg, size);
	(void)push_CallStack(&conn->callstack, (CallStack *)cs);
}

/*
	assumes that there is also a return address on the stack
	therefore it takes the next element from the stack
*/
void PopArg(Conn *conn, void *arg, int size) {
CallStack *cs;

	if (conn == NULL || arg == NULL || conn->callstack == NULL || conn->callstack->next == NULL)
		return;

	if ((cs = remove_CallStack(&conn->callstack, conn->callstack->next)) == NULL)
		return;

	memcpy(arg, (char *)cs + sizeof(ListType), size);
	Free(cs);
}

/*
	read the argument, but do not remove it from the stack

	assumes that there is also a return address on the stack
	therefore it takes the next element from the stack
*/
void PeekArg(Conn *conn, void *arg, int size) {
CallStack *cs;

	if (conn == NULL || arg == NULL || conn->callstack == NULL || conn->callstack->next == NULL)
		return;

	cs = conn->callstack->next;
	if (cs == NULL) {
		memset(arg, 0, size);
		return;
	}
	memcpy(arg, (char *)cs + sizeof(ListType), size);
}

/*
	overwrite stack argument with a new one

	assumes that there is also a return address on the stack
	therefore it takes the next element from the stack
*/
void PokeArg(Conn *conn, void *arg, int size) {
CallStack *cs_ret, *cs_arg;
char *new_cs;

	if (conn == NULL || arg == NULL || conn->callstack == NULL || conn->callstack->next == NULL)
		return;

	if ((new_cs = (char *)Malloc(sizeof(ListType)+size, TYPE_CHAR)) == NULL)
		return;

	memcpy(new_cs + sizeof(ListType), arg, size);

/* use the conventional (but slow) way */
	cs_ret = pop_CallStack(&conn->callstack);
	cs_arg = pop_CallStack(&conn->callstack);
	Free(cs_arg);
	(void)push_CallStack(&conn->callstack, (CallStack *)new_cs);
	(void)push_CallStack(&conn->callstack, cs_ret);
}

/* EOB */
