/*
    Derived from sources on comp.lang.speech belived to be public domain.    
    Changes Copyright (c) 1994,2001-2002 Nick Ing-Simmons. 
    All rights reserved.
 
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
#include "charset.h"
#include "english.h"
#include "phones.h"

char **dialect = ph_br;


/* $Id: english.c,v 1.1 2006-06-11 07:13:24 urchlay Exp $
 */
char *english_id = "$Id: english.c,v 1.1 2006-06-11 07:13:24 urchlay Exp $";

static unsigned cardinal(long int value, darray_ptr phone);
static unsigned ordinal(long int value, darray_ptr phone);
static int vowel(int chr, int maybe);
static int consonant(int chr);
static Rule *rules(int ch);

/*
   **      English to Phoneme rules.
   **
   **      Derived from:
   **
   **           AUTOMATIC TRANSLATION OF ENGLISH TEXT TO PHONETICS
   **                  BY MEANS OF LETTER-TO-SOUND RULES
   **
   **                      NRL Report 7948
   **
   **                    January 21st, 1976
   **          Naval Research Laboratory, Washington, D.C.
   **
   **
   **      Published by the National Technical Information Service as
   **      document "AD/A021 929".
   **
   **
   **
   **      The Phoneme codes:
   **
   **              IY      bEEt            IH      bIt
   **              EY      gAte            EH      gEt
   **              AE      fAt             AA      fAther
   **              AO      lAWn            OW      lOne
   **              UH      fUll            UW      fOOl
   **              ER      mURdER          AX      About
   **              AH      bUt             AY      hIde
   **              AW      hOW             OY      tOY
   **
   **              p       Pack            b       Back
   **              t       Time            d       Dime
   **              k       Coat            g       Goat
   **              f       Fault           v       Vault
   **              TH      eTHer           DH      eiTHer
   **              s       Sue             z       Zoo
   **              SH      leaSH           ZH      leiSure
   **              HH      How             m       suM
   **              n       suN             NG      suNG
   **              l       Laugh           w       Wear
   **              y       Young           r       Rate
   **              CH      CHar            j       Jar
   **              WH      WHere
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
   **              .       One of B, D, V, G, J, L, M, N, R, W or Z (voiced
   **                      consonants)
   **              %       One of ER, E, ES, ED, ING, ELY (a suffix)
   **                      (Found in right context only)
   **              +       One of E, I or Y (a "front" vowel)
   **
 */


/* Context definitions */
static char Anything[] = ""; /* No context requirement */

static char Nothing[] = " "; /* Context is beginning or end of word */

static char Silent[] = ""; /* No phonemes */


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
 {".", "'s", Anything, "z"},
 {"#:.e", "'s", Anything, "z"},
 {"#", "'s", Anything, "z"},
 {Anything, "'", Anything, ""},
 {Anything, ",", Anything, " "},
 {Anything, ".", Anything, " "},
 {Anything, "?", Anything, " "},
 {Anything, "!", Anything, " "},
 {Anything, 0, Anything, Silent},
};

