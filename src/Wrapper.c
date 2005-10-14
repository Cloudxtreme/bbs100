/*
    bbs100 3.0 WJ105
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

Wrapper *set_Wrapper(Wrapper *w, int flags, int *addr, int *mask, char *comment) {
	if (w == NULL)
		return NULL;

	w->flags = flags;

	if (addr == NULL)
		memset(w->addr, 0, sizeof(int)*8);
	else
		memcpy(w->addr, addr, sizeof(int)*8);

	if (mask == NULL)
		memset(w->mask, 0, sizeof(int)*8);
	else
		memcpy(w->mask, mask, sizeof(int)*8);

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

		add_Wrapper(&AllWrappers, w);
	}
	closefile(f);
	return 0;

err_load_Wrapper:
	closefile(f);
	listdestroy_Wrapper(AllWrappers);
	AllWrappers = NULL;
	return -1;
}

Wrapper *make_Wrapper(char *allowbuf, char *netbuf, char *maskbuf, char *comment) {
Wrapper *w;
int flags, addr[8], mask[8];

	if (allowbuf == NULL || !*allowbuf
		|| netbuf == NULL || !*netbuf
		|| maskbuf == NULL || !*maskbuf)
		return NULL;

	flags = 0;
	if (read_inet_addr(netbuf, addr, &flags))
		return NULL;

	if (read_inet_mask(maskbuf, mask, flags))
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

	if (set_Wrapper(w, flags, addr, mask, comment) == NULL) {
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
			print_inet_addr(w->addr, addr_buf, MAX_LINE, w->flags),
			print_inet_mask(w->mask, mask_buf, MAX_LINE, w->flags)
		);
		if (w->comment != NULL)
			fprintf(f->f, "\t# %s", w->comment);
		fprintf(f->f, "\n");

		w = w->next;
	}
	closefile(f);
	return 0;
}


/*
	see if addr is allowed to connect: use standard tcpd method ;
	always allow, unless denied

	it is possible to deny whole networks and allow specific hosts

	Mind that IPv4 masks can block IPv6 addresses and vice versa;
	internally the wrappers are all converted to 8 16-bit words
	The reason for this is that when you connect to ::ffff:127.0.0.1,
	it presents itself in a different way than when you use 127.0.0.1
	or ::1,	when they are really all the same destination (in the end)
	Therefore I feel it is OK to treat the wrappers all in the same manner,
	whether they are for IPv4 addresses or not
*/
int allow_Wrapper(char *ipnum, int apply_all) {
Wrapper *w;
int addr[8], flags;

	if (ipnum == NULL)
		return 1;			/* bug or other problem; allow */

	flags = 0;
	if (read_inet_addr(ipnum, addr, &flags)) {
		log_err("allow_Wrapper(): read_inet_addr(%s) failed (bug?), allowing connection", ipnum);
		return 1;
	}
/* see if it is explicitly allowed */
	for(w = AllWrappers; w != NULL; w = w->next) {
		if (apply_all && !(w->flags & WRAPPER_APPLY_ALL))
			continue;

		if ((w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, addr))
			return 1;
	}
/* see if denied */
	for(w = AllWrappers; w != NULL; w = w->next) {
		if (apply_all && !(w->flags & WRAPPER_APPLY_ALL))
			continue;

		if (!(w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, addr))
			return 0;
	}
/* default: allow */
	return 1;
}

int allow_one_Wrapper(Wrapper *w, char *ipnum, int apply_all) {
int addr[8], flags;

	if (w == NULL)
		return 1;

	flags = 0;
	if (read_inet_addr(ipnum, addr, &flags)) {
		log_err("allow_one_Wrapper(): read_inet_addr(%s) failed (bug?), allowing connection", ipnum);
		return 1;
	}
/* see if it is explicitly allowed */

	if (apply_all && !(w->flags & WRAPPER_APPLY_ALL))
		return 1;

	if ((w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, addr))
		return 1;

/* see if denied */
	if (!(w->flags & WRAPPER_ALLOW) && mask_Wrapper(w, addr))
		return 0;

/* default: allow */
	return 1;
}

/*
	see if the mask fits
	(this doesn't say anything about allowing or denying)
*/
int mask_Wrapper(Wrapper *w, int *addr) {
int i;

	if (w == NULL) {
		log_err("check_Wrapper(): BUG! w == NULL");
		return 0;
	}
	for(i = 0; i < 8; i++) {
		if ((w->addr[i] & w->mask[i]) != (addr[i] & w->mask[i]))
			break;
	}
	if (i == 8)
		return 1;

	return 0;
}

