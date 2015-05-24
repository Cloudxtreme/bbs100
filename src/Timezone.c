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
	Timezone.c	WJ103
*/

#include "config.h"
#include "defines.h"
#include "Timezone.h"
#include "cstring.h"
#include "Hash.h"
#include "AtomicFile.h"
#include "Param.h"
#include "Memory.h"
#include "util.h"
#include "log.h"
#include "debug.h"
#include "mydirentry.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


Hash *tz_hash = NULL;


int init_Timezone(void) {
	if (tz_hash == NULL	&& (tz_hash = new_Hash()) == NULL)
		return -1;

	tz_hash->hashaddr = hashaddr_ascii;
	return 0;
}

Timezone *new_Timezone(void) {
Timezone *tz;

	if ((tz = (Timezone *)Malloc(sizeof(Timezone), TYPE_TIMEZONE)) == NULL)
		return NULL;

	return tz;
}

void destroy_Timezone(Timezone *tz) {
	if (tz != NULL) {
		if (tz->refcount != 0)
			log_err("destroy_Timezone(): refcount != 0 (%d)", tz->refcount);

		Free(tz->transitions);
		Free(tz->tznames);
		Free(tz);
	}
}

DST_Transition *new_DST_Transition(void) {
DST_Transition *dst;

	if ((dst = (DST_Transition *)Malloc(sizeof(DST_Transition), TYPE_DST_TRANS)) == NULL)
		return NULL;

	return dst;
}

void destroy_DST_Transition(DST_Transition *dst) {
	Free(dst);
}

static int cmp_dst_transition(const void *a, const void *b) {
const DST_Transition *t1, *t2;

	t1 = (const DST_Transition *)a;
	t2 = (const DST_Transition *)b;
	
	if ((unsigned long)t1->when < (unsigned long)t2->when) {
		return -1;
	}
	if ((unsigned long)t1->when > (unsigned long)t2->when) {
		return 1;
	}
	return 0;
}