static Rule A_rules[] =
{
 {Anything, "a", Nothing, "@"},
 {Nothing, "air", Anything, "e@"},
 {Nothing, "are", Nothing, "AR"},
 {Nothing, "a", "cc", "{"},
 {Nothing, "a", "cq", "{"},
 {Nothing, "acqu", Anything, "@kw"},
 {Nothing, "ar", "o", "@r"},
 {Anything, "ar", "#", "er"},
 {"^", "as", "#", "eIs"},
 {Anything, "a", "wa", "@"},
 {Anything, "aw", Anything, "O"},
 {" :", "any", Anything, "enI"},
 {Anything, "a", "^+#", "eI"},
 {"#:", "ally", Anything, "@lI"},
 {Nothing, "al", "#", "@l"},
 {Anything, "again", Anything, "@gen"},
 {"#:", "ag", "e", "IdZ"},
 {Anything, "a", "^+:#", "{"},
 {" :", "a", "^+ ", "eI"},
 {Anything, "a", "^%", "eI"},
 {Nothing, "arr", Anything, "@r"},
 {Anything, "arr", Anything, "Ar"}, 
 {" :", "ar", Nothing, "Qr"},
 {Anything, "ar", Nothing, "AR"},
 {Anything, "ar", "r", "A"},
 {Anything, "air", Anything, "er"},
 {Anything, "ai", Anything, "eI"},
 {Anything, "ays", Nothing, "Iz"},
 {Anything, "ay", Anything, "eI"},
 {Anything, "au", Anything, "O"},
 {"#:", "al", Nothing, "@l"},
 {"#:", "als", Nothing, "@lz"},
 {Anything, "al", "f", "A"},
 {Anything, "al", "k", "O"},
 {Anything, "al", "m", "{"},
 {Anything, "al", "^", "Ol"},
 {" :", "able", Anything, "eIb@l"},
 {Anything, "able", Anything, "@b@l"},
 {Anything, "ang", "+", "eIndZ"},
 {"^", "a", "^#", "eI"},
 {Anything, "a", "h", "A"},
 {Anything, "aa", Anything, "A"},
 {Anything, "a", Anything, "{"},
 {Anything, "à", Anything, "A"},
 {Anything, "â", Anything, "{"},
 {Anything, "ä", Anything, "A"},
 {Anything, 0, Anything, Silent},
};

static Rule B_rules[] =
{
 {Nothing, "beaut", Anything, "bjut"},
 {Nothing, "be", "^#", "bI"},
 {Anything, "being", Anything, "biIN"},
 {Nothing, "both", Nothing, "b@UT"},
 {Nothing, "bus", "#", "bIz"},
 {Anything, "buil", Anything, "bIl"},
 {"m", "b","=", ""},
 {Anything, "b", "t", ""},
 {Anything, "bb", Anything, "b"},
 {Anything, "b", Anything, "b"},
 {Anything, 0, Anything, Silent},
};

static Rule C_rules[] =
{
/*  {Anything, "ch", "^", "k"},     suspect */
 {"^e", "ch", Anything, "k"},
 {Anything, "ch", Anything, "tS"},
 {" s", "ci", "#", "saI"},
 {Anything, "ci", "a", "S"},
 {Anything, "ci", "o", "S"},
 {Anything, "ci", "en", "S"},
 {Anything, "c", "+", "s"},
 {Anything, "ckk", Anything, "k"},
 {Anything, "ck", Anything, "k"},
 {Anything, "com", "%", "kVm"},
 {Anything, "cc", "+", "ks"},
 {Anything, "cc", "#", "k"},
 {Anything, "cqu", Anything, "kw"},
 {Anything, "c", Anything, "k"},
 {Anything, "ç", Anything, "s"},
 {Anything, 0, Anything, Silent},
};

static Rule D_rules[] =
{
 {Anything, "dg", Anything, "dZ"},
 {Anything, "dj", Anything, "dZ"},
 {"#:", "ded", Nothing, "dId"},
 {".e", "d", Nothing, "d"},
 {"#:^e", "d", Nothing, "t"},
 {Nothing, "de", "^#", "dI"},         /* e is not long */
 {Nothing, "do", Nothing, "du"},
 {Nothing, "does", Anything, "dVz"},
 {Nothing, "doing", Anything, "duIN"},
 {Nothing, "dow", Anything, "daU"},
 {Anything, "du", "a", "dZu"},
 {Anything, "dd", Anything, "d"},
 {Anything, "d", Anything, "d"},
 {Anything, 0, Anything, Silent},
};

