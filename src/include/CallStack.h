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
	CallStack.h	WJ99
*/

#ifndef CALLSTACK_H_WJ99
#define CALLSTACK_H_WJ99 1

#include "List.h"

#define add_CallStack(x,y)			add_List((x), (y))
#define concat_CallStack(x,y)		concat_List((x), (y))
#define remove_CallStack(x,y)		remove_List((x), (y))
#define pop_CallStack(x)			pop_List((x), destroy_CallStack)
#define rewind_CallStack(x)			(CallStack *)rewind_List((x))
#define unwind_CallStack(x)			(CallStack *)unwind_List((x))
#define listdestroy_CallStack(x)	listdestroy_List((x), destroy_CallStack)

#define Pop(x)						pop_CallStack(&(x)->callstack)

#define PUSH(x,y)					Push((x)->conn,(void (*)(void *, char))(y))
#define POP(x)						Pop((x)->conn)
#define CALL(x,y)					Call((x)->conn,(void (*)(void *, char))(y))
#define JMP(x,y)					Jump((x)->conn,(void (*)(void *, char))(y))
#define MOV(x,y)					Move((x)->conn,(void (*)(void *, char))(y))
#define RET(x)						Ret((x)->conn)

#define INIT_STATE	0
#define LOOP_STATE	' '

#define CURRENT_STATE(x)			(x)->conn->callstack->ip((x)->conn->data, INIT_STATE)
#define CURRENT_INPUT(x,y)			(x)->conn->callstack->ip((x)->conn->data, (y))


#ifndef CONN_DEFINED
#define CONN_DEFINED 1
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
void Call(Conn *, void (*)(void *, char));
void Move(Conn *, void (*)(void *, char));
void Jump(Conn *, void (*)(void *, char));
void Ret(Conn *);

#endif	/* CALLSTACK_H_WJ99 */

/* EOB */
