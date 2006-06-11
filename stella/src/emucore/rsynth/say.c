/*
    Copyright (c) 1994,2001-2003 Nick Ing-Simmons. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA

*/
#include "config.h"
/*  jms: deutsch */
/* $Id: say.c,v 1.1 2006-06-11 07:13:27 urchlay Exp $
   $Log: not supported by cvs2svn $
 * Revision 1.13  1994/11/08  13:30:50  a904209
 * 2.0 release
 *
 * Revision 1.12  1994/11/04  13:32:31  a904209
 * 1.99.1 - Change configure stuff
 *
 * Revision 1.11  1994/11/02  10:55:31  a904209
 * Add autoconf. Tested on SunOS/Solaris
 *
 * Revision 1.10  1994/10/04  17:12:50  a904209
 * 3rd pre-release
 *
 * Revision 1.9  1994/10/04  09:08:27  a904209
 * Next Patch merge
 *
 * Revision 1.8  1994/10/03  08:41:47  a904209
 * 2nd pre-release
 *
 * Revision 1.7  1994/09/19  15:48:29  a904209
 * Split hplay.c, gdbm dictionary, start of f0 contour, netaudio and HP ports
 *
 * Revision 1.6  1994/04/15  16:47:37  a904209
 * Edits for Solaris2.3 (aka SunOs 5.3)
 *
 * Revision 1.5  1994/02/24  15:03:05  a904209
 * Added contributed linux, NeXT and SGI ports.
 *
 * Revision 1.4  93/11/18  16:29:06  a904209
 * Migrated nsyth.c towards Jon's scheme - merge still incomplete
 *
 * Revision 1.3  93/11/16  14:32:44  a904209
 * Added RCS Ids, partial merge of Jon's new klatt/parwave
 *
 * Revision 1.3  93/11/16  14:00:58  a904209
 * Add IDs and merge Jon's klatt sources - incomplete
 *
 */
char *say_id = "$Id: say.c,v 1.1 2006-06-11 07:13:27 urchlay Exp $";
extern char *Revision;
#include <stdio.h>
#include <ctype.h>
#include "useconfig.h"
#include <math.h>
#ifdef OS2
#include <signal.h>
#include <setjmp.h>
#include <float.h>
#endif				/* OS2 */
#include "darray.h"
#include "rsynth.h"
#include "hplay.h"
#include "dict.h"
#include "lang.h"
#include "text.h"
#include "getargs.h"
#include "phones.h"
#include "aufile.h"
#include "charset.h"		/* national differences ,jms */
#include "english.h"
#include "deutsch.h"
#include "say.h"


unsigned
spell_out(char *word, int n, darray_ptr phone)
{
    unsigned nph = 0;
#ifndef DEBUG
    fprintf(stderr, "Spelling '%.*s'\n", n, word);
#endif				/* DEBUG */
    while (n-- > 0) {
	char *spellword = lang->char_name[deaccent(*word++) & 0x7F];
	fprintf(stderr, " %s\n", spellword);
	nph += NRL(spellword, strlen(spellword), phone);
	darray_append(phone, ' ');
	nph += 1;
    }
    return nph;
}

int
suspect_word(char *w, int n)
{
    unsigned char *s = (unsigned char *) w;
    int i = 0;
    int seen_lower = 0;
    int seen_upper = 0;
    int seen_vowel = 0;
    int last = 0;
#ifdef GERMAN
    if (n == 1)
	return 1;		/* jms: 1 char is not a word */
    /* jms  only "I" is... */
#endif				/* GERMAN */
    for (i = 0; i < n; i++) {
	int ch = *s++;
	if (i && last != '-' && isupper(ch))
	    seen_upper = 1;
	if (islower(ch)) {
	    seen_lower = 1;
	    ch = toupper(ch);
	}
	if (isvowel(ch, 1))
	    seen_vowel = 1;
	last = ch;
    }
    last = !seen_vowel || (seen_upper && seen_lower) || !seen_lower;
#ifdef DEBUG
    fprintf(stderr, "%d %.*s v=%d u=%d l=%d\n", last, n, w, seen_vowel,
	    seen_upper, seen_lower);
#endif
    return last;
}


static unsigned
xlate_word(char *word, int n, darray_ptr phone, int verbose)
{
    unsigned nph = 0;
    if (*word != '[') {
	if (have_dict) {
	    unsigned char *p = dict_find(word, n);
	    if (p) {
		unsigned char *s = p;
		while (*s) {
		    char *x = dialect[(unsigned) (*s++)];
		    while (*x) {
			darray_append(phone, *x++);
			nph++;
		    }
		}
		darray_append(phone, ' ');
		free(p);
		return nph + 1;
	    }
	    else {
		/* If supposed word contains '.' or '-' try breaking it up... */
		char *h = word;
		while (h < word + n) {
		    if (*h == '.' || *h == '-') {
			nph += xlate_word(word, h++ - word, phone, verbose);
			nph += xlate_word(h, word + n - h, phone, verbose);
			return nph;
		    }
		    else
			h++;
		}
	    }
	}
	if (suspect_word(word, n)) {
	    nph = spell_out(word, n, phone);
	    return (nph);
	}
	else {
#ifndef DEBUG
	    if (have_dict || verbose)
		fprintf(stderr, "Guess '%.*s'\n", n, word);
#endif
	    nph += NRL(word, n, phone);
	}
    }
    else {
	if ((++word)[(--n) - 1] == ']')
	    n--;
	while (n-- > 0) {
	    darray_append(phone, *word++);
	    nph++;
	}
    }
    // darray_append(phone, ' ');
    return nph + 1;
}