static Rule E_rules[] =
{
 {"#:", "e", Nothing, ""},
 {"':^", "e", Nothing, ""},
 {" :", "e", Nothing, "i"},
 {"#", "ed", Nothing, "d"},
 {"#:", "e", "d ", ""},    
 {Nothing, "e", "n^", "I"},
 {Anything, "ev", "er", "ev"},
 {Anything, "e", "^%", "i"},
 {Anything, "eri", "#", "iri"},
 {Anything, "eri", Anything, "erI"},
 {Anything, "ery", Anything, "@RI"},
 {"#:", "er", "#", "3"},
 {Anything, "er", "#", "er"},
 {Anything, "er", Nothing, "@R"},
 {Anything, "er", Anything, "3"},
 {Nothing, "even", Anything, "iven"},
 {"#:", "e", "w", ""},
 {Anything, "ew", Anything, "ju"},
 {Anything, "e", "o", "i"},
 {"#:s", "es", Nothing, "Iz"},
 {"#:c", "es", Nothing, "Iz"},
 {"#:g", "es", Nothing, "Iz"},
 {"#:z", "es", Nothing, "Iz"},
 {"#:x", "es", Nothing, "Iz"},
 {"#:j", "es", Nothing, "Iz"},
 {"#:ch", "es", Nothing, "Iz"},
 {"#:sh", "es", Nothing, "Iz"},
 {"#:", "e", "s ", ""},
 {"#:", "ely", Nothing, "li"},
 {"#:", "ement", Anything, "ment"},
 {Anything, "eful", Anything, "fUl"},
 {Anything, "ee", Anything, "i"},
 {Anything, "earn", Anything, "3n"},
 {Nothing, "ear", "^", "3"},
 {"#:", "ea", Nothing, "i@"},
 {Anything, "ea", "su", "e"},
 {Anything, "ea", "l", "I@"},
 {Anything, "eau", Anything, "@U"},
 {Anything, "ea", Anything, "i"},
 {Anything, "eigh", Anything, "eI"},
 {Anything, "ei", Anything, "i"},
 {Anything, "eye", Anything, "aI"},
 {Anything, "ey", Anything, "i"},
 {Anything, "eu", Anything, "ju"},
 {Anything, "e", Anything, "e"},
 {Anything, 0, Anything, Silent},
};

static Rule Ea_rules[] =
{
 {Anything, "éc", "c", "I"},         /* éclair */
 {Anything, "ém", "m", "e"},         /* émigré */
 {Anything, "é", Anything, "eI"},
 {Anything, "ê", Anything, "eI"},
 {Anything, 0, Anything, Silent},
};

static Rule F_rules[] =
{
 {Anything, "find", Anything, "faInd"},
 {Anything, "ful", Anything, "fUl"},
 {Anything, "ff", Anything, "f"},
 {Anything, "f", Anything, "f"},
 {Anything, 0, Anything, Silent},
};

static Rule G_rules[] =
{
 {Anything, "giv", Anything, "gIv"},
 {Nothing, "g", "i^", "g"},
 {Anything, "ge", "t", "ge"},
 {"su", "gges", Anything, "gdZes"},
 {Anything, "gg", Anything, "g"},
 {" b#", "g", Anything, "g"},
 {Anything, "g", "+", "dZ"},
 {Anything, "great", Anything, "greIt"},
 {"#", "gh", Anything, ""},
 {Anything, "gh", Anything, "g"},
 {Anything, "g", Anything, "g"},
 {Anything, 0, Anything, Silent},
};

static Rule H_rules[] =
{
 {Nothing, "hav", Anything, "h{v"},
 {Nothing, "heir", Anything, ""},
 {Nothing, "here", Anything, "hir"},
 {Nothing, "hour", Anything, "aU3"},
 {Anything, "how", Anything, "haU"},
 {Anything, "h", "#", "h"},
 {Anything, "h", Anything, ""},
 {Anything, 0, Anything, Silent},
};

