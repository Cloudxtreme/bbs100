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
	Wrapper.c	WJ99

	just like tcp wrappers ; 'allow' and 'deny'
*/

#include "config.h"
#include "Wrapper.h"
#include "defines.h"
#include "cstring.h"
#include "util.h"
#include "log.h"
#include "cstring.h"
#include "Memory.h"
#include "memset.h"
#include "AtomicFile.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

Wrapper *AllWrappers = NULL;


Wrapper *new_Wrapper(void) {
	return (Wrapper *)Malloc(sizeof(Wrapper), TYPE_WRAPPER);
}

void destroy_Wrapper(Wrapper *w) {
	if (w == NULL)
		return;

	Free(w->comment);
	Free(w);
}

Wrapper *set_Wrapper(Wrapper *w, int flags, IP_addr *addr, IP_addr *mask, char *comment) {
int size;

	if (w == NULL)
		return NULL;

	w->flags = flags;

	size = (flags & WRAPPER_IP4) ? sizeof(struct in_addr) : sizeof(struct in6_addr);

	if (addr == NULL)
		memset(&w->addr, 0, size);
	else
		memcpy(&w->addr, addr, size);

	if (mask == NULL)
		memset(&w->mask, 0, size);
	else
		memcpy(&w->mask, mask, size);

	if (comment != NULL && *comment) {
		while(*comment && (*comment == ' ' || *comment == '\t' || *comment == '#' || *comment == '\n'))
			comment++;

		if (*comment)
			w->comment = cstrdup(comment);
	} else
		w->comment = NULL;

	return w;
}

int load_Wrapper(char *filename) {
AtomicFile *f;
Wrapper *w;
char buf[4096], *p, *allowbuf, *netbuf, *maskbuf, *comment;
int lineno;

	if (filename == NULL || (f = openfile(filename, "r")) == NULL)
		return -1;

	lineno = 0;
	while(fgets(buf, 4096, f->f) != NULL) {
		lineno++;
		cstrip_line(buf);
		cstrip_spaces(buf);
		if (!*buf || *buf == '#')			/* ignore comment */
			continue;

/* keyword 'allow|deny' */
		allowbuf = buf;

		if ((p = cstrchr(buf, ' ')) == NULL) {
			fprintf(stderr, "%s:%d: syntax error\n", filename, lineno);
			goto err_load_Wrapper;
		}
		*p = 0;
		p++;
/* get net address */
		netbuf = p;

		if ((p = cstrchr(p, '/')) == NULL) {
			fprintf(stderr, "%s:%d: syntax error\n", filename, lineno);
			goto err_load_Wrapper;
		}
		*p = 0;
		p++;
/* get netmask */
		maskbuf = p;

		p = cstrchr(p, ' ');
		if (p != NULL) {
			*p = 0;
			p++;			/* we have additional comment */
			if (!*p)
				p = NULL;
		}
		comment = p;

		if ((w = make_Wrapper(allowbuf, netbuf, maskbuf, comment)) == NULL)
			goto err_load_Wrapper;

		(void)add_Wrapper(&AllWrappers, w);
	}
	closefile(f);
	return 0;

err_load_Wrapper:
	cancelfile(f);
	listdestroy_Wrapper(AllWrappers);
	AllWrappers = NULL;
	return -1;
}

Wrapper *make_Wrapper(char *allowbuf, char *netbuf, char *maskbuf, char *comment) {
Wrapper *w;
int flags;
IP_addr addr, mask;

	if (allowbuf == NULL || !*allowbuf
		|| netbuf == NULL || !*netbuf
		|| maskbuf == NULL || !*maskbuf)
		return NULL;

	flags = 0;
	if (read_inet_addr(netbuf, &addr, &flags))
		return NULL;

	if (read_inet_mask(maskbuf, &mask, flags))
		return NULL;

	if (!cstricmp(allowbuf, "allow"))
		flags |= WRAPPER_ALLOW;
	else
		if (!cstricmp(allowbuf, "deny"))
			;
		else
			if (!cstricmp(allowbuf, "allow_all"))
				flags |= (WRAPPER_ALLOW|WRAPPER_APPLY_ALL);
			else
				if (!cstricmp(allowbuf, "deny_all"))
					flags |= WRAPPER_APPLY_ALL;
				else {
					log_err("make_Wrapper(): unknown keyword '%s'; must be either 'allow' or 'deny'", allowbuf);
					return NULL;
				}

	if ((w = new_Wrapper()) == NULL)
		return NULL;

	if (set_Wrapper(w, flags, &addr, &mask, comment) == NULL) {
		destroy_Wrapper(w);
		return NULL;
	}
	return w;
}