/*
	load_Timezone() is used when someone logs in and wants to use a timezone
	therefore it increments a reference counter, which denotes how many users
	are using this timezone structure
	load_Timezone() assumes someone is actually using it; the caller must free
	this claimed resource by calling unload_Timezone() when done (on destroy_User())
*/
Timezone *load_Timezone(char *name) {
char filename[MAX_PATHLEN], buf[32];
AtomicFile *f;
Timezone *tz;
int tzh_ttisgmtcnt, tzh_ttisstdcnt, tzh_leapcnt, tzh_timecnt, tzh_typecnt, tzh_charcnt;
int i;

	if ((tz = in_Hash(tz_hash, name)) != NULL) {		/* already loaded */
		tz->refcount++;									/* somebody is using it (!) */
		return tz;
	}
	bufprintf(filename, sizeof(filename), "%s/%s", PARAM_ZONEINFODIR, name);
	path_strip(filename);

	if ((f = openfile(filename, "r")) == NULL) {
		log_warn("load_Timezone(): failed to open file %s", filename);
		return NULL;
	}
	if ((tz = new_Timezone()) == NULL) {
		log_err("load_Timezone(): failed to allocate new Timezone structure");
		closefile(f);
		return NULL;
	}
/*
	see man tzfile(5) for the format of zoneinfo files
*/
	if (fread(buf, 1, 4, f->f) < 4) {
		log_err("load_Timezone(): premature end-of-file loading %s", filename);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	buf[4] = 0;
	if (strcmp(buf, "TZif")) {
		log_err("load_Timezone(): 'TZif' file identifier not found in %s", filename);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	if (fread(buf, 1, 16, f->f) < 16) {		/* skip 16 bytes; reserved for future use */
		log_err("load_Timezone(): premature end-of-file loading %s", filename);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	tzh_ttisgmtcnt = (int)fread_int32(f->f);
	tzh_ttisstdcnt = (int)fread_int32(f->f);
	tzh_leapcnt = (int)fread_int32(f->f);
	tzh_timecnt = (int)fread_int32(f->f);
	tzh_typecnt = (int)fread_int32(f->f);
	tzh_charcnt = (int)fread_int32(f->f);

	if (tzh_ttisgmtcnt < 0 || tzh_ttisstdcnt < 0 || tzh_leapcnt < 0
		|| tzh_timecnt < 0 || tzh_typecnt < 0 || tzh_charcnt < 0) {
		log_err("load_Timezone(): premature end-of-file loading %s", filename);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
/*
	it is valid to have zero DST transitions
	if so, types[0] should be used
*/
	if (tzh_timecnt > 0 && (tz->transitions = (DST_Transition *)Malloc(tzh_timecnt * sizeof(DST_Transition), TYPE_DST_TRANS)) == NULL) {
		log_err("load_Timezone(): out of memory allocating %d DST_Transitions", tzh_timecnt);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	tz->num_trans = tzh_timecnt;

	for(i = 0; i < tzh_timecnt; i++) {
		tz->transitions[i].when = (time_t)fread_int32(f->f);

		if (tz->transitions[i].when == (time_t)-1) {
			log_err("load_Timezone(): premature end-of-file loading %s", filename);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}
	}
	for(i = 0; i < tzh_timecnt; i++) {
		if ((tz->transitions[i].type_idx = fgetc(f->f)) == -1) {
			log_err("load_Timezone(): premature end-of-file loading %s", filename);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}
	}
	if (tzh_typecnt == 0) {
		log_err("load_Timezone(): tzh_typecnt == 0 in %s", filename);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	if ((tz->types = (TimeType *)Malloc(tzh_typecnt * sizeof(TimeType), TYPE_TIMETYPE)) == NULL) {
		log_err("load_Timezone(): out of memory allocating %d TimeTypes", tzh_typecnt);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	tz->num_types = tzh_typecnt;

	for(i = 0; i < tz->num_trans; i++) {
		if (tz->transitions[i].type_idx < 0 || tz->transitions[i].type_idx >= tz->num_types) {
			log_warn("load_Timezone(%s): error in type_idx, doing fix-up", name);
			tz->transitions[i].type_idx = 0;
		}
	}
	for(i = 0; i < tzh_typecnt; i++) {
		tz->types[i].gmtoff = (int)fread_int32(f->f);
		tz->types[i].isdst = fgetc(f->f);
		if ((tz->types[i].tzname_idx = fgetc(f->f)) == -1) {
			log_err("load_Timezone(): premature end-of-file loading %s", filename);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}
		if (tz->types[i].tzname_idx > tzh_charcnt) {
			log_err("load_Timezone(): illegal tzname_idx in %s", filename);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}
	}
	if ((tz->tznames = (char *)Malloc(tzh_charcnt+1, TYPE_CHAR)) == NULL) {
		log_err("load_Timezone(): out of memory allocating %d bytes", tzh_charcnt);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	if (fread(tz->tznames, 1, tzh_charcnt, f->f) < tzh_charcnt) {
		log_err("load_Timezone(): premature end-of-file loading %s", filename);
		closefile(f);
		destroy_Timezone(tz);
		return NULL;
	}
	tz->tznames[tzh_charcnt] = 0;

	if (tzh_leapcnt) {
/*
	crazy world ... we have leap seconds! Because the rotation of the Earth is
	not constantly the same, the astronomical time is slightly different from the
	(used) UTC time, which is based on atomic clocks.

	There is 'standard time' and 'wallclock time'. Standard time includes leap seconds
	and wallclock doesn't (ever seen a clock that can display 61 seconds?)
	So, there is a difference between a DST transition given in standard time and a
	DST transition given in wallclock time. It is stupid that there are two possible
	formats in a zoneinfo definition. What I do is load the leap definitions,
	and do a fixup for all DST transitions that were specified as wallclock time.

	This code is really untested, because I haven't had the pleasure of finding
	one single zoneinfo file that actually specifies leap seconds. Leap seconds
	do occur almost every year, but the zoneinfo files are generally in
	standard format -- and they should be, because leap seconds are not really
	a timezone issue, but a global issue

	Due to some confusion raised over the exact meaning of the standard/wall
	indicator (read below, at the loading of the UTC/local indicators) I'm skipping
	support for leap seconds.
*/
		log_warn("load_Timezone(): leap seconds are not supported in %s", filename);
	}
/*
		LeapSecond *leaps;
		int standard;

		log_warn("load_Timezone(): running untested code: doing leap seconds fixup for %s", filename);

		if ((leaps = (LeapSecond *)Malloc(tzh_leapcnt * sizeof(LeapSecond), TYPE_CHAR)) == NULL) {
			log_err("load_Timezone(): out of memory allocating %d LeapSeconds", tzh_leapcnt);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}
		for(i = 0; i < tzh_leapcnt; i++) {
			leaps[i].when = (time_t)fread_int32(f->f);
			leaps[i].num_secs = (int)fread_int32(f->f);
		}
		if (tzh_ttisstdcnt != tzh_typecnt)
			log_warn("load_Timezone(): weird, tzh_ttisstdcnt (%d) != tzh_typecnt (%d) in %s", tzh_ttisstdcnt, tzh_typecnt, filename);

		for(i = 0; i < tzh_ttisstdcnt; i++) {
			if ((standard = fgetc(f->f)) == -1) {
				log_err("load_Timezone(): premature end-of-file loading %s", filename);
				Free(leaps);
				closefile(f);
				destroy_Timezone(tz);
				return NULL;
			}
			if (!standard) {
				int a;

	all this does is add all previously occurred leap seconds to the timestamps
	in the transition list

				for(a = 0; a < tz->num_trans; a++) {
					if (tz->transitions[a].type_idx == i) {
						int b, total_leaps;

						b = total_leaps = 0;
						while(b < tzh_leapcnt && tz->transitions[a].when < leaps[b].when) {
							total_leaps += leaps[b].num_secs;
							b++;
						}
						tz->transitions[a].when = (time_t)((unsigned long)tz->transitions[a].when + (unsigned long)total_leaps);
					}
				}
			}
		}
		Free(leaps);
	} else {
*/
						/* skip the leap second stuff */

		for(i = 0; i < tzh_leapcnt; i++) {
			if (fread_int32(f->f) == -1L || fread_int32(f->f) == -1L) {
				log_err("load_Timezone(): premature end-of-file loading %s", filename);
				closefile(f);
				destroy_Timezone(tz);
				return NULL;
			}
		}
		if (tzh_ttisstdcnt != tzh_typecnt)
			log_warn("load_Timezone(): weird, tzh_ttisstdcnt (%d) != tzh_typecnt (%d) in %s", tzh_ttisstdcnt, tzh_typecnt, filename);

		for(i = 0; i < tzh_ttisstdcnt; i++) {
			if (fgetc(f->f) == -1) {
				log_err("load_Timezone(): premature end-of-file loading %s", filename);
				closefile(f);
				destroy_Timezone(tz);
				return NULL;
			}
		}

/*	}	*/

/*
	By the way, one more thing ...
	the DST transitions can be specified as UTC or as local time (for this zone)
	so I wanted to do a fixup here so I can use UTC all the way

	In reality, I found that all zoneinfo files specify the DST transitions in UTC,
	and that the UTC/local indicators are 0 for the US and 1 for Europe. In other words,
	the UTC/local indicators make no sense (to me). So I just skip this bit for now
	and hope all goes well.

	(This raises some confusion over the meaning of the standard/wall indicators as well)
*/
	if (tzh_ttisgmtcnt != tzh_typecnt)
		log_warn("load_Timezone(): weird, tzh_ttisgmtcnt (%d) != tzh_typecnt (%d) in %s", tzh_ttisgmtcnt, tzh_typecnt, filename);

/*
	for(i = 0; i < tzh_ttisgmtcnt; i++) {
		if ((local = fgetc(f->f)) == -1) {
			log_err("load_Timezone(): premature end-of-file loading %s", filename);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}

	don't do any fix-ups here (see above)

		if (local) {
			int k;

			for(k = 0; k < tz->num_trans; k++) {
				if (tz->transitions[k].type_idx == i) {
					log_debug("load_Timezone(): UTC/local fixup: trans %d -= %d (type %d)", k, tz->types[i].gmtoff, i);
					tz->transitions[k].when -= tz->types[i].gmtoff;
				}
			}
		}
	}
*/
	closefile(f);

/*
	determine pointer to 'current' transition
*/
	tz->curr_idx = 0;
	if (tz->num_trans > 1) {
		int n;
		unsigned long in_two_years;
		DST_Transition *trans;

		qsort(tz->transitions, tz->num_trans, sizeof(DST_Transition), cmp_dst_transition);

		while((tz->curr_idx+1) < tz->num_trans && tz->transitions[tz->curr_idx+1].when <= rtc)
			tz->curr_idx++;

/*
	at this moment, all DST transitions are resident in memory
	the zoneinfo files have a habit of storing every single transition
	since the last ice age, so I'm going to free up some memory
	I'm only interested in the transitions for the next 2 years to come
	(anyone running bbs100 with an uptime of more than 2 years will
	have the chance of seeing the clock fail ;)

	(I guess you could do the same thing for the TimeTypes, but there
	are so few that it is not really a problem)
*/
		n = tz->curr_idx;
		in_two_years = rtc + 2 * 365 * SECS_IN_DAY;
		while(n < tz->num_trans && tz->transitions[n].when <= in_two_years)
			n++;

		n = n - tz->curr_idx + 1;
		if ((trans = (DST_Transition *)Malloc(n * sizeof(DST_Transition), TYPE_DST_TRANS)) != NULL) {
			memcpy(trans, &tz->transitions[tz->curr_idx], n * sizeof(DST_Transition));
			Free(tz->transitions);
			tz->transitions = trans;
			tz->num_trans = n;
			tz->curr_idx = 0;
		}
	}											/* else we have only 1 entry */
	if (add_Hash(tz_hash, name, tz, (void (*)(void *))destroy_Timezone) == -1) {
		log_err("load_Timezone(): failed to add new Timezone to tz_hash");
		destroy_Timezone(tz);
		return NULL;
	}
	tz->refcount = 1;
	return tz;
}

void unload_Timezone(char *name) {
Timezone *tz;

	if ((tz = (Timezone *)in_Hash(tz_hash, name)) != NULL) {
		tz->refcount--;
		if (tz->refcount <= 0) {
			remove_Hash(tz_hash, name);
			destroy_Timezone(tz);
/*			log_info("unload_Timezone(): timezone %s unloaded", name);	*/
		}
	}
}

/*
	return the name of the timezone
*/
char *name_Timezone(Timezone *tz) {
char *gmt = "GMT";
int tz_type;

	if (tz == NULL)
		return gmt;

	if (tz->transitions != NULL)
		tz_type = tz->transitions[tz->curr_idx].type_idx;
	else
		tz_type = 0;

	if (tz_type < 0 || tz_type > tz->num_types)
		tz_type = 0;

	if (tz->types != NULL && tz->num_types > 0)
		return tz->tznames + tz->types[tz_type].tzname_idx;
	
	return gmt;		/* all wrong, but we have nothing better ... */
}


/*

void dump_Timezone(Timezone *tz) {
int i;

	if (tz == NULL) {
		log_debug("dump_Timezone(): tz == NULL");
		return;
	}
	log_debug("dump_Timezone(): tz == {");
	log_debug("  refcount = %d", tz->refcount);
	log_debug("  curr_idx = %d", tz->curr_idx);
	log_debug("  num_trans = %d", tz->num_trans);
	log_debug("  num_types = %d", tz->num_types);

	for(i = 0; i < tz->num_trans; i++) {
		log_debug("  DST_Transition %d {", i);
		log_debug("    when = %s", print_date(NULL, tz->transitions[i].when));
		log_debug("    type_idx = %d", tz->transitions[i].type_idx);
		log_debug("  }");
	}
	for(i = 0; i < tz->num_types; i++) {
		log_debug("  TimeType %d {", i);
		log_debug("    gmtoff = %d", tz->types[i].gmtoff);
		log_debug("    isdst = %d", tz->types[i].isdst);
		log_debug("    tzname_idx = %d [%s]", tz->types[i].tzname_idx, tz->tznames+tz->types[i].tzname_idx);
		log_debug("  }");
	}
	log_debug("}");
}

*/

/* EOB */