static Rule I_rules[] =
{
 {Nothing, "iain", Nothing, "I@n"},
 {Nothing, "ing", Nothing, "IN"},
 {Nothing, "in", Anything, "In"},
 {Nothing, "i", Nothing, "aI"},
 {Anything, "i", "nd", "aI"}, 
 {Anything, "ier", Anything, "i@R"},
 {"#:r", "ied", Anything, "id"},
 {Anything, "ied", Nothing, "aId"},
 {Anything, "ies", Nothing, "Iz"},
 {Anything, "ian", Nothing, "@n"},
 {Anything, "ien", Anything, "ien"},
 {Anything, "ie", "t", "aIe"},
 {" :", "i", "%", "aI"},
 {Anything, "ie", "st", "II"},
 {Anything, "i", "%", "i"},
 {Anything, "ie", Anything, "i"},
 {Anything, "i", "^+:#", "I"},
 {Anything, "ir", "#", "aIr"},
 {Anything, "iz", "%", "aIz"},
 {Anything, "is", "%", "aIz"},
 {Anything, "i", "d%", "aI"},
 {"^", "i", "^e", "aI"},
 {"+^", "i", "^+", "I"},
 {Anything, "i", "t%", "aI"},
 {"#:^", "i", "^+", "I"},
 {Anything, "i", "^+", "aI"},
 {Anything, "ir", Anything, "3"},
 {Anything, "igh", Anything, "aI"},
 {Anything, "ild", Anything, "aIld"},
 {Anything, "ign", Nothing, "aIn"},
 {Anything, "ign", "^", "aIn"},
 {Anything, "ign", "%", "aIn"},
 {Anything, "ique", Anything, "ik"},
 {"^", "i", "^#", "aI"},
 {Anything, "i", Anything, "I"},
 {Anything, 0, Anything, Silent},
};

static Rule J_rules[] =
{
 {Nothing, "jew", Anything, "dZu"},
 {Anything, "j", Anything, "dZ"},
 {Anything, 0, Anything, Silent},
};

static Rule K_rules[] =
{
 {Nothing, "k", "n", ""},
 {Anything, "kk", Anything, "k"},
 {Anything, "k", Anything, "k"},
 {Anything, 0, Anything, Silent},
};

static Rule L_rules[] =
{
 {Anything, "lo", "c#", "l@U"},
 {"l", "l", Anything, ""},
 {"#:^", "l", "%", "@l"},
 {Anything, "lead", Anything, "lid"},
 {Anything, "l", Anything, "l"},
 {Anything, 0, Anything, Silent},
};

static Rule M_rules[] =
{
 {Anything, "mm", Anything, "m"},
 {Anything, "m", Anything, "m"},
 {Anything, 0, Anything, Silent},
};

static Rule N_rules[] =
{
 {"e", "ng", "+", "ndZ"},
 {Anything, "ng", "r", "Ng"},
 {Anything, "nge", Nothing, "ndZ"},
 {Anything, "ng", "#", "Ng"},
 {Anything, "ngl", "%", "Ng@l"},
 {Anything, "ng", Anything, "N"},
 {Anything, "nk", Anything, "Nk"},
 {Nothing, "now", Nothing, "naU"},
 {"#", "ng", Nothing, "Ng"},
 {Anything, "nn", Anything, "n"},
 {"m", "n", "=", ""},
 {"m", "n", "ed ", ""},
 {Anything, "n", Anything, "n"},
 {Anything, "ñ", Anything, "n"},
 {Anything, 0, Anything, Silent},
};

static Rule O_rules[] =
{
 {"m", "ov", Anything, "uv"},
 {Nothing, "of", Nothing, "Qv"},
 {Anything, "orough", Anything, "3@U"},
 {"#:", "or", Nothing, "3"},
 {"#:", "ors", Nothing, "3z"},
 {Anything, "or", "r", "Q"},
 {Nothing, "one", Anything, "wVn"},
 {Anything, "ow", Anything, "aU"},
 {Nothing, "over", Anything, "@Uv3"},
 {Anything, "ov", Anything, "Vv"},
 {Anything, "o", "^%", "@U"},
 {Anything, "o", "^en", "@U"},
 {Anything, "o", "^i#", "@U"},
 {Anything, "ol", "d", "@Ul"},
 {Anything, "ought", Anything, "Ot"},
 {Anything, "ough", Anything, "Vf"},
 {Nothing, "ou", Anything, "aU"},
 {Anything, "ou", "s#", "aU"},
 {Anything, "ou", "s", "@"},
 {Anything, "ou", "r", "O"},
 {Anything, "oul", "d", "U"},
 {"^", "ou", "^l", "V"},
 {Anything, "ou", "p", "u"},
 {Anything, "ou", Anything, "aU"},
 {Anything, "oy", Anything, "OI"},
 {Anything, "oing", Anything, "@UIN"},
 {Anything, "oi", Anything, "OI"},
 {Anything, "oo", "r", "O"},
 {Anything, "oo", "k", "U"},
 {Anything, "oo", "d", "U"},
 {Anything, "oo", Anything, "u"},
 {Anything, "o", "e", "@U"},
 {Anything, "o", Nothing, "@U"},
 {Anything, "oa", Anything, "@U"},
 {Nothing, "only", Anything, "@Unli"},
 {Nothing, "once", Anything, "wVns"},
 {Anything, "on't", Anything, "@Unt"},
 {"c", "o", "n", "Q"},
 {Anything, "o", "ng", "Q"},
 {" :^", "o", "n", "Q"},
 {"i", "o", "n", "@"},
 {"#:", "on", Nothing, "Qn"},
 {"#^", "o", "n", "@"},
 {Anything, "o", "st", "@U"},
 {Anything, "o", "ff", "Q"},
 {Anything, "of", "^", "Qf"},
 {Anything, "other", Anything, "VD@R"},
 {Anything, "o", "ss", "Q"},
 {"#:^", "o", "m", "Q"},
 {Anything, "omb", Anything, "Om"},
 {Anything, "o", Anything, "Q"},
 {Anything, "ö", Anything, "Q"},
 {Anything, 0, Anything, Silent},
};

