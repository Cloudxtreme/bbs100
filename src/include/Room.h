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
	Room.h	WJ99
*/

#ifndef ROOM_H_WJ99
#define ROOM_H_WJ99 1

#include <config.h>

#include "defines.h"
#include "List.h"
#include "StringList.h"
#include "Message.h"
#include "PList.h"

#define add_Room(x,y)			(Room *)add_List((x), (y))
#define prepend_Room(x,y)		(Room *)prepend_List((x), (y))
#define concat_Room(x,y)		(Room *)concat_List((x), (y))
#define remove_Room(x,y)		(Room *)remove_List((x), (y))
#define rewind_Room(x)			(Room *)rewind_List(x)
#define unwind_Room(x)			(Room *)unwind_List(x)
#define sort_Room(x,y)			(Room *)sort_List((x), (y))
#define listdestroy_Room(x)		listdestroy_List((x), destroy_Room)

/*
	fixed room numbers
*/
#define LOBBY_ROOM				0
#define MAIL_ROOM				1
#define HOME_ROOM				2
#define SPECIAL_ROOMS			3

/*
	room flags
*/
#define ROOM_READONLY			1
#define ROOM_SUBJECTS			2
#define ROOM_INVITE_ONLY		4
#define ROOM_ANONYMOUS			8
#define ROOM_HIDDEN				0x10
#define ROOM_NOZAP				0x20
#define ROOM_CHATROOM			0x40
#define ROOM_ALL				0x7f	/* ROOM_READONLY | ROOM_SUBJECTS | ... | ROOM_HOME */
#define ROOM_DIRTY				0x80

#define LOAD_ROOM_DATA			1
#define LOAD_ROOM_AIDES			2
#define LOAD_ROOM_INVITED		4
#define LOAD_ROOM_KICKED		8
#define LOAD_ROOM_INFO			0x10
#define LOAD_ROOM_CHAT_HISTORY	0x20
#define LOAD_ROOM_ALL			0x3f

#ifndef USER_DEFINED
#define USER_DEFINED 1
typedef struct User_tag User;
#endif

typedef struct Room_tag Room;

struct Room_tag {
	List(Room);

	char *name, *category;
	unsigned int number, flags, roominfo_changed;
	unsigned long generation;
	long tail_msg, head_msg, max_msgs;
	StringList *room_aides, *kicked, *invited;
	StringQueue *chat_history;
	StringIO *info;
	PQueue *inside;		/* queue of pointers to online users */
};

extern Room *AllRooms;
extern Room *HomeRooms;
extern Room *Lobby_room;

Room *new_Room(void);
void destroy_Room(Room *);

Room *load_Room(unsigned int, int);
Room *load_Mail(char *, int);
Room *load_Home(char *, int);
Room *load_RoomData(char *, unsigned int, int);
int load_RoomData_version0(File *, Room *, int);
int load_RoomData_version1(File *, Room *, int);
int load_roominfo(Room *, char *);

int save_Room(Room *);
int save_Room_version1(File *, Room *);

void newMsg(Room *, User *);
long newMsgs(Room *, long);
void resize_Room(Room *, long, User *);
char *room_name(User *, Room *, char *, int);
void room_readdir(Room *);
void room_readmaildir(Room *, char *);
void room_readroomdir(Room *, char *, int);
long room_top(Room *);

int init_Room(void);
Room *find_Room(User *, char *);
Room *find_abbrevRoom(User *, char *);
Room *find_Roombynumber(User *, unsigned int);
Room *find_Roombynumber_username(User *, char *, unsigned int);
Room *find_Home(char *);
int room_exists(char *);
int roomnumber_exists(unsigned int);
void unload_Room (Room *);
int room_sort_by_category(void *, void *);
int room_sort_by_number(void *, void *);

#endif	/* ROOM_H_WJ99 */

/* EOB */
