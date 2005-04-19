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
	Conn.h	WJ105
*/

#ifndef CONN_H_WJ105
#define CONN_H_WJ105	1

#include "defines.h"
#include "CallStack.h"
#include "List.h"

#define add_Conn(x,y)		(Conn *)add_List((x), (y))
#define concat_Conn(x,y)	(Conn *)concat_List((x), (y))
#define remove_Conn(x,y)	remove_List((x), (y))
#define listdestroy_Conn(x)	listdestroy_List((x), destroy_List)
#define rewind_Conn(x)		(Conn *)rewind_List(x)
#define unwind_Conn(x)		(Conn *)unwind_List(x)
#define sort_Conn(x, y)		(Conn *)sort_List((x), (y))

#define CONN_LISTEN			1
#define CONN_CONNECTING		2
#define CONN_ESTABLISHED	4
#define CONN_SELECT			8
#define CONN_LOOPING		0x10


#ifndef CONN_DEFINED
#define CONN_DEFINED 1
typedef struct Conn_tag Conn;
#endif


/*
	ConnType really defines the 'class' of the connection
	and these are the member functions
*/
typedef struct {
	void (*readable)(Conn *);
	void (*writable)(Conn *);
	void (*process)(Conn *, char);
	void (*accept)(Conn *);
	void (*complete_connect)(Conn *);
	void (*close)(Conn *);
	void (*linkdead)(Conn *);
	void (*destroy)(Conn *);
} ConnType;

struct Conn_tag {
	List(Conn);

	ConnType *conn_type;

	int sock, state, input_head, input_tail, output_idx;
	unsigned long loop_counter;

	char inputbuf[MAX_INPUTBUF], outputbuf[MAX_OUTPUTBUF];
	char ipnum[MAX_LINE], hostname[MAX_LINE];

	void *data;					/* connection specific data */
	CallStack *callstack;
};

extern Conn *AllConns;

Conn *new_Conn(void);
void destroy_Conn(Conn *);

void dummy_Conn_handler(Conn *);
void dummy_Conn_process(Conn *, char);

void write_Conn(Conn *, char *);
void putc_Conn(Conn *, char);
void flush_Conn(Conn *);

#endif	/* CONN_H_WJ105 */

/* EOB */
