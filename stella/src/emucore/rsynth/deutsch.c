/*
    Contributed to rsynth by a forgotten author (based on english.c)
    Changes Copyright (c) 1994,2001-2002 Nick Ing-Simmons. All rights reserved.
 
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
#include <stdio.h>
#include <ctype.h>
#include "darray.h"
#include "lang.h"
#include "say.h"
#include "deutsch.h"

/* $Id: deutsch.c,v 1.1 2006-06-11 07:13:24 urchlay Exp $
 */
char *deutsch_id = "$Id: deutsch.c,v 1.1 2006-06-11 07:13:24 urchlay Exp $";

static unsigned cardinal(long int value, darray_ptr phone);
static unsigned ordinal(long int value, darray_ptr phone);
static int vowel(int chr, int maybe);
static int consonant(int chr);
static Rule *rules(int ch);


/*
   deutsche phoneme
   **
   **
   **
   **      The Phoneme codes:
   **
   **       <blank>, %, p, t, k, b, d, g, m, n, N, f, T, h, v, D,
   **       j, tS, dZ, S, x, Z, s, z,
   **       l, r, rr R, w, u, U, i, I,
   **       A, &, V, @, o, O, 0, e, 3,
   **       eI aI oI aU @U I@ e@ U@ O@ oU
   **
   **
   **      Rules are made up of four parts:
   **
   **              The left context.
   **              The text to match.
   **              The right context.
   **              The phonemes to substitute for the matched text.
   **
   **      Procedure:
   **
   **              Seperate each block of letters (apostrophes included)
   **              and add a space on each side.  For each unmatched
   **              letter in the word, look through the rules where the
   **              text to match starts with the letter in the word.  If
   **              the text to match is found and the right and left
   **              context patterns also match, output the phonemes for
   **              that rule and skip to the next unmatched letter.
   **
   **
   **      Special Context Symbols:
   **
   **              #       One or more vowels
   **              :       Zero or more consonants
   **              ^       One consonant.
   **              $       duplicate consonant 
   **              .       One of B, D, V, G, J, L, M, N, R, W or Z (voiced
   **                      consonants)
   **              %       One of ER, E, ES, ED, ING, ELY (a suffix)
   **                      (Found in right context only)
   **              +       One of E, I or Y (a "front" vowel)
   **
 */


/* Context definitions */
static char Anything[] = "";
 /* No context requirement */

static char Nothing[] = " ";
 /* Context is beginning or end of word */

static char Silent[] = "";
 /* No phonemes */


#define LEFT_PART       0
#define MATCH_PART      1
#define RIGHT_PART      2
#define OUT_PART        3


/*0 = Punctuation */
/*
   **      LEFT_PART       MATCH_PART      RIGHT_PART      OUT_PART
 */


static Rule punct_rules[] =
{
 {Anything, " ", Anything, " "},
 {Anything, "-", Anything, ""},
 {".", "'S", Anything, "z"},
 {"#:.E", "'S", Anything, "z"},
 {"#", "'S", Anything, "z"},
 {Anything, "'", Anything, ""},
 {Anything, ",", Anything, " "},
 {Anything, ".", Anything, " "},
 {Anything, "?", Anything, " "},
 {Anything, "!", Anything, " "},
 {Anything, 0, Anything, Silent},
};

static Rule A_rules[] =
{
/*  {Anything, "A", Nothing, "@"}, */
 {Nothing, "A", Nothing, "A"},
 {Anything, "A", Nothing, "V"},
/*  {Anything, "A", "^+:#", "&"}, */
/*  {Anything, "A", "^%", "a"}, */
 {Anything, "AU", Anything, "aU"},
/*  {Anything, "A", Anything, "a"}, */
 {Anything, "AE", Anything, "ee"},	/* Ä */
 {Anything, "AR", Anything, "V@"},	/* narkose */
 {Nothing, "AB", "B", "Vb "},     /* abbilden */
 {Nothing, "AN", "N", "Vn "},     /* annehmen */

/*  {Anything, "A", "^^", "V"},     geht nicht! koennen auch verschiedene sein */
 {Anything, "A", "$", "V"},
                                           /* {Anything, "A", "ß", "A"},         *//* das */
                                           /* {Anything, "A", "S", "A"},         *//* das */
 {Anything, "A", "H", "VV"},
/* {Anything, "A", Anything, "V"}, */
 {Anything, "A", Anything, "A"},
 {Anything, 0, Anything, Silent},
};