static Rule P_rules[] =
{
 {Anything, "ph", Anything, "f"},
 {Anything, "peop", Anything, "pip"},
 {Anything, "pow", Anything, "paU"},
 {Anything, "put", Nothing, "pUt"},
 {Anything, "pp", Anything, "p"},
 {Nothing, "p", "n", ""},
 {Nothing, "p", "s", ""},
 {Nothing, "p", "t", ""},
 {Anything, "p", Anything, "p"},
 {Anything, 0, Anything, Silent},
};

static Rule Q_rules[] =
{
 {Anything, "qua", Anything, "kwQ"},
 {Anything, "que", Nothing, "k"},
 {Anything, "qu", Anything, "kw"},
 {Anything, "q", Anything, "k"},
 {Anything, 0, Anything, Silent},
};

static Rule R_rules[] =
{
 {Nothing, "re", "^#", "ri"},
 {Anything, "rr", Anything, "r"},
 {"#", "r", Nothing, "R"},
 {Anything, "r", Anything, "r"},
 {Anything, 0, Anything, Silent},
};

static Rule S_rules[] =
{
 {Anything, "sh", Anything, "S"},
 {"#", "sion", Anything, "Z@n"},
 {Anything, "some", Anything, "sVm"},
 {"#", "sur", "#", "Z3"},
 {Anything, "sur", "#", "S3"},
 {"#", "su", "#", "Zu"},
 {"#", "ssu", "#", "Su"},
 {"#", "sed", Nothing, "zd"},
 {"ou", "se", Nothing, "s"},
 {"#", "s", "#", "z"},
 {Anything, "said", Anything, "sed"},
 {"^", "sion", Anything, "S@n"},
 {Anything, "s", "s", ""},
 {"y", "s", Nothing, "z"},
#if 0
 {"l", "s", Anything, "s"},
 {"n", "s", Anything, "s"},
 {"r", "s", Anything, "s"},
#endif
 {".", "s", Nothing, "z"},
 {"#:.e", "s", Nothing, "z"},
 {"#:^##", "s", Nothing, "z"},
 {"#:^#", "s", Nothing, "s"}, 
 {"u", "s", Nothing, "z"},
 {" :#", "s", Nothing, "z"},
 {Nothing, "sch", Anything, "sk"},
 {Anything, "s", "c+", ""},
 {"#", "sm", Anything, "zm"},
 {"#", "sn", "'", "z@n"},
 {Anything, "s", Anything, "s"},
 {Anything, 0, Anything, Silent},
};

