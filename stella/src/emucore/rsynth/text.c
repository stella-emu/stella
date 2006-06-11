/*
    Derived from sources on comp.lang.speech belived to be public domain.    
    Changes Copyright (c) 1994,2001-2002 Nick Ing-Simmons. 
 
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
/* $Id: text.c,v 1.1 2006-06-11 07:13:27 urchlay Exp $
   jms: added rule for duplicate consonant
   jms: changed isvowel function
 */
char *text_id = "$Id: text.c,v 1.1 2006-06-11 07:13:27 urchlay Exp $";
#include <stdio.h>
#include <ctype.h>
#include "useconfig.h"
#include "darray.h"
#include "phtoelm.h"
#include "lang.h"
#include "text.h"
#include "say.h"     

int rule_debug = 0;


#define FALSE (0)
#define TRUE (!0)

/*
   **      English to Phoneme translation.
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
   **                      (Right context only)
   **              +       One of E, I or Y (a "front" vowel)
   **              $       Duplicate consonant (for German)
   **              =       S or nothing (Right only) plurals etc.
 */



static int leftmatch(unsigned char *pattern, unsigned char *context);

static int
leftmatch(unsigned char *pattern, unsigned char *context)
                                           /* first char of pattern to match in text */
                                           /* last char of text to be matched */
{
 unsigned char *pat;
 unsigned char *text;
 int count;
 if (*pattern == '\0')
  /* null string matches any context */
  {
   return TRUE;
  }

 /* point to last character in pattern string */
 count = strlen((char *) pattern);
 pat = pattern + (count - 1);
 text = context;
 for (; count > 0; pat--, count--)
  {
   /* First check for simple text or space */
   if (isalpha(*pat & 0xFF) || *pat == '\'' || *pat == ' ')
    {
     if (*pat != *text)
      return FALSE;
     else
      {
       text--;
       continue;
      }
    }
   switch (*pat)
    {
     case '#':                   /* One or more vowels */
      if (!isvowel(*text,0))
       return FALSE;
      text--;
      while (isvowel(*text,0))
       text--;
      break;      

     case ':':                   /* Zero or more consonants */
      while (isconsonant(*text))
       text--;
      break;

     case '^':                   /* One consonant */
      if (!isconsonant(*text))
       return FALSE;
      text--;
      break;

     case '$':                   /* duplicate consonant jms */
      if (!isconsonant(*text))
       return FALSE;
      if (*text != *(text - 1))
       return FALSE;
      text--;
      text--;
      break;

     case '.':                   /* B, D, V, G, J, L, M, N, R, W, Z */
      if (*text != 'b' && *text != 'd' && *text != 'v'
          && *text != 'g' && *text != 'j' && *text != 'l'
          && *text != 'm' && *text != 'n' && *text != 'r'
          && *text != 'w' && *text != 'z')
       return FALSE;
      text--;
      break;

     case '+':                   /* E, I or Y (front vowel) */
      if (*text != 'e' && *text != 'i' && *text != 'y')
       return FALSE;
      text--;
      break;  

     case '%':
     default:
      fprintf(stderr, "Bad char in left rule: '%c'\n", *pat);
      return FALSE;
    }
  }
 return TRUE;
}

static int rightmatch(unsigned char *pattern, unsigned char *context);