static Rule B_rules[] =
{
 {"#", "BB", "#", "b"},
 {Anything, "B", Anything, "b"},
 {Anything, 0, Anything, Silent},
};

static Rule C_rules[] =
{
/*  {Nothing, "CH", "^", "k"}, */
/*  {Anything, "CH", "I", "x"},           //chinese */
/*  {Anything, "CH", Nothing, "x"},       //charakter */
 {Nothing, "CH", Anything, "k"},  /* charakter */
 {Anything, "CH", Anything, "x"},
 {Anything, "CI", "EN", "S"},
 {Anything, "CK", Anything, "k"},
 {Anything, "COM", "%", "kVm"},
 {Anything, "C", Anything, "k"},
 {Anything, 0, Anything, Silent},
};

static Rule D_rules[] =
{
 {"#:", "DED", Nothing, "dId"},
 {".E", "D", Nothing, "d"},
 {"#:^E", "D", Nothing, "t"},
 {"#", "DD", "#", "d"},
 {"#", "DD", Nothing, "d"},
 {Anything, "D", Anything, "d"},
 {Anything, 0, Anything, Silent},
};

static Rule E_rules[] =
{
/* {Nothing, "E", Nothing, "ee"}, */
/* {Nothing, Anything, "E", "ee"}, */
 {":", "E", Nothing, "@"},
 {Anything, "ER", Nothing, "3"},
 {Anything, "EV", "ER", "ev"},
/*  {"#:", "ER", "#", "3"}, */
 {Anything, "ER", "#", "er"},
 {Anything, "EW", Anything, "ev"},
 {Anything, "EI", Anything, "aI"},
 {Anything, "EU", Anything, "oI"},
 {Anything, "EH", Anything, "eee"},	/* ??? ge-habt <-> geh-abt */
 {Anything, "EE", Anything, "eee"},
 {Anything, "E", "^^", "e"},
 {Anything, "E", "$", "e"},
 {Anything, "EN", Nothing, "@n"},
 {Anything, "E", Anything, "ee"},
/*  {Anything, "E", Anything, "e"}, */
 {Anything, 0, Anything, Silent},
};

static Rule F_rules[] =
{
 {"F", "F", "F", "f f"},
/*  {Anything, "F", "F", ""}, */
 {"#", "FF", "#", "f"},
 {"#", "FF", Nothing, "f"},
 {Anything, "F", Anything, "f"},
 {Anything, 0, Anything, Silent},
};

static Rule G_rules[] =
{
 {Nothing, "GE", "E", "g@ "},     /*  geehrte */
 {Nothing, "GIN", Nothing, "dZin"},
 {Anything, "GREAT", Anything, "greIt"},
 {"#", "GG", "#", "g"},
 {Anything, "G", Anything, "g"},
 {Anything, 0, Anything, Silent},
};

static Rule H_rules[] =
{
 {Anything, "H", "#", "h"},
 {Anything, "H", Anything, ""},
 {Anything, 0, Anything, Silent},
};

static Rule I_rules[] =
{
 {Anything, "IE", Anything, "i"},
 {Anything, "IER", Anything, "i3"},
 {Anything, "IQUE", Anything, "ik"},
 {Anything, "I", "ß", "i"},
 {Anything, "I", "H", "i"},
 {Anything, "I", "$", "I"},
 {Anything, "I", Anything, "i"},
 {Anything, 0, Anything, Silent},
};