static Rule T_rules[] =
{
 {Nothing, "the", Nothing, "D@"},
 {Anything, "that", Nothing, "D{t"},
 {Nothing, "this", Nothing, "DIs"},
 {Nothing, "they", Anything, "DeI"},
 {Nothing, "there", Anything, "De@R"},
 {Anything, "ther", Anything, "D3"},
 {Anything, "their", Anything, "De@R"},
 {Nothing, "than", Nothing, "D{n"},
 {Nothing, "them", Nothing, "Dem"},
 {Anything, "these", Nothing, "Diz"},
 {Nothing, "then", Anything, "Den"},
 {Anything, "those", Anything, "D@Uz"},
 {Anything, "though", Nothing, "D@U"},
 {Nothing, "thus", Anything, "DVs"},

 {Anything, "through", Anything, "Tru"},
 {Anything, "th", Anything, "T"},

 {Nothing, "to", Nothing, "tu"},
 {Anything, "tch", Anything, "tS"},
 {"#:", "ted", Nothing, "tId"},
 {Anything, "ti", "o", "S"},
 {Anything, "ti", "a", "S"},
 {Anything, "tien", Anything, "S@n"},
 {Anything, "tur", "#", "tS3"},
 {Anything, "tu", "a", "tSu"},
 {Nothing, "two", Anything, "tu"},
 {Anything, "tt", Anything, "t"},
 {Anything, "t", Anything, "t"},
 {Anything, 0, Anything, Silent},
};

static Rule U_rules[] =
{
 {Nothing, "un", "i", "jun"},
 {Nothing, "un", Anything, "Vn"},
 {Nothing, "upon", Anything, "@pQn"},
 {"t", "ur", "#", "Ur"},
 {"s", "ur", "#", "Ur"},
 {"r", "ur", "#", "Ur"},
 {"d", "ur", "#", "Ur"},
 {"l", "ur", "#", "Ur"},
 {"z", "ur", "#", "Ur"},
 {"n", "ur", "#", "Ur"},
 {"j", "ur", "#", "Ur"},
 {"th", "ur", "#", "Ur"},
 {"ch", "ur", "#", "Ur"},
 {"sh", "ur", "#", "Ur"},
 {Anything, "ur", "#", "jUr"},
 {Anything, "ur", Anything, "3"},
 {Anything, "u", "^ ", "V"},
 {Anything, "u", "^^", "V"},
 {Anything, "uy", Anything, "aI"},
 {" g", "u", "#", ""},
 {"g", "u", "%", ""},
 {"g", "u", "#", "w"},
 {"#n", "u", Anything, "ju"},
 {"t", "u", Anything, "u"},
 {"s", "u", Anything, "u"},
 {"r", "u", Anything, "u"},
 {"d", "u", Anything, "u"},
 {"l", "u", Anything, "u"},
 {"z", "u", Anything, "u"},
 {"n", "u", Anything, "u"},
 {"j", "u", Anything, "u"},
 {"th", "u", Anything, "u"},
 {"ch", "u", Anything, "u"},
 {"sh", "u", Anything, "u"},
 {Anything, "u", Anything, "ju"},
 {Anything, 0, Anything, Silent},
};

static Rule V_rules[] =
{
 {Anything, "view", Anything, "vju"},
 {"v", "v", Anything, ""},
 {Anything, "v", Anything, "v"},
 {Anything, 0, Anything, Silent},
};

static Rule W_rules[] =
{
 {Nothing, "were", Anything, "w3R"},
 {Anything, "wa", "s", "wQ"},
 {Anything, "wa", "t", "wQ"},
 {Anything, "where", Anything, "we@"},
 {Anything, "what", Anything, "wQt"},
 {Anything, "with", Anything, "wID"},
 {Anything, "whol", Anything, "h@Ul"},
 {Anything, "who", Anything, "hu"},
 {Anything, "wh", Anything, "hw"},
 {Anything, "war", Anything, "wOr"},
 {Anything, "wor", "^", "w3"},
 {Anything, "wr", Anything, "r"},
 {Anything, "w", Anything, "w"},
 {Anything, 0, Anything, Silent},
};

static Rule X_rules[] =
{
 {"eau", "x", Nothing, "z"},
 {Nothing, "x", Anything, "z"},
 {Anything, "xc", Anything, "ks"},
 {Anything, "x", "ure ", "kS"},
 {Anything, "x", "ious ", "kS"},
 {Anything, "x", "ion ", "kS"},
 {Anything, "x", Anything, "ks"},
 {Anything, 0, Anything, Silent},
};