static int
rightmatch(unsigned char *pattern, unsigned char *context)
                                           /* first char of pattern to match in text */
                                           /* last char of text to be matched */
{
 unsigned char *pat;
 unsigned char *text;
 if (*pattern == '\0')
  /* null string matches any context */
  return TRUE;
 pat = pattern;
 text = context;
 for (pat = pattern; *pat != '\0'; pat++)
  {
   /* First check for simple text or space */
   if (isalpha(*pat & 0xFF) || *pat == '\'' || *pat == ' ')
    {
     if (*pat != *text)
      return FALSE;
     else
      {
       text++;
       continue;
      }
    }
   switch (*pat)
    {
     case '#':                   /* One or more vowels */
      if (!isvowel(*text,0))
       return FALSE;
      text++;
      while (isvowel(*text,0))
       text++;
      break;

     case '=':                   /* Terminal, optional 's' */
      if (*text == 's' && text[1] == ' ')
       text++;
      if (*text != ' ')
       return FALSE;
      text++; 
      break;

     case ':':                   /* Zero or more consonants */
      while (isconsonant(*text))
       text++;
      break;

     case '^':                   /* One consonant */
      if (!isconsonant(*text))
       return FALSE;
      text++;
      break;

     case '$':                   /* duplicate consonant jms */
      if (!isconsonant(*text))
       return FALSE;
      if (*text != *(text + 1))
       return FALSE;
      text++;
      text++;
      break;

     case '.':                   /* B, D, V, G, J, L, M, N, R, W, Z */
      if (*text != 'b' && *text != 'd' && *text != 'v'
          && *text != 'g' && *text != 'j' && *text != 'l'
          && *text != 'm' && *text != 'n' && *text != 'r'
          && *text != 'w' && *text != 'z')
       return FALSE;
      text++;
      break;

     case '+':                   /* E, I or Y (front vowel) */
      if (*text != 'e' && *text != 'i' && *text != 'y')
       return FALSE;
      text++;
      break;

     case '%':                   /* ER, E, ES, ED, ING, ELY (a suffix) */
      if (*text == 'e')
       {
        text++;
        if (*text == 'l')
         {
          text++;
          if (*text == 'y')
           {
            text++;
            break;
           }
          else
           {
            text--;               /* Don't gobble L */
            break;
           }
         }
        else if (*text == 'r' || *text == 's' || *text == 'd')
         text++;
        break;
       }
      else if (*text == 'i')
       {
        text++;
        if (*text == 'n')
         {
          text++;
          if (*text == 'g')
           {
            text++;
            break;
           }
         }
        return FALSE;
       }
      else
       return FALSE;
     default:
      fprintf(stderr, "Bad char in right rule:'%c'\n", *pat);
      return FALSE;
    }
  }
 return TRUE;
}

static int find_rule(void *arg, out_p out, unsigned char *word, int index, Rule(*rules));

static int
find_rule(void *arg, out_p out, unsigned char *word, int index, Rule *rules)
{        
 if (rule_debug)
  printf("Looking for %c in %c rules\n",word[index],*rules->match);
 for (;;)                         /* Search for the rule */
  {
   Rule *rule = rules++;
   unsigned char *match = (unsigned char *) rule->match;
   int remainder;
   if (match == 0) /* bad symbol! */
    {
     if (rule_debug)
      fprintf(stderr, "Error: Can't find rule for: '%c' in \"%s\"\n",
             word[index], word);
     return index + 1;            /* Skip it! */
    }
   for (remainder = index; *match != '\0'; match++, remainder++)
    {
     if (*match != word[remainder])
      break;
    }
   if (*match != '\0')
    {
     continue;                     /* found missmatch */
    }

   if (!leftmatch((unsigned char *) rule->left, &word[index - 1]))
    continue;

   if (!rightmatch((unsigned char *) rule->right, &word[remainder]))
    continue;

   if (rule_debug)
    printf("...%s|%s|%s...=> %s succeded!\n", rule->left, rule->match, rule->right, rule->output); 

   (*out) (arg, rule->output);
   return remainder;
  }
}



void
guess_word(void *arg, out_p out, unsigned char *word)
{
 int index;                       /* Current position in word */
 index = 1;                       /* Skip the initial blank */
 do 
  {
   index = find_rule(arg, out, word, index, lang->rules(word[index]));
  }
 while (word[index] != '\0');
}


int
NRL(char *s, unsigned int n, darray_ptr phone)
{
 int old = phone->items;
 unsigned char *word = (unsigned char *) malloc(n + 3);
 unsigned char *d = word;
 *d++ = ' ';
 while (n-- > 0)
  {
   unsigned ch = *s++ & 0xFF;
   if (isupper(ch))
    ch = tolower(ch);
   *d++ = ch;
  }
 *d++ = ' ';
 *d = '\0';
 guess_word(phone, darray_cat, word);
 free(word);
 return phone->items - old;
}
