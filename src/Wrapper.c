/*
    bbs100 1.2.3 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
#include "AtomicFile.h"

#include <stdio.h>
#include <stdlib.h>

Wrapper *new_Wrapper(int allow, unsigned long net, unsigned long mask,
	char *comment) {
Wrapper *w;

	if ((w = (Wrapper *)Malloc(sizeof(Wrapper), TYPE_WRAPPER)) == NULL)
		return NULL;

	w->allow = allow;
	w->net = net;
	w->mask = mask;

	if (comment != NULL && *comment) {
		while(*comment && (*comment == ' ' || *comment == '\t' || *comment == '#' || *comment == '\n'))
			comment++;

		if (*comment)
			w->comment = cstrdup(comment);
	} else
		w->comment = NULL;
	return w;
}

void destroy_Wrapper(Wrapper *w) {
	if (w == NULL)
		return;

	Free(w->comment);
	Free(w);
}



int load_Wrapper(Wrapper **root, char *filename) {
AtomicFile *f;
char buf[4096], *p, *allowbuf, *netbuf, *maskbuf, *comment;

	if (root == NULL || filename == NULL || (f = openfile(filename, "r")) == NULL)
		return -1;

	while(fgets(buf, 4096, f->f) != NULL) {
		cstrip_line(buf);
		cstrip_spaces(buf);
		if (!*buf || *buf == '#')			/* ignore comment */
			continue;

/* keyword 'allow|deny' */
		allowbuf = buf;

		if ((p = cstrchr(buf, ' ')) == NULL)
			goto err_load_Wrapper;
		*p = 0;
		p++;
/* get net address */
		netbuf = p;

		if ((p = cstrchr(p, '/')) == NULL)
			goto err_load_Wrapper;
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

		if (make_Wrapper(root, allowbuf, netbuf, maskbuf, comment) == NULL)
			goto err_load_Wrapper;
	}
	closefile(f);
	return 0;

err_load_Wrapper:
	closefile(f);
	if (root != NULL) {
		listdestroy_Wrapper(*root);
		*root = NULL;
	}
	return -1;
}

Wrapper *make_Wrapper(Wrapper **root, char *allowbuf, char *netbuf,
	char *maskbuf, char *comment) {
Wrapper *wrapper;
int n1, n2, n3, n4, m1, m2, m3, m4, allow;
unsigned long net, mask;

	if (root == NULL
		|| allowbuf == NULL || !*allowbuf
		|| netbuf == NULL || !*netbuf
		|| maskbuf == NULL || !*maskbuf)
		return NULL;

	if (sscanf(netbuf, "%d.%d.%d.%d", &n1, &n2, &n3, &n4) != 4)
		return NULL;

	if (n1 < 0 || n1 > 255
		|| n2 < 0 || n2 > 255
		|| n3 < 0 || n3 > 255
		|| n4 < 0 || n4 > 255) {
		log_err("malformed net address '%s'", netbuf);
		return NULL;
	}
	net = n1;
	net <<= 8;
	net |= n2;
	net <<= 8;
	net |= n3;
	net <<= 8;
	net |= n4;

	if (sscanf(maskbuf, "%d.%d.%d.%d", &m1, &m2, &m3, &m4) != 4)
		return NULL;

	if (m1 < 0 || m1 > 255
		|| m2 < 0 || m2 > 255
		|| m3 < 0 || m3 > 255
		|| m4 < 0 || m4 > 255) {
		log_err("malformed net mask '%s'", maskbuf);
		return NULL;
	}
	mask = m1;
	mask <<= 8;
	mask |= m2;
	mask <<= 8;
	mask |= m3;
	mask <<= 8;
	mask |= m4;

	if (!cstricmp(allowbuf, "allow"))
		allow = 1;
	else
		if (!cstricmp(allowbuf, "deny"))
			allow = 0;
		else {
			log_err("unknown keyword '%s'; must be either 'allow' or 'deny'", allowbuf);
			return NULL;
		}

	if ((wrapper = new_Wrapper(allow, net, mask, comment)) == NULL)
		return NULL;

	add_Wrapper(root, wrapper);
	return wrapper;
}

int save_Wrapper(Wrapper *w, char *filename) {
AtomicFile *f;
int n, m;

	if (filename == NULL || (f = openfile(filename, "w")) == NULL)
		return -1;

	while(w != NULL) {
		n = w->net;
		m = w->mask;
		fprintf(f->f, "%s %d.%d.%d.%d/%d.%d.%d.%d", (w->allow == 0) ? "deny" : "allow",
			(n >> 24) & 255, (n >> 16) & 255, (n >> 8) & 255, n & 255,
			(m >> 24) & 255, (m >> 16) & 255, (m >> 8) & 255, m & 255);
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
	since whole networks can be denied and specific hosts within those networks
	can be allowed, we have to check both
*/
int allow_Wrapper(Wrapper *root, unsigned long addr) {
Wrapper *w;

/* see if allowed */
	for(w = root; w != NULL; w = w->next)
		if (w->allow && (w->net == (addr & w->mask)))
			return 1;

/* see if denied */
	for(w = root; w != NULL; w = w->next)
		if (!w->allow && (w->net == (addr & w->mask)))
			return 0;

/* default: allowed */
	return 1;
}

/* EOB */
