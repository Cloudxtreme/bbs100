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
	Types.c	WJ100

	Memory types table
*/

#include "config.h"
#include "Types.h"
#include "StringList.h"
#include "PList.h"
#include "CallStack.h"
#include "SymbolTable.h"
#include "Timer.h"
#include "SignalVector.h"
#include "CachedFile.h"
#include "AtomicFile.h"
#include "Wrapper.h"
#include "User.h"
#include "Room.h"
#include "Joined.h"
#include "Message.h"
#include "BufferedMsg.h"
#include "Timezone.h"
#include "Hash.h"
#include "Conn.h"
#include "Telnet.h"
#include "KVPair.h"
#include "StringIO.h"
#include "Display.h"
#include "Queue.h"
#include "NewUserLog.h"
#include "DirList.h"
#include "Types.h"

Typedef Types_table[NUM_TYPES] = {
	{ "char",				SIZE_CHAR,					},
	{ "int",				sizeof(int),				},
	{ "long",				sizeof(long),				},
	{ "<pointer>",			sizeof(void *),				},
	{ "StringList",			sizeof(StringList),			},
	{ "PList",				sizeof(PList),				},
	{ "CallStack",			sizeof(CallStack),			},
	{ "SymbolTable",		sizeof(SymbolTable),		},
	{ "Timer",				sizeof(Timer),				},
	{ "SignalVector",		sizeof(SignalVector),		},
	{ "User",				sizeof(User),				},
	{ "Room",				sizeof(Room),				},
	{ "Joined",				sizeof(Joined),				},
	{ "Message",			sizeof(Message),			},
	{ "BufferedMsg",		sizeof(BufferedMsg),		},
	{ "File",				sizeof(File),				},
	{ "Wrapper",			sizeof(Wrapper),			},
	{ "CachedFile",			sizeof(CachedFile),			},
	{ "AtomicFile",			sizeof(AtomicFile),			},
	{ "Timezone",			sizeof(Timezone),			},
	{ "DST_Trans",			sizeof(DST_Transition),		},
	{ "TimeType",			sizeof(TimeType),			},
	{ "Hash",				sizeof(Hash),				},
	{ "Conn",				sizeof(Conn),				},
	{ "Telnet",				sizeof(Telnet),				},
	{ "KVPair",				sizeof(KVPair),				},
	{ "StringIO",			sizeof(StringIO),			},
	{ "Display",			sizeof(Display),			},
	{ "Queue",				sizeof(QueueType),			},
	{ "MailTo",				sizeof(MailTo),				},
	{ "DirList",			sizeof(DirList),			},
	{ "NewUserLog",			sizeof(NewUserLog),			},
};

/* EOB */