static Rule Y_rules[] =
{
 {Anything, "young", Anything, "jVN"},
 {Nothing, "you", Anything, "ju"},
 {Nothing, "yes", Anything, "jes"},
 {Nothing, "y", Anything, "j"},
 {"#:^", "y", Nothing, "I"},
 {"#:^", "y", "i", "i"},
 {" :", "y", Nothing, "aI"},
 {" :", "y", "#", "aI"},
 {" :", "y", "^+:#", "I"},
 {" :", "y", "^#", "aI"},
 {Anything, "y", Anything, "I"},
 {Anything, 0, Anything, Silent},
};

static Rule Z_rules[] =
{
 {Nothing, "zion", Anything, "zaI@n"},
 {Nothing, "zeal", "o", "zel"},
 {Anything, "zz", Anything, "z"},
 {"t", "z", Nothing, "s"},
 {Anything, "z", Anything, "z"},
 {Anything, 0, Anything, Silent},
};

static Rule *Rules[28] =
{
 punct_rules,
 A_rules, B_rules, C_rules, D_rules, E_rules, F_rules, G_rules,
 H_rules, I_rules, J_rules, K_rules, L_rules, M_rules, N_rules,
 O_rules, P_rules, Q_rules, R_rules, S_rules, T_rules, U_rules,
 V_rules, W_rules, X_rules, Y_rules, Z_rules
};

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
 "space", "exclamation mark", "double quote", "hash",
 "dollar", "percent", "ampersand", "quote",
 "open parenthesis", "close parenthesis", "asterisk", "plus",
 "comma", "minus", "full stop", "slash",
 "zero", "one", "two", "three",
 "four", "five", "six", "seven",
 "eight", "nine", "colon", "semi colon",
 "less than", "equals", "greater than", "question mark",
#ifndef ALPHA_IN_DICT
 "at", "ay", "bee", "see",
 "dee", "e", "eff", "gee",
 "aych", "i", "jay", "kay",
 "ell", "em", "en", "ohe",
 "pee", "kju", "are", "es",
 "tee", "you", "vee", "double you",
 "eks", "why", "zed", "open bracket",
#else                             /* ALPHA_IN_DICT */
 "at", "A", "B", "C",
 "D", "E", "F", "G",
 "H", "I", "J", "K",
 "L", "M", "N", "O",
 "P", "Q", "R", "S",
 "T", "U", "V", "W",
 "X", "Y", "Z", "open bracket",
#endif                            /* ALPHA_IN_DICT */
 "back slash", "close bracket", "circumflex", "underscore",
#ifndef ALPHA_IN_DICT
 "back quote", "ay", "bee", "see",
 "dee", "e", "eff", "gee",
 "aych", "i", "jay", "kay",
 "ell", "em", "en", "ohe",
 "pee", "kju", "are", "es",
 "tee", "you", "vee", "double you",
 "eks", "why", "zed", "open brace",
#else                             /* ALPHA_IN_DICT */
 "back quote", "A", "B", "C",
 "D", "E", "F", "G",
 "H", "I", "J", "K",
 "L", "M", "N", "O",
 "P", "Q", "R", "S",
 "T", "U", "V", "W",
 "X", "Y", "Z", "open brace",
#endif                            /* ALPHA_IN_DICT */
 "vertical bar", "close brace", "tilde", "delete",
 NULL
};

static int
vowel(int chr, int maybe)
{
 if (!isalpha(chr))
  return 0;
 switch(deaccent(chr))
  {
   case 'A':
   case 'E':
   case 'I':
   case 'O':
   case 'U':
   case 'a':
   case 'e':
   case 'i':
   case 'o':
   case 'u':
    return 1;
   case 'y':
   case 'Y':
    return maybe;
  }
 return 0;
}

static int
consonant(int chr)
{
 return (isalpha(chr) && !vowel(chr,0));
}

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
 "zero", "one", "two", "three",
 "four", "five", "six", "seven",
 "eight", "nine",
 "ten", "eleven", "twelve", "thirteen",
 "fourteen", "fifteen", "sixteen", "seventeen",
 "eighteen", "nineteen"
};