static Rule J_rules[] =
{
 {Anything, "J", Anything, "j"},
 {Anything, 0, Anything, Silent},
};

static Rule K_rules[] =
{
 {Nothing, "K", "N", ""},
 {Anything, "K", Anything, "k"},
 {Anything, 0, Anything, Silent},
};

static Rule L_rules[] =
{
 {"#", "LL", "#", "l"},
 {"#", "LL", Nothing, "l"},
 {Anything, "L", Anything, "l"},
 {Anything, 0, Anything, Silent},
};

static Rule M_rules[] =
{
 {"#", "MM", "#", "m"},
 {"#", "MM", Nothing, "m"},
 {Anything, "M", Anything, "m"},
 {Anything, 0, Anything, Silent},
};

static Rule N_rules[] =
{
 {"#", "NN", "#", "n"},
 {"#", "NN", Nothing, "n"},
 {Anything, "N", Anything, "n"},
 {Anything, 0, Anything, Silent},
};

static Rule O_rules[] =
{
 {Anything, "OE", Anything, "@@"},
 {Nothing, "OK", Nothing, "okei"},
 {Anything, "OY", Anything, "oI"},
 {Anything, "OI", Anything, "oI"},
 {"C", "O", "N", "0"},
 {Anything, "O", "$", "0"},
 {Anything, "O", "ß", "oo"},
 {Anything, "O", "H", "oo"},
 {Anything, "O", Anything, "o"},
 {Anything, 0, Anything, Silent},
};

static Rule P_rules[] =
{
 {"#", "PP", "#", "p"},
 {"#", "PP", Nothing, "p"},
 {Anything, "PH", Anything, "f"},
 {Anything, "PEOP", Anything, "pip"},
 {Anything, "POW", Anything, "paU"},
 {Anything, "PUT", Nothing, "pUt"},
 {Anything, "P", Anything, "p"},
 {Anything, 0, Anything, Silent},
};

static Rule Q_rules[] =
{
 {Anything, "QUAR", Anything, "kwar"},
 {Anything, "QU", Anything, "kw"},
 {Anything, "Q", Anything, "k"},
 {Anything, 0, Anything, Silent},
};

static Rule R_rules[] =
{
 {Nothing, "RE", "^#", "re"},
 {"#", "RR", "#", "r"},
 {"#", "RR", Nothing, "r"},
 {Anything, "R", Anything, "r"},
 {Anything, 0, Anything, Silent},
};

static Rule S_rules[] =
{
 {Anything, "SH", Anything, "S"},
 {"#", "SS", "#", "s"},
 {"#", "SS", Nothing, "s"},
/*  {Anything, "S", "S", ""}, */
 {".", "S", Nothing, "z"},
 {"#:.E", "S", Nothing, "z"},
 {"#:^##", "S", Nothing, "z"},
 {"#:^#", "S", Nothing, "s"},
 {"U", "S", Nothing, "s"},
 {" :#", "S", Nothing, "z"},
 {Anything, "SCH", Anything, "S"},
 {Anything, "S", Anything, "s"},
 {Anything, 0, Anything, Silent},
};

static Rule T_rules[] =
{
 {"#", "TT", "#", "t"},
 {"#", "TT", Nothing, "t"},
 {Nothing, "TWO", Anything, "tu"},
 {Anything, "TION", Nothing, "tsion"},
 {Anything, "T", Anything, "t"},
 {Anything, 0, Anything, Silent},
};

static Rule U_rules[] =
{
 {Nothing, "UN", "N", "un "},     /* un-nötig */
 {Anything, "U", "$", "U"},
 {Anything, "U", "H", "uu"},
 {Anything, "UE", Anything, "I"},
 {Anything, "U", Anything, "u"},
 {Anything, 0, Anything, Silent},
};

static Rule V_rules[] =
{
 {Anything, "VIEW", Anything, "vju"},
 {Nothing, "V", Anything, "f"},
 {Anything, "V", Anything, "v"},
 {Anything, 0, Anything, Silent},
};