int save_Wrapper(Wrapper *w, char *filename) {
AtomicFile *f;
char addr_buf[MAX_LINE], mask_buf[MAX_LINE];

	if (filename == NULL || (f = openfile(filename, "w")) == NULL)
		return -1;

	while(w != NULL) {
		fprintf(f->f, "%s%s %s/%s", (w->flags & WRAPPER_ALLOW) ? "allow" : "deny",
			(w->flags & WRAPPER_APPLY_ALL) ? "_all" : "",
			print_inet_addr(&w->addr, addr_buf, MAX_LINE, w->flags),
			print_inet_mask(&w->mask, mask_buf, MAX_LINE, w->flags)
		);
		if (w->comment != NULL)
			fprintf(f->f, "\t# %s", w->comment);
		fprintf(f->f, "\n");

		w = w->next;
	}
	return closefile(f);
}


/*
	see if addr is allowed to connect: use standard tcpd method ;
	always allow, unless denied

	it is possible to deny whole networks and allow specific hosts
*/
int allow_Wrapper(char *ipnum, int apply_all) {
Wrapper *w;
IP_addr addr;
int flags;

	if (ipnum == NULL)
		return 1;			/* bug or other problem; allow */

	flags = 0;
	if (read_inet_addr(ipnum, &addr, &flags)) {
		log_err("allow_Wrapper(): read_inet_addr(%s) failed, allowing connection", ipnum);
		return 1;
	}
/* see if it is explicitly allowed */
	for(w = AllWrappers; w != NULL; w = w->next) {
		if (apply_all && !(w->flags & WRAPPER_APPLY_ALL))
			continue;

		if ((w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, &addr))
			return 1;
	}
/* see if denied */
	for(w = AllWrappers; w != NULL; w = w->next) {
		if (apply_all && !(w->flags & WRAPPER_APPLY_ALL))
			continue;

		if (!(w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, &addr))
			return 0;
	}
/* default: allow */
	return 1;
}

int allow_one_Wrapper(Wrapper *w, char *ipnum, int apply_all) {
int flags;
IP_addr addr;

	if (w == NULL)
		return 1;

	flags = 0;
	if (read_inet_addr(ipnum, &addr, &flags)) {
		log_err("allow_one_Wrapper(): read_inet_addr(%s) failed, allowing connection", ipnum);
		return 1;
	}
/* see if it is explicitly allowed */

	if (apply_all && !(w->flags & WRAPPER_APPLY_ALL))
		return 1;

	if ((w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, &addr))
		return 1;

/* see if denied */
	if (!(w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, &addr))
		return 0;

/* default: allow */
	return 1;
}

/*
	see if the mask fits
	(this doesn't say anything about allowing or denying)
*/
int mask_Wrapper(Wrapper *w, IP_addr *addr) {
int i, size;

	if (w == NULL) {
		log_err("check_Wrapper(): BUG! w == NULL");
		return 0;
	}
	size = (w->flags & WRAPPER_IP4) ? 4 : 16;
	for(i = 0; i < size; i++) {
		if ((w->addr.saddr[i] & w->mask.saddr[i]) != (addr->saddr[i] & w->mask.saddr[i]))
			break;
	}
	if (i == size)
		return 1;

	return 0;
}

int read_wrapper_addr(Wrapper *w, char *addr) {
int oldflags;

	if (w == NULL || addr == NULL)
		return -1;

	oldflags = w->flags;
	if (read_inet_addr(addr, &w->addr, &w->flags))
		return -1;

/* if type of wrapper changed, reset the high mask bits */
	if ((oldflags & WRAPPER_IP4) != (w->flags & WRAPPER_IP4))
		memset((char *)&w->mask+sizeof(struct in_addr), 0, sizeof(IP_addr) - sizeof(struct in_addr));

	return 0;
}