static char *Twenties[] =
{
 "twenty", "thirty", "forty", "fifty",
 "sixty", "seventy", "eighty", "ninety"
};


static char *Ordinals[] =
{
 "zeroth", "first", "second", "third",
 "fourth", "fifth", "sixth", "seventh",
 "eighth", "ninth",
 "tenth", "eleventh", "twelfth", "thirteenth",
 "fourteenth", "fifteenth", "sixteenth", "seventeenth",
 "eighteenth", "nineteenth"
};


static char *Ord_twenties[] =
{
 "twentieth", "thirtieth", "fortieth", "fiftieth",
 "sixtieth", "seventieth", "eightieth", "ninetieth"
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
     nph += xlate_string("a lot", phone);
     return nph;
    }
  }
 if (value >= 1000000000L)
  /* Billions */
  {
   nph += cardinal(value / 1000000000L, phone);
   nph += xlate_string("billion", phone);
   value = value % 1000000000;
   if (value == 0)
    return nph;                   /* Even billion */
   if (value < 100)
    nph += xlate_string("and", phone);
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
    nph += xlate_string("and", phone);
   /* as in THREE MILLION AND FIVE */
  }

 /* Thousands 1000..1099 2000..99999 */
 /* 1100 to 1999 is eleven-hunderd to ninteen-hunderd */
 if ((value >= 1000L && value <= 1099L) || value >= 2000L)
  {
   nph += cardinal(value / 1000L, phone);
   nph += xlate_string("thousand", phone);
   value = value % 1000L;
   if (value == 0)
    return nph;                   /* Even thousand */
   if (value < 100)
    nph += xlate_string("and", phone);
   /* as in THREE THOUSAND AND FIVE */
  }
 if (value >= 100L)
  {
   nph += xlate_string(Cardinals[value / 100], phone);
   nph += xlate_string("hundred", phone);
   value = value % 100;
   if (value == 0)
    return nph;                   /* Even hundred */
  }
 if (value >= 20)
  {
   nph += xlate_string(Twenties[(value - 20) / 10], phone);
   value = value % 10;
   if (value == 0)
    return nph;                   /* Even ten */
  }
 nph += xlate_string(Cardinals[value], phone);
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
     nph += xlate_string("a lot", phone);
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
     nph += xlate_string("billionth", phone);
     return nph;                  /* Even billion */
    }
   nph += xlate_string("billion", phone);
   if (value < 100)
    nph += xlate_string("and", phone);
   /* as in THREE BILLION AND FIVE */
  }

 if (value >= 1000000L)
  /* Millions */
  {
   nph += cardinal(value / 1000000L, phone);
   value = value % 1000000L;
   if (value == 0)
    {
     nph += xlate_string("millionth", phone);
     return nph;                  /* Even million */
    }
   nph += xlate_string("million", phone);
   if (value < 100)
    nph += xlate_string("and", phone);
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
     nph += xlate_string("thousandth", phone);
     return nph;                  /* Even thousand */
    }
   nph += xlate_string("thousand", phone);
   if (value < 100)
    nph += xlate_string("and", phone);
   /* as in THREE THOUSAND AND FIVE */
  }
 if (value >= 100L)
  {
   nph += xlate_string(Cardinals[value / 100], phone);
   value = value % 100;
   if (value == 0)
    {
     nph += xlate_string("hundredth", phone);
     return nph;                  /* Even hundred */
    }
   nph += xlate_string("hundred", phone);
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
 if (isalpha(ch))
  {
   if (isupper(ch))  
    ch = tolower(ch);
   type = ch - 'a' + 1;
   if (type > 'z' - 'a' + 1)
    {
     switch((char) ch)
      {
       case 'ç' : return C_rules;
       case 'ê' : 
       case 'é' : return Ea_rules;
       default  : return rules(deaccent(ch));
      }
    }
  }
 return Rules[type];
}

lang_t English =
{
 ASCII,
 "point",
 vowel,
 consonant,
 ordinal,
 cardinal,
 rules
};
