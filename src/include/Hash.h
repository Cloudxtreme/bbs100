/*
	Hash.h	WJ103
*/

#ifndef HASH_H_WJ103
#define HASH_H_WJ103	1

#include "List.h"

#define add_HashList(x,y)		(HashList *)add_List((x), (y))
#define concat_HashList(x,y)	(HashList *)concat_List((x), (y))
#define remove_HashList(x,y)	remove_List((x), (y))
#define listdestroy_HashList(x)	listdestroy_List((x), destroy_HashList)
#define rewind_HashList(x)		(HashList *)rewind_List(x)
#define unwind_HashList(x)		(HashList *)unwind_List(x)

#define HASH_MIN_SIZE	32
#define HASH_GROW_SIZE	32

#define MAX_HASH_KEY	32

typedef struct HashList_tag HashList;

struct HashList_tag {
	List(HashList);

	char key[MAX_HASH_KEY];
	void *value;
};

typedef struct {
	int size, num;

	int (*hashaddr)(char *);
	HashList **hash;
} Hash;

Hash *new_Hash(void);
void destroy_Hash(Hash *);

int resize_Hash(Hash *, int);

int add_Hash(Hash *, char *, void *);
void *remove_Hash(Hash *, char *);
void *in_Hash(Hash *, char *);

#endif	/* HASH_H_WJ103 */

/* EOB */
