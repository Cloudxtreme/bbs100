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
#include "Lang.h"
#include "Conn.h"
#include "Telnet.h"
#include "KVPair.h"
#include "StringIO.h"
#include "Display.h"
#include "Types.h"

Typedef Types_table[NUM_TYPES+1] = {
	{ "char",				sizeof(char),				},
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
	{ "Lang",				sizeof(Lang),				},
	{ "Conn",				sizeof(Conn),				},
	{ "Telnet",				sizeof(Telnet),				},
	{ "KVPair",				sizeof(KVPair),				},
	{ "StringIO",			sizeof(StringIO),			},
	{ "Display",			sizeof(Display),			},
	{ "(unknown)",			0,							},
};

/* EOB */
