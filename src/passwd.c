/*
	passwd.c	WJ99

	DES crypt a phrase	WJ97
	- revised by WJ99

	These functions take a phrase and cut it into 8-character chunks,
	which are in turn DES encrypted one by one using different salts.
	In order to fool traditional dictionary crackers, 4-character chunks
	of the phrase are reversed, resulting in some quite unusual passwords
	for each chunk.

	I removed my own fast DES_crypt() routine and replaced it with the
	standard crypt(). I do not want to have people misuse this bbs' code
	for writing password crackers. (they can download that anywhere anyway,
	but who cares)

	FreeBSD 4.0 has the possibility of installing an MD5 encryption method
	for crypt(3). The old crypt_phrase() couldn't handle MD5 crypto strings.
	From version 1.1.3 on, init_crypt() detects whether the system uses a
	standard DES crypt() or not, and decides whether to use the long
	crypt_phrase() routine or just plain crypt().
*/

#include <config.h>

#include "passwd.h"
#include "Timer.h"
#include "sys_time.h"
#include "cstring.h"

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>

static char chartab[256];
static int std_crypt = 1;		/* does this system have a standard DES crypt(3) ? */

/*
	initialize salt character table

	1.1.3 : detect whether we have a system that supports '$type$salt$crypto'
*/
void init_crypt(void) {
int i;
char c = '.';

	for(i = 0; i < 256; i++) {
		chartab[i] = c;
		c++;
		if (c == '9'+1)
			c = 'A';
		if (c == 'Z'+1)
			c = 'a';
		if (c == 'z'+1)
			c = '.';
	}
	std_crypt = detect_std_crypt();
}

int detect_std_crypt(void) {
char salt[3], *p;

	salt[0] = 'A';
	salt[1] = 'B';
	salt[2] = 0;

	p = crypt("std_crypt() test!", salt);
	if (p != NULL) {
		if (strlen(p) != 13)
			return 0;

		if (p[0] != salt[0] || p[1] != salt[1])
			return 0;
	} else {				/* crypt of long string failed, try with a short one */
		p = crypt("stdcrypt", salt);
		if (p != NULL) {
			if (strlen(p) != 13)
				return 0;

			if (p[0] != salt[0] || p[1] != salt[1])
				return 0;
		}
	}
	return 1;
}


/* fill string up to nearest multiply */
static void fill(char *str, int mul) {
int filler, i, len;

	len = strlen(str);
	filler = len % mul;
	if (!filler)
		return;

	filler = mul - filler;
	for(i = 0; i < filler; i++)
		str[len++] = ' ';
	str[len] = 0;
}

/* reverse parts of the string to fool dictionary crackers */
static void reverse4(char *str) {
int len, i;
char c;

	len = strlen(str);
	for(i = 0; i < len; i+=4) {
		c = str[i];
		str[i] = str[i+3];
		str[i+3] = c;

		c = str[i+1];
		str[i+1] = str[i+2];
		str[i+2] = c;
	}
}


/*
	DES crypt a phrase
	Note: returns a static buffer
	Note: init_crypt() should have been called before

	Since 'salt' is in this function internally, and since it is
	dependant on time and pid, you cannot use this function to
	check for pass-phrases. Use verify_passphrase() instead (see below)
*/
char *crypt_phrase(char *phrase) {
static char buf[MAX_CRYPTED];
char salt[3], phrase_buf[MAX_PASSPHRASE+1];
pid_t pid;

	strncpy(phrase_buf, phrase, MAX_PASSPHRASE);
	phrase_buf[MAX_PASSPHRASE] = 0;
	phrase = phrase_buf;

	pid = getpid();
	salt[0] = (char)(rtc ^ pid);
	salt[1] = (char)((rtc >> 4) ^ pid);
	salt[2] = 0;

	if (!std_crypt) {
/*
	System has a non-standard crypt() function, assume it will correctly
	handle a phrase as input
*/
		strncpy(buf, crypt(phrase_buf, salt), MAX_CRYPTED);
		buf[MAX_CRYPTED-1] = 0;
		return buf;
	} else {
/*
	System has a standard crypt() function, use the BBSes algorithm
	to crypt a phrase
*/
		char *bufp;
		int len, plen = 0;

		len = strlen(phrase);
		if (len >= MAX_PASSPHRASE)
			phrase[MAX_PASSPHRASE] = 0;
		else
			if (len < 16)
				fill(phrase, 16);

		fill(phrase, 4);
		reverse4(phrase);
		len = strlen(phrase);

		bufp = buf;

		for(;;) {
			salt[0] = chartab[salt[0] & 255];
			salt[1] = chartab[salt[1] & 255];
			strncpy(bufp, crypt(phrase+plen, salt), 13);
			bufp += 13;

			plen += 8;
			if (plen >= len)
				break;

			salt[0] >>= 1;
			salt[0] ^= pid;
			salt[1] >>= 1;
			salt[1] ^= pid;
		}
		*bufp = 0;
	}
	return buf;
}

/*
	verify a crypt_phrase()d pass-phrase
*/
int verify_phrase(char *phrase, char *crypted) {
char buf[MAX_CRYPTED], salt[256];

	salt[2] = 0;

	if (!std_crypt) {
		char *d;

/* get the salt of a "$num$salt$crypto" format string */
		if (*crypted == '$' && (d = cstrchr(crypted+1, '$')) != NULL) {
			*d = 0;
			strcpy(salt, crypted+1);
			*d = '$';
		} else {
			salt[0] = crypted[0];
			salt[1] = crypted[1];
		}
		strncpy(buf, crypt(phrase, salt), MAX_CRYPTED);
		buf[MAX_CRYPTED-1] = 0;
	} else {
		char *bufp, phrase_buf[MAX_PASSPHRASE+1];
		int crypted_len, clen = 0;
		int phrase_len, plen = 0;

		bufp = buf;

		strncpy(phrase_buf, phrase, MAX_PASSPHRASE);
		phrase_buf[MAX_PASSPHRASE] = 0;
		phrase = phrase_buf;

		phrase_len = strlen(phrase);
		if (phrase_len >= MAX_PASSPHRASE)
			phrase[MAX_PASSPHRASE] = 0;
		else
			if (phrase_len < 16)
				fill(phrase, 16);

		fill(phrase, 4);
		reverse4(phrase);
		phrase_len = strlen(phrase);

		crypted_len = strlen(crypted);

		for(;;) {
			salt[0] = crypted[clen];
			salt[1] = crypted[clen+1];
			strncpy(bufp, crypt(phrase+plen, salt), 13);
			bufp += 13;
			clen += 13;
			plen += 8;
			if (clen >= crypted_len)
				break;
			if (plen >= phrase_len)
				break;
		}
/* check this to prevent a match of only a part of the entire phrase */
		if (clen < crypted_len || plen < phrase_len)
			return -1;

		*bufp = 0;
	}
	return strcmp(buf, crypted);		/* 0 if it matches */
}

/* EOB */