static Rule W_rules[] =
{
 {Anything, "WHERE", Anything, "hwer"},
 {Anything, "WHAT", Anything, "hw0t"},
 {Anything, "WHO", Anything, "hu"},
 {Anything, "W", Anything, "v"},
 {Anything, "W", Nothing, "f"},   /* kiew */
 {Anything, 0, Anything, Silent},
};

static Rule X_rules[] =
{
 {Anything, "X", Anything, "ks"},
 {Anything, 0, Anything, Silent},
};

static Rule Y_rules[] =
{
 {Nothing, "YES", Anything, "jes"},
 {Nothing, "Y", Anything, "I"},
 {Anything, "Y", Anything, "I"},
 {Anything, 0, Anything, Silent},
};

static Rule Z_rules[] =
{
 {Anything, "Z", Anything, "ts"},
 {Anything, 0, Anything, Silent},
};

static Rule Uml_rules[] =         /* jms */
{
 {Anything, "ÄU", Anything, "oI"},
 {Anything, "ß", Anything, "s"},
 {Anything, "Ä", Anything, "ee"},
 {Anything, "Ö", Anything, "@@"},
 {Anything, "Ü", Anything, "I"},
 {Anything, 0, Anything, Silent},
};

static Rule *Rules[] =
{
 punct_rules,
 A_rules, B_rules, C_rules, D_rules, E_rules, F_rules, G_rules,
 H_rules, I_rules, J_rules, K_rules, L_rules, M_rules, N_rules,
 O_rules, P_rules, Q_rules, R_rules, S_rules, T_rules, U_rules,
 V_rules, W_rules, X_rules, Y_rules, Z_rules, Uml_rules
};

static int
vowel(int chr, int maybe)
{
 if (!isalpha(chr))
  return 0;
 switch (chr)
  {
   case 'A':
   case 'E':
   case 'I':
   case 'Ä':
   case 'Ö':
   case 'Ü':       
   case 'O':
   case 'U':
   case 'Y':
   case 'a':
   case 'e':
   case 'i':
   case 'ä':
   case 'ö':
   case 'ü':       
   case 'o':
   case 'u':
   case 'y':
    return 1;
  }
 return 0;
}

static int
consonant(int chr)
{
 return (isalpha(chr) && !vowel(chr,0));
}

static char *ASCII[] =
{
 "null", "", "", "",
 "", "", "", "",
 "", "", "", "",
 "", "", "", "",
 "", "", "", "",
 "", "", "", "",
 "", "", "", "",
 "", "", "", "",
 "blenk", "ausrufezeichen", "double quote", "hash",
 "dollar", "prozent", "ampersand", "quote",
 "klammer auf", "klammer zu", /*"asterisk" */ "mal", "plus",
 "comma", "minus", "punkt", "slash",
 "null", "eins", "zwei", "drei",
 "vier", "fünf", "sechs", "sieben",
 "acht", "neun", "doppelpunkt", "semicolon",
 "kleiner als", "gleich", "grösser als", "fragezeichen",
#ifndef ALPHA_IN_DICT
 "at",
 "a", "bee", "zee",
 "dee", "ee", "eff", "ge",
 "ha", "i", "jott", "ka",
 "ell", "em", "en", "oo",
 "pee", "kuh", "err", "es",
 "tee", "uh", "vau", "we",
 "iks", "ypsilon", "zed", "klammer auf",
#else                             /* ALPHA_IN_DICT */
 "at", "A", "B", "C",
 "D", "E", "F", "G",
 "H", "I", "J", "K",
 "L", "M", "N", "O",
 "P", "Q", "R", "S",
 "T", "U", "V", "W",
 "X", "Y", "Z", "klammer auf",
#endif                            /* ALPHA_IN_DICT */
 "back slash", "klammer zu", "zirkumflex", "unterstrich",
#ifndef ALPHA_IN_DICT
 "back quote",
 "a", "bee", "zee",
 "dee", "ee", "eff", "ge",
 "ha", "ii", "jott", "ka",
 "ell", "em", "en", "oo",
 "pee", "kuh", "err", "es",
 "tee", "uh", "vau", "we",
 "iks", "ypsilon", "zed", "klammer auf",
#else                             /* ALPHA_IN_DICT */
 "back quote", "A", "B", "C",
 "D", "E", "F", "G",
 "H", "I", "J", "K",
 "L", "M", "N", "O",
 "P", "Q", "R", "S",
 "T", "U", "V", "W",
 "X", "Y", "Z", "open brace",
#endif                            /* ALPHA_IN_DICT */
 "senkrechter strich", "klammer zu", "tilde", "löschen",
 NULL
};


