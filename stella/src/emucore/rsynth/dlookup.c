/*
    Copyright (c) 1994,2001-2002 Nick Ing-Simmons. All rights reserved.

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
#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "useconfig.h"
#include "dict.h"
#include "phones.h"
#include "getargs.h"
#include "english.h"

lang_t *lang = &English;


static void show(char *s);
static void
show(char *s)
{
    unsigned char *p = dict_find(s, strlen(s));
    printf("%s", s);
    if (p) {
	int l = strlen((char *) p);
	int i;
	for (i = 0; i < l; i++)
	    printf(" %s", ph_name[(unsigned) (p[i])]);
	printf(" [");
	for (i = 0; i < l; i++)
	    printf("%s", dialect[(unsigned) (p[i])]);
	printf("]\n");
	free(p);
    }
    else
	printf(" ???\n");
}


int
main(int argc, char **argv, char **envp)
{
    argc = getargs("Dictionary", argc, argv,
		   "d", "", &dict_path, "Which dictionary [b|a|g]", NULL);



    if (help_only) {
	fprintf(stderr, "Usage: %s [options as above] words to lookup\n",
		argv[0]);
    }
    else {
	if (dict_path && *dict_path)
	    dict_init(dict_path);
	if (have_dict) {
	    int i;
	    for (i = 1; i < argc; i++) {
		show(argv[i]);
	    }
	    dict_term();
	}
    }
    return 0;
}
