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
	CallStack.h	WJ99
*/

#ifndef CALLSTACK_H_WJ99
#define CALLSTACK_H_WJ99 1

#include "List.h"

#define push_CallStack(x,y)			(CallStack *)prepend_List((x), (y))
#define pop_CallStack(x)			(CallStack *)pop_List(x)
#define remove_CallStack(x,y)		(CallStack *)remove_List((x), (y))
#define listdestroy_CallStack(x)	listdestroy_List((x), destroy_CallStack)

#define INIT_STATE	0
#define INIT_PROMPT	(char)-1
#define LOOP_STATE	' '

#define CURRENT_STATE(x)			CURRENT_STATE_X((x), INIT_STATE)
#define CURRENT_STATE_X(x,y)		(x)->conn->callstack->ip((x)->conn->data, (char)(y))

#define PUSH(x,y)					Push((x)->conn,(void (*)(void *, char))(y))
#define POP(x)						destroy_CallStack(pop_CallStack(&(x)->conn->callstack))
#define PUSH_ARG(x,y,z)				PushArg((x)->conn, (y), (z))
#define POP_ARG(x,y,z)				PopArg((x)->conn, (y), (z))
#define PEEK_ARG(x,y,z)				PeekArg((x)->conn, (y), (z))
#define POKE_ARG(x,y,z)				PokeArg((x)->conn, (y), (z))
#define CALL(x,y)					Callx((x)->conn,(void (*)(void *, char))(y), (char)INIT_STATE)
#define CALLX(x,y,z)				Callx((x)->conn,(void (*)(void *, char))(y), (char)(z))
#define JMP(x,y)					Jump((x)->conn,(void (*)(void *, char))(y))
#define MOV(x,y)					Move((x)->conn,(void (*)(void *, char))(y))
#define RET(x)						Retx((x)->conn, (char)INIT_STATE)
#define RETX(x,y)					Retx((x)->conn, (char)(y))
#define LOOP(x,y)					loop_Conn((x)->conn, (y))

#ifndef CONN_DEFINED
#define CONN_DEFINED	1
typedef struct Conn_tag Conn;
#endif

typedef struct CallStack_tag CallStack;

struct CallStack_tag {
	List(CallStack);

	void (*ip)(void *, char);
};

CallStack *new_CallStack(void);
void destroy_CallStack(CallStack *);

void Push(Conn *, void (*)(void *, char));
void Callx(Conn *, void (*)(void *, char), char);
void Move(Conn *, void (*)(void *, char));
void Jump(Conn *, void (*)(void *, char));
void Retx(Conn *, char);
void PushArg(Conn *, void *, int);
void PopArg(Conn *, void *, int);
void PeekArg(Conn *, void *, int);
void PokeArg(Conn *, void *, int);

#endif	/* CALLSTACK_H_WJ99 */

/* EOB */
