/*
    bbs100 1.2.2 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	Timezone.c	WJ103
*/

#include "config.h"
#include "defines.h"
#include "Timezone.h"
#include "Hash.h"
#include "AtomicFile.h"
#include "Param.h"
#include "Memory.h"
#include "util.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>


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
int i, local;

	if ((tz = in_Hash(tz_hash, name)) != NULL) {		/* already loaded */
		tz->refcount++;									/* somebody is using it (!) */
		return tz;
	}
	sprintf(filename, "%s/%s", PARAM_ZONEINFODIR, name);
	path_strip(filename);

	if ((f = openfile(filename, "r")) == NULL) {
		log_err("load_Timezone(): failed to open file %s", filename);
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
		log_err("load_Timezone(): file identifier not found in %s", filename);
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
	if ((tz->transitions = (DST_Transition *)Malloc(tzh_timecnt * sizeof(DST_Transition), TYPE_DST_TRANS)) == NULL) {
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

	for(i = 0; i < tzh_typecnt; i++) {
		tz->types[i].gmtoff = (int)fread_int32(f->f);
		tz->types[i].isdst = fgetc(f->f);
		if ((tz->types[i].tzname_idx = fgetc(f->f)) == -1) {
			log_err("load_Timezone(): premature end-of-file loading %s", filename);
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

/*
	crazy world ... we have leap seconds! Because of the rotation of the Earth
	and such weird stuff. I just want to know what time it is.
	There is 'standard time' (POSIX time) and 'wallclock time'. POSIX time lacks
	leap seconds. So, there is a difference between a DST transition given in
	POSIX time and a DST transition given in wallclock time. This does not make
	me happy. What you can do is load the leap definitions, and do a fixup for
	all DST transitions that were specified as POSIX time.
	I checked the zoneinfo files, but none of them have any leap second entries
	in them. So, I'm being extremely lazy here and am secretly skipping support
	for leap seconds in zoneinfo files for now. By the way, do not really have
	anything to do with timezones anyway. They should occur all over the world.

	The actual code should look something like this:

		typedef struct {
			time_t when;
			int num_secs;
		} LeapSecond;

		LeapSecond *leaps;

		if ((tz->leaps = (LeapSecond *)Malloc(tzh_leapcnt * sizeof(LeapSecond), TYPE_CHAR)) == NULL) {
			log_err("load_Timezone(): out of memory allocating %d LeapSeconds", tzh_leapcnt);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}
		tz->num_leaps = tzh_leapcnt;

		for(i = 0; i < tzh_leapcnt; i++) {
			leaps[i].when = (time_t)fread_int32(f->f);
			leaps[i].num_secs = (int)fread_int32(f->f);
		}
		if (tzh_ttisstdcnt != tzh_typecnt)
			log_warn("load_Timezone(): weird, tzh_ttisstdcnt (%d) != tzh_typecnt (%d) in %s", tzh_ttisstdcnt, tzh_typecnt, filename);

		for(i = 0; i < tzh_ttisstdcnt; i++) {
			if ((wall = fgetc(f->f)) == -1) {
				log_err("load_Timezone(): premature end-of-file loading %s", filename);
				Free(leaps);
				closefile(f);
				destroy_Timezone(tz);
				return NULL;
			}
			if (!wall) {

	yeah and what now ... (sigh)

			}
		}
		Free(leaps);
*/

/* skip the leap second stuff */

	if (tzh_leapcnt)
		log_warn("load_Timezone(): leap seconds in file %s are being ignored", filename);

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
/*
	By the way, one more thing ...
	the DST transitions can be specified as UTC or as local time (for this zone)
	so I do a fixup here so I can use UTC all the way
*/
	if (tzh_ttisgmtcnt != tzh_typecnt)
		log_warn("load_Timezone(): weird, tzh_ttisgmtcnt (%d) != tzh_typecnt (%d) in %s", tzh_ttisgmtcnt, tzh_typecnt, filename);

	for(i = 0; i < tzh_ttisgmtcnt; i++) {
		if ((local = fgetc(f->f)) == -1) {
			log_err("load_Timezone(): premature end-of-file loading %s", filename);
			closefile(f);
			destroy_Timezone(tz);
			return NULL;
		}
		if (local) {				/* do fixup; convert 'when' to UTC */
			int k;

			for(k = 0; k < tz->num_trans; k++) {
				if (tz->transitions[k].type_idx == i)
					tz->transitions[k].when -= tz->types[i].gmtoff;
			}
		}
	}
	closefile(f);

	if (add_Hash(tz_hash, name, tz) == -1) {
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
		}
	}
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
	log_debug("  next_idx = %d", tz->next_idx);
	log_debug("  num_trans = %d", tz->num_trans);
	log_debug("  num_types = %d", tz->num_types);

	for(i = 0; i < tz->num_trans; i++) {
		log_debug("  DST_Transition {");
		log_debug("    when = %s", print_date(NULL, tz->transitions[i].when));
		log_debug("    type_idx = %d", tz->transitions[i].type_idx);
		log_debug("  }");
	}
	for(i = 0; i < tz->num_types; i++) {
		log_debug("  TimeType {");
		log_debug("    gmtoff = %d", tz->types[i].gmtoff);
		log_debug("    isdst = %d", tz->types[i].isdst);
		log_debug("    tzname_idx = %d [%s]", tz->types[i].tzname_idx, tz->tznames+tz->types[i].tzname_idx);
		log_debug("  }");
	}
	log_debug("}");
}

*/

/* EOB */