/*
   **              Integer to Readable ASCII Conversion Routine.
   **
   ** Synopsis:
   **
   **      say_cardinal(value)
   **              long int     value;          -- The number to output
   **
   **      The number is translated into a string of words
   **
 */
static char *Cardinals[] =
{
 "null", "eins", "zwei", "drei",
 "vier", "fünf", "sechs", "sieben",
 "acht", "neun",
 "zehn", "elf", "zwölf", "dreizehn",
 "vierzehn", "fünfzehn", "sechszehn", "siebzehn",
 "achtzehn", "neunzehn"
};


static char *Twenties[] =
{
 "zwanzig", "dreissig", "vierzig", "fünfzig",
 "sechzig", "siebzig", "achtzig", "neunzig"
};


static char *Ordinals[] =
{
 "nullter", "erster", "zweiter", "dritter",
 "vierter", "fünfter", "sechster", "siebter",
 "achter", "neunter",
 "zehnter", "elfter", "zwölfter", "dreizehnter",
 "vierzehnter", "fünfzehnter", "sechszehnter", "siebzehnter",
 "achtzehnter", "neunzehnter"
};


static char *Ord_twenties[] =
{
 "zwanzigster", "dreissigster", "vierzigster", "fünfzigster",
 "sechzigster", "siebzigster", "achtzigster", "neunzigster"
};

/*
   ** Translate a number to phonemes.  This version is for CARDINAL numbers.
   **       Note: this is recursive.
 */
static unsigned
cardinal(long int value, darray_ptr phone)
{
 unsigned nph = 0;
 if (value < 0)
  {
   nph += xlate_string("minus", phone);
   value = (-value);
   if (value < 0)                 /* Overflow!  -32768 */
    {
     nph += xlate_string("zu viele", phone);
     return nph;
    }
  }
 if (value >= 1000000000L)
  /* Billions */
  {
   nph += cardinal(value / 1000000000L, phone);
   nph += xlate_string("milliarde", phone);	/* this is different !! jms */
   value = value % 1000000000;
   if (value == 0)
    return nph;                   /* Even billion */
   if (value < 100)
    nph += xlate_string("und", phone);
   /* as in THREE BILLION AND FIVE */
  }
 if (value >= 1000000L)
  /* Millions */
  {
   nph += cardinal(value / 1000000L, phone);
   nph += xlate_string("million", phone);
   value = value % 1000000L;
   if (value == 0)
    return nph;                   /* Even million */
   if (value < 100)
    nph += xlate_string("und", phone);
   /* as in THREE MILLION AND FIVE */
  }

 /* Thousands 1000..1099 2000..99999 */
 /* 1100 to 1999 is eleven-hunderd to ninteen-hunderd */
 if ((value >= 1000L && value <= 1099L) || value >= 2000L)
  {
   nph += cardinal(value / 1000L, phone);
   nph += xlate_string("tausend", phone);
   value = value % 1000L;
   if (value == 0)
    return nph;                   /* Even thousand */
   if (value < 100)
    nph += xlate_string("und", phone);
   /* as in THREE THOUSAND AND FIVE */
  }
 if (value >= 100L)
  {
   nph += xlate_string(Cardinals[value / 100], phone);
   nph += xlate_string("hundert", phone);
   value = value % 100;
   if (value == 0)
    return nph;                   /* Even hundred */
  }
 if ((value % 10) > 0 && value > 20)
  nph += xlate_string(Cardinals[value % 10], phone);	/* jms */
 if ((value % 10) > 0 && value > 20)
  nph += xlate_string("und", phone);	/* jms */
 if (value >= 20)
  {
   nph += xlate_string(Twenties[(value - 20) / 10], phone);
   /* value = value % 10; weg jms */
   /* if (value == 0)     weg jms */
   /*  return nph;        weg jms *//* Even ten */
  }
 if (value < 20)
  nph += xlate_string(Cardinals[value], phone);	/* jms */
 return nph;
}

