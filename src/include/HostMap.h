/*
	hostmap.h	WJ100
*/

#ifndef HOSTMAP_H_WJ100
#define HOSTMAP_H_WJ100	1

#include "List.h"

#define add_HostMap(x,y)		(HostMap *)add_List((x), (y))
#define concat_HostMap(x,y)		(HostMap *)concat_List((x), (y))
#define remove_HostMap(x,y)		(HostMap *)remove_List((x), (y))
#define listdestroy_HostMap(x)	listdestroy_List((x), destroy_HostMap)
#define rewind_HostMap(x)		(HostMap *)rewind_List(x)
#define unwind_HostMap(x)		(HostMap *)unwind_List(x)
#define sort_HostMap(x,y)		(HostMap *)sort_List((x), (y))

typedef struct HostMap_tag	HostMap;

struct HostMap_tag {
	List(HostMap);

	char *site, *desc;
};

extern HostMap *hostmap;

HostMap *new_HostMap(char *, char *);
void destroy_HostMap(HostMap *);
int load_HostMap(char *);
int hostmap_sort_func(void *, void *);
char *HostMap_desc(char *);

#endif	/* HOSTMAP_H_WJ100 */

/* EOB */
