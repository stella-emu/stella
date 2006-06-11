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
/* jms: changed default for dict_path to "g" */
#include "config.h"
#ifdef HAVE_IO_H
#include <io.h>
#endif				/* HAVE_IO_H */
#include <stdio.h>
#include <ctype.h>
#include "charset.h"
#include "phones.h"
#include "useconfig.h"
#include "dbif.h"

#ifndef DEFAULT_DICT
#define DEFAULT_DICT "b"
#endif
char *dict_path = DEFAULT_DICT;

#ifdef DB_HANDLE

DB_HANDLE dict;

#include "lang.h"
#include "dict.h"

#ifndef DICT_DIR
#define DICT_DIR "/usr/local/lib/dict"
#endif


unsigned char *
dict_lookup(const char *s, unsigned int n)
{
    if (!n)
	n = strlen(s);
    if (dict) {
	unsigned char *p = (unsigned char *) s;
	DATUM key;
	DATUM data;
	unsigned int i;
	char *buf = (char *) malloc(n);
	DATUM_SET(key, buf, n);
	for (i = 0; i < n; i++) {
	    buf[i] = (isupper(p[i])) ? tolower(p[i]) : p[i];
	}
	DB_FETCH(dict, key, data);
	free(buf);
	if (DATUM_DATA(data)) {
	    unsigned char *w = (unsigned char *) realloc(DATUM_DATA(data),
							 DATUM_SIZE(data) +
							 1);
	    w[DATUM_SIZE(data)] = '\0';
	    return w;
	}
    }
    return NULL;
}

static void choose_dialect(void);

static void
choose_dialect(void)
{
    unsigned char *word = dict_find("wash", 0);
    if (word && word[0] == W) {
	if (word[1] == OH)
	    dialect = ph_br;
	else if (word[1] == AA1)
	    dialect = ph_am;
	free(word);
    }
}


int
dict_init(const char *dictname)
{
    char *buf = NULL;
#ifdef OS2
    char *dictdir = getenv("ETC");
    if (dictdir == NULL)
	dictdir = DICT_DIR;	/* jms: if ETC is not set */
#else
    char *dictdir = DICT_DIR;
#endif
    buf =
	(char *) malloc(strlen(dictdir) + strlen("Dict.db") +
			strlen(dict_path) + 2);
    sprintf(buf, "%sDict.db", dictname);
    DB_OPEN_READ(dict, buf);
    if (!dict) {
	sprintf(buf, "%s/%sDict.db", dictdir, dictname);
	DB_OPEN_READ(dict, buf);
    }
    if (dict) {
	dict_path = (char *) realloc(buf, strlen(buf) + 1);
	lang->lookup = dict_lookup;
	choose_dialect();
    }
    return have_dict;
}

void
dict_term(void)
{
    if (dict) {
	DB_CLOSE(dict);
	free(dict_path);
	lang->lookup = 0;
	dict = 0;
    }
}

#else

unsigned char *
dict_find(char *s, unsigned n)
{
    return NULL;
}

int
dict_init(int argc, char *argv[])
{
    return argc;
}

void
dict_term(void)
{

}

#endif