/*
   ** Translate a number to phonemes.  This version is for ORDINAL numbers.
   **       Note: this is recursive.
 */
static unsigned
ordinal(long int value, darray_ptr phone)
{
 unsigned nph = 0;
 if (value < 0)
  {
   nph += xlate_string("minus", phone);
   value = (-value);
   if (value < 0)                 /* Overflow!  -32768 */
    {
     nph += xlate_string("zu viele", phone);
     return nph;
    }
  }
 if (value >= 1000000000L)
  /* Billions */
  {
   nph += cardinal(value / 1000000000L, phone);
   value = value % 1000000000;
   if (value == 0)
    {
     nph += xlate_string("milliardste", phone);
     return nph;                  /* Even billion */
    }
   nph += xlate_string("milliarde", phone);
   if (value < 100)
    nph += xlate_string("und", phone);
   /* as in THREE BILLION AND FIVE */
  }

 if (value >= 1000000L)
  /* Millions */
  {
   nph += cardinal(value / 1000000L, phone);
   value = value % 1000000L;
   if (value == 0)
    {
     nph += xlate_string("millionster", phone);
     return nph;                  /* Even million */
    }
   nph += xlate_string("million", phone);
   if (value < 100)
    nph += xlate_string("und", phone);
   /* as in THREE MILLION AND FIVE */
  }

 /* Thousands 1000..1099 2000..99999 */
 /* 1100 to 1999 is eleven-hunderd to ninteen-hunderd */
 if ((value >= 1000L && value <= 1099L) || value >= 2000L)
  {
   nph += cardinal(value / 1000L, phone);
   value = value % 1000L;
   if (value == 0)
    {
     nph += xlate_string("tausendster", phone);
     return nph;                  /* Even thousand */
    }
   nph += xlate_string("tausend", phone);
   if (value < 100)
    nph += xlate_string("und", phone);
   /* as in THREE THOUSAND AND FIVE */
  }
 if (value >= 100L)
  {
   nph += xlate_string(Cardinals[value / 100], phone);
   value = value % 100;
   if (value == 0)
    {
     nph += xlate_string("hundertster", phone);
     return nph;                  /* Even hundred */
    }
   nph += xlate_string("hundert", phone);
  }
 if (value >= 20)
  {
   if ((value % 10) == 0)
    {
     nph += xlate_string(Ord_twenties[(value - 20) / 10], phone);
     return nph;                  /* Even ten */
    }
   nph += xlate_string(Twenties[(value - 20) / 10], phone);
   value = value % 10;
  }
 nph += xlate_string(Ordinals[value], phone);
 return nph;
}

static Rule *
rules(int ch)
{
 int type = 0;
 if (isupper(ch) || ch == 'ß')
  type = ch - 'A' + 1;
 if (type > 'Z' - 'A' + 2)
  type = 'Z' - 'A' + 2;                    /* umlaute , jms */
 return Rules[type];
}

lang_t Deutsch =
{
 ASCII,
 "punkt",
 vowel,
 consonant,
 ordinal,
 cardinal,
 rules
};