int read_wrapper_mask(Wrapper *w, char *mask) {
	return read_inet_mask(mask, &w->mask, w->flags);
}

/*
	read IPv4 or IPv6 internet address
*/
int read_inet_addr(char *ipnum, IP_addr *addr, int *flags) {
	if (ipnum == NULL || !*ipnum || addr == NULL || flags == NULL)
		return -1;

	*flags &= ~WRAPPER_IP4;
	if (inet_pton(AF_INET, ipnum, &addr->ipv4) > 0) {		/* try as IPv4 */
		*flags |= WRAPPER_IP4;
		return 0;
	}
	if (inet_pton(AF_INET6, ipnum, &addr->ipv6) > 0)		/* try as IPv6 */
		return 0;

	log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
	return -1;
}

int read_inet_mask(char *maskbuf, IP_addr *mask, int flags) {
int bits;

	if (maskbuf == NULL || !*maskbuf || mask == NULL)
		return -1;

	if (is_numeric(maskbuf)) {
		bits = cstrtoul(maskbuf, 10);
		ip_bitmask(bits, mask, flags);
		return 0;
	}
	bits = 0;
	if (read_inet_addr(maskbuf, mask, &bits))
		return -1;

	if (flags & WRAPPER_IP4) {
		if (bits & WRAPPER_IP4)
			return 0;

		ip_bitmask(32, mask, flags);
		return -1;
	}
	if (!(bits & WRAPPER_IP4))
		return 0;

	ip_bitmask(128, mask, flags);
	return -1;
}

char *print_inet_addr(IP_addr *addr, char *buf, int buflen, int flags) {
	if (addr == NULL || buf == NULL || buflen <= 0)
		return NULL;

	return (char *)inet_ntop((flags & WRAPPER_IP4) ? AF_INET : AF_INET6, addr->saddr, buf, buflen);
}

char *print_inet_mask(IP_addr *mask, char *buf, int buflen, int flags) {
int i, num_bits, complex_mask, size;
unsigned char bite, bits;

	if (mask == NULL || buf == NULL || buflen <= 0)
		return NULL;

/* see if we can print the mask as '/24' or something like that */

	num_bits = complex_mask = 0;

	size = (flags & WRAPPER_IP4) ? 4 : 16;
	for(i = 0; i < size; i++) {
		if ((unsigned char)mask->saddr[i] == 0xff) {
			num_bits += 8;
			continue;
		}
		bite = (unsigned char)mask->saddr[i];
		bits = (1 << 7);
		while(bits) {
			if (bite & bits) {		/* count number of set bits */
				bite &= ~bits;		/* clear that bit */
				num_bits++;
			} else {
				if (bite)			/* if only zeroes left, it's (possibly) a short mask */
					complex_mask = 1;
				break;
			}
			bits >>= 1;
		}
		if (complex_mask)
			break;

		for(i++; i < size; i++) {
			if (mask->saddr[i]) {	/* if only zeroes left, it's a short mask */
				complex_mask = 1;
				break;
			}
		}
	}
	if (!complex_mask) {
		bufprintf(buf, buflen, "%d", num_bits);
		return buf;
	}
	return print_inet_addr(mask, buf, buflen, flags);
}

/*
	generate bitmask from short (eg. '/24') netmask
*/
void ip_bitmask(int bits, IP_addr *mask, int flags) {
int i, num, rest, size;

	if (mask == NULL)
		return;

	memset(mask, 0, sizeof(IP_addr));

	size = (flags & WRAPPER_IP4) ? 4 : 16;		/* # of bytes in IPv4/6 address */

	if (bits < 0)
		bits = 0;

	if (bits > size*8)
		bits = size*8;

	num = bits / 8;
	rest = bits % 8;

	for(i = 0; i < num; i++)
		mask->saddr[i] = 0xff;

	if (rest)
		mask->saddr[i++] = ~(0xff >> rest);

	while(i < size)
		mask->saddr[i++] = 0;
}

/* EOB */