unsigned
_xlate_string(rsynth_t * rsynth, char *string, darray_ptr phone)
{
    unsigned nph = 0;
    unsigned char *s = (unsigned char *) string;
    int ch;
    while (isspace(ch = *s))
	s++;
    while ((ch = *s)) {
	unsigned char *word = s;
	if (isalpha(ch)) {
	    while (isalpha(ch = *s)
		   || ((ch == '\'' || ch == '-' || ch == '.')
		       && isalpha(s[1])))
		s++;
	    if (!ch || isspace(ch) || ispunct(ch)
		|| (isdigit(ch) && !suspect_word((char *) word, s - word))) {
		nph += xlate_word((char *) word, s - word, phone, rsynth_verbose(rsynth));
	    }
	    else {
		while ((ch = *s) && !isspace(ch) && !ispunct(ch))
		    s++;
		nph += spell_out((char *) word, s - word, phone);
	    }
	}
	else if (isdigit(ch) || (ch == '-' && isdigit(s[1]))) {
	    int sign = (ch == '-') ? -1 : 1;
	    long value = 0;
	    if (sign < 0)
		ch = *++s;
	    while (isdigit(ch = *s)) {
		value = value * 10 + ch - '0';
		s++;
	    }
	    if (ch == '.' && isdigit(s[1])) {
		word = ++s;
		nph += xlate_cardinal(value * sign, phone);
		nph += xlate_string(lang->point, phone);
		while (isdigit(ch = *s))
		    s++;
		nph += spell_out((char *) word, s - word, phone);
	    }
	    else {
		/* check for ordinals, date, time etc. can go in here */
		nph += xlate_cardinal(value * sign, phone);
	    }
	}
	else if (ch == '[' && strchr((char *) s, ']')) {
	    unsigned char *word = s;
	    while (*s && *s++ != ']')
		/* nothing */ ;
	    nph += xlate_word((char *) word, s - word, phone, rsynth_verbose(rsynth));
	}
	else if (ispunct(ch)) {
	    switch (ch) {
		/* On end of sentence flush the buffer ... */
	    case '!':
	    case '?':
	    case '.':
#if 1
	    case ';':
	    case ',':
	    case '(':
	    case ')':
#endif
		if ((!s[1] || isspace(s[1])) && phone->items) {
		    if (rsynth) {
			rsynth_phones(rsynth,
				      (char *) darray_find(phone, 0),
				      phone->items);
			phone->items = 0;
		    }
		}
		s++;
		darray_append(phone, ' ');
		break;
	    case '"':		/* change pitch ? */
	    case ':':
	    case '-':
#if 0
	    case ';':
	    case ',':
	    case '(':
	    case ')':
#endif
		s++;
		darray_append(phone, ' ');
		break;
	    case '[':
		{
		    unsigned char *e =
			(unsigned char *) strchr((char *) s, ']');
		    if (e) {
			s++;
			while (s < e)
			    darray_append(phone, *s++);
			s = e + 1;
			break;
		    }
		}
	    default:
		nph += spell_out((char *) word, 1, phone);
		s++;
		break;
	    }
	}
	else {
	    while ((ch = *s) && !isspace(ch))
		s++;
	    nph += spell_out((char *) word, s - word, phone);
	}
	while (isspace(ch = *s))
	    s++;
    }
    return nph;
}

unsigned
xlate_string(char *string, darray_ptr phone)
{
    return _xlate_string(0, string, phone);
}

void
say_string(rsynth_t * rsynth, char *s)
{
    darray_t phone;
    darray_init(&phone, sizeof(char), 256);	/* chg jms */
    _xlate_string(rsynth, s, &phone);
    if (phone.items)
	rsynth_phones(rsynth, (char *) darray_find(&phone, 0), phone.items);
    darray_free(&phone);
}


void
say_file(rsynth_t * rsynth, FILE * f)
{
    darray_t line;
    darray_t phone;
    darray_init(&line, sizeof(char), 128);
    darray_init(&phone, sizeof(char), 128);
    while (darray_fget(f, &line)) {
	_xlate_string(rsynth, (char *) darray_find(&line, 0), &phone);
	line.items = 0;
    }
    if (phone.items)
	rsynth_phones(rsynth,
		      (char *) darray_find(&phone, 0), phone.items);
    darray_free(&phone);
    darray_free(&line);
}