/*
	read IPv4 or IPv6 internet address

	IPv4 addresses are internally converted to ::ffff:123.123.123.123
	although this is technically incorrect (since we may well be listening
	to an IPv4 interface..!) but this does work OK for the wrappers

	This function can probably be simplified a lot by using getnameinfo(),
	but I want to stay away from using the internals of sockaddr_in and
	sockaddr_in6
*/
int read_inet_addr(char *ipnum, int *ip6, int *flags) {
int a1, a2, a3, a4, l, i, num_colons, abbrev;
char addr[MAX_LINE], *p, *startp;

	if (ipnum == NULL || !*ipnum || ip6 == NULL || flags == NULL)
		return -1;

	cstrncpy(addr, ipnum, MAX_LINE);

	if (sscanf(addr, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4) {
		if (a1 < 0 || a1 > 255
			|| a2 < 0 || a2 > 255
			|| a3 < 0 || a3 > 255
			|| a4 < 0 || a4 > 255) {
			log_err("read_inet_addr(): invalid IPv4 address %s", ipnum);
			return -1;
		}
		ip6[0] = ip6[1] = ip6[2] = ip6[3] = ip6[4] = 0;
		ip6[5] = 0xffff;
		ip6[6] = (a1 << 8) | a2;
		ip6[7] = (a3 << 8) | a4;
		*flags |= WRAPPER_IP4;
		return 0;
	}
	*flags = 0;
	if (cstrchr(addr, ':') == NULL) {
		log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
		return -1;
	}
	if (*addr == '[') {
		l = strlen(addr)-1;
		if (addr[l] != ']') {
			log_err("read_inet_addr(): missing ']' at the end of address '%s'\n", ipnum);
			return -1;
		}
		addr[l] = 0;
		memmove(addr, addr+1, l);
		l--;
	}
	memset(ip6, 0, sizeof(int)*8);
	num_colons = abbrev = 0;
	startp = addr;

/* start at left-hand side */
	i = 0;
	while((p = cstrchr(startp, ':')) != NULL) {
		*p = 0;
		num_colons++;
		if (num_colons > 7) {
			log_err("read_inet_addr(): too many colons in '%s'\n", ipnum);
			return -1;
		}
		p++;
		if (*p == ':') {		/* double colon */
			abbrev = 1;
			num_colons++;
			if (num_colons > 7) {
				log_err("read_inet_addr(): too many colons in '%s'\n", ipnum);
				return -1;
			}
			p++;
			if (*p == ':') {
				log_err("read_inet_addr(): invalid syntax '%s'\n", ipnum);
				return -1;
			}
			if (!*startp)
				break;
		}
		if (!*startp) {
			log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
			return -1;
		}
		if (sscanf(startp, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4) {
			if (i != 6) {
				log_err("read_inet_addr(): invalid mixed address '%s'\n", ipnum);
				return -1;
			}
			if (a1 < 0 || a1 > 255
				|| a2 < 0 || a2 > 255
				|| a3 < 0 || a3 > 255
				|| a4 < 0 || a4 > 255) {
				log_err("read_inet_addr(): invalid mixed address %s", ipnum);
				return -1;
			}
			ip6[6] = (a1 << 8) | a2;
			ip6[7] = (a3 << 8) | a4;
			startp = p = NULL;
			num_colons++;
			*flags |= WRAPPER_MIXED;
			break;
		} else {
			if (!is_hexadecimal(startp)) {
				log_err("read_inet_addr(): invalid characters in address '%s'\n", ipnum);
				return -1;
			}
			ip6[i] = (int)cstrtoul(startp, 16);
			if (ip6[i] < 0 || ip6[i] > 0xffff) {
				log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
				return -1;
			}
			i++;
		}
		if (abbrev)			/* found a double colon */
			break;

		startp = p;
	}
	if (p == NULL && startp != NULL && *startp) {
		if (sscanf(startp, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4) {
			if (i != 6) {
				log_err("read_inet_addr(): invalid mixed address '%s'\n", ipnum);
				return -1;
			}
			if (a1 < 0 || a1 > 255
				|| a2 < 0 || a2 > 255
				|| a3 < 0 || a3 > 255
				|| a4 < 0 || a4 > 255) {
				log_err("read_inet_addr(): invalid mixed address %s", ipnum);
				return -1;
			}
			ip6[6] = (a1 << 8) | a2;
			ip6[7] = (a3 << 8) | a4;
			startp = p = NULL;
			num_colons++;
			*flags |= WRAPPER_MIXED;
		} else {
			if (!is_hexadecimal(startp)) {
				log_err("read_inet_addr(): invalid characters in address '%s'\n", ipnum);
				return -1;
			}
			ip6[i] = (int)cstrtoul(startp, 16);
			if (ip6[i] < 0 || ip6[i] > 0xffff) {
				log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
				return -1;
			}
			i++;
		}
	}
	if (p != NULL && *p) {
/* right-hand side */
		i = 7;
		startp = p;
		while((p = cstrrchr(startp, ':')) != NULL) {
			num_colons++;
			if (num_colons > 7) {
				fprintf(stderr, "too many colons\n");
				return -1;
			}
			if (abbrev && p > startp && p[-1] == ':') {
				log_err("read_inet_addr(): multiple double colons in '%s'\n", ipnum);
				return -1;
			}
			*p = 0;
			p++;
			if (!*p) {
				log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
				return -1;
			}
			if (sscanf(p, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4) {
				if (i != 7) {
					log_err("read_inet_addr(): invalid mixed address '%s'\n", ipnum);
					return -1;
				}
				if (a1 < 0 || a1 > 255
					|| a2 < 0 || a2 > 255
					|| a3 < 0 || a3 > 255
					|| a4 < 0 || a4 > 255) {
					log_err("read_inet_addr(): invalid mixed address %s", ipnum);
					return -1;
				}
				ip6[6] = (a1 << 8) | a2;
				ip6[7] = (a3 << 8) | a4;
				i = 5;
				num_colons++;
				*flags |= WRAPPER_MIXED;
			} else {
				if (!is_hexadecimal(p)) {
					log_err("read_inet_addr(): invalid characters in address '%s'\n", ipnum);
					return -1;
				}
				ip6[i] = (int)cstrtoul(p, 16);
				if (ip6[i] < 0 || ip6[i] > 0xffff) {
					log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
					return -1;
				}
				i--;
			}
			if (i < 0)
				break;
		}
	}
	if (p == NULL && startp != NULL && *startp && i >= 0) {
		if (sscanf(startp, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4) {
			if (i != 7) {
				log_err("read_inet_addr(): invalid mixed address '%s'\n", ipnum);
				return -1;
			}
			if (a1 < 0 || a1 > 255
				|| a2 < 0 || a2 > 255
				|| a3 < 0 || a3 > 255
				|| a4 < 0 || a4 > 255) {
				log_err("read_inet_addr(): invalid mixed address %s", ipnum);
				return -1;
			}
			ip6[6] = (a1 << 8) | a2;
			ip6[7] = (a3 << 8) | a4;
			i = 5;
			num_colons++;
			*flags |= WRAPPER_MIXED;
		} else {
			if (!is_hexadecimal(startp)) {
				log_err("read_inet_addr(): invalid characters in address '%s'\n", ipnum);
				return -1;
			}
			ip6[i] = (int)cstrtoul(startp, 16);
			if (ip6[i] < 0 || ip6[i] > 0xffff) {
				log_err("read_inet_addr(): invalid address '%s'\n", ipnum);
				return -1;
			}
			i--;
		}
	}
	if (!abbrev && num_colons != 7) {
		log_err("read_inet_addr(): too few colons in '%s'\n", ipnum);
		return -1;
	}
	return 0;
}

int read_inet_mask(char *maskbuf, int *mask, int flags) {
int bits;

	if (maskbuf == NULL || !*maskbuf || mask == NULL)
		return -1;

	if (is_numeric(maskbuf)) {
		bits = cstrtoul(maskbuf, 10);

		if (flags & WRAPPER_IP4)
			ipv4_bitmask(bits, mask);
		else
			ipv6_bitmask(bits, mask);
		return 0;
	}
	bits = 0;
	if (read_inet_addr(maskbuf, mask, &bits))
		return -1;

	if (bits & WRAPPER_IP4)
		mask[0] = mask[1] = mask[2] = mask[3] = mask[4] = 0xffff;

	return 0;
}

char *print_inet_addr(int *addr, char *buf, int buflen, int flags) {
	if (addr == NULL || buf == NULL || buflen <= 0)
		return NULL;

	if (flags & WRAPPER_IP4)
		return print_ipv4_addr(addr, buf, buflen);

	return print_ipv6_addr(addr, buf, buflen, flags);
}

char *print_ipv4_addr(int *addr, char *buf, int buflen) {
int a1, a2, a3, a4;

	if (addr == NULL || buf == NULL || buflen <= 0)
		return NULL;

	a1 = (addr[6] >> 8) & 0xff;
	a2 = addr[6] & 0xff;
	a3 = (addr[7] >> 8) & 0xff;
	a4 = addr[7] & 0xff;

	bufprintf(buf, buflen, "%d.%d.%d.%d", a1, a2, a3, a4);
	return buf;
}

/*
	print IPv6 address into buffer

	flags are WRAPPER_xxx flags
*/
char *print_ipv6_addr(int *addr, char *buf, int buflen, int flags) {
int i, num, best_zero_pos, longest_zero_len, zero_pos, zero_len, l;

	if (addr == NULL || buf == NULL || buflen <= 0)
		return NULL;

	l = 0;
	*buf = 0;

	best_zero_pos = -1;
	longest_zero_len = 0;

	if (flags & WRAPPER_MIXED)
		num = 6;
	else
		num = 8;

	for(i = 0; i < num; i++) {
		if (!addr[i]) {
			zero_pos = i;
			zero_len = 0;
			while(!addr[i] && i < 8) {
				zero_len++;
				i++;
			}
			if (zero_len > longest_zero_len) {
				longest_zero_len = zero_len;
				best_zero_pos = zero_pos;
			}
		}
	}
	if (longest_zero_len == 1) {
		longest_zero_len = 0;
		best_zero_pos = -1;
	}
	for(i = 0; i < num; i++) {
		if (i == best_zero_pos) {
			i += longest_zero_len-1;
			buf[l++] = ':';
			buf[l++] = ':';
			continue;
		}
		l += bufprintf(buf+l, buflen - l, "%x", addr[i]);

		if ((i < (num - 1)) && best_zero_pos != i+1)
			buf[l++] = ':';
	}
	if (flags & WRAPPER_MIXED) {
		if (buf[l-1] != ':')
			buf[l++] = ':';

		l += bufprintf(buf+l, buflen - l, "%d.%d.%d.%d", (addr[6] >> 8) & 0xff, addr[6] & 0xff, (addr[7] >> 8) & 0xff, addr[7] & 0xff);
	}
	buf[l] = 0;
	return buf;
}

char *print_inet_mask(int *mask, char *buf, int buflen, int flags) {
int i, b, bits, num_bits, zeroes, complex_mask;

	if (mask == NULL || buf == NULL || buflen <= 0)
		return NULL;

/* see if we can print the mask as '/24' or something like that */

	num_bits = zeroes = complex_mask = 0;

	for(i = (flags & WRAPPER_IP4) ? 6 : 0; i < 8; i++) {
		for(b = 15; b >= 0; b--) {
			bits = 1 << b;
			if (mask[i] & bits) {
				if (zeroes) {
					complex_mask = 1;
					break;
				}
				num_bits++;
			} else
				zeroes = 1;
		}
		if (complex_mask)
			break;
	}
	if (!complex_mask) {
		bufprintf(buf, buflen, "%d", num_bits);
		return buf;
	}
	return print_inet_addr(mask, buf, buflen, flags);
}

void ipv4_bitmask(int bits, int *mask) {
int i, num, rest;

	if (bits < 0)
		bits = 0;

	if (bits > 32)
		bits = 32;

	num = 6 + bits / 16;
	rest = bits % 16;

	for(i = 0; i < num; i++)
		mask[i] = 0xffff;

	if (rest)
		mask[i++] = (0xffff >> rest) ^ 0xffff;

	while(i < 8)
		mask[i++] = 0;
}

void ipv6_bitmask(int bits, int *mask) {
int i, num, rest;

	if (bits < 0)
		bits = 0;

	if (bits > 128)
		bits = 128;

	num = bits / 16;
	rest = bits % 16;

	for(i = 0; i < num; i++)
		mask[i] = 0xffff;

	if (rest)
		mask[i++] = (0xffff >> rest) ^ 0xffff;

	while(i < 8)
		mask[i++] = 0;
}

/* EOB */
