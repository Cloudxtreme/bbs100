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
	Conn.h	WJ105
*/

#ifndef CONN_H_WJ105
#define CONN_H_WJ105	1

#include "defines.h"
#include "CallStack.h"
#include "StringIO.h"
#include "List.h"

#define add_Conn(x,y)		(Conn *)add_List((x), (y))
#define concat_Conn(x,y)	(Conn *)concat_List((x), (y))
#define remove_Conn(x,y)	(Conn *)remove_List((x), (y))
#define listdestroy_Conn(x)	listdestroy_List((x), destroy_Conn)
#define rewind_Conn(x)		(Conn *)rewind_List(x)
#define unwind_Conn(x)		(Conn *)unwind_List(x)
#define sort_Conn(x, y)		(Conn *)sort_List((x), (y))

#define CONN_LISTEN			1
#define CONN_CONNECTING		2
#define CONN_ESTABLISHED	3
#define CONN_LOOPING		4
#define CONN_WAIT_CLOSE		5
#define CONN_WAIT_CLOSE2	6
#define CONN_CLOSED			7

#define MIN_INPUTBUF		16
#define MIN_OUTPUTBUF		STRINGIO_MINSIZE

#ifndef CONN_DEFINED
#define CONN_DEFINED 1
typedef struct Conn_tag Conn;
#endif


/*
	ConnType really defines the 'class' of the connection
	and these are the member functions
*/
typedef struct {
	void (*process)(Conn *, char);
	void (*accept)(Conn *);
	void (*complete_connect)(Conn *);
	void (*wait_close)(Conn *);
	void (*close)(Conn *);
	void (*linkdead)(Conn *);
	void (*destroy)(Conn *);
} ConnType;

struct Conn_tag {
	List(Conn);

	ConnType *conn_type;

	int sock, state;			/* state is one of CONN_xxx */
	unsigned long loop_counter;

	char *ipnum, *hostname;
	StringIO *input, *output;	/* I/O buffers */

	void *data;					/* connection specific data */
	CallStack *callstack;
};

extern Conn *AllConns;

Conn *new_Conn(void);
void destroy_Conn(Conn *);

void dummy_Conn_handler(Conn *);
void dummy_Conn_process(Conn *, char);

int write_Conn(Conn *, char *, int);
int putc_Conn(Conn *, char);
int put_Conn(Conn *, char *);
int flush_Conn(Conn *);
int input_Conn(Conn *);

void close_Conn(Conn *);
void loop_Conn(Conn *, unsigned long);

void shut_allconns(void);

#endif	/* CONN_H_WJ105 */

/* EOB */
