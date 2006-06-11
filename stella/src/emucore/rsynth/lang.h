#ifndef LANG_H 
#define LANG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "darray.h"

extern char **dialect;

/* Rule is an array of 4 character pointers */
typedef struct rule_s
 {
  char *left;
  char *match;
  char *right;
  char *output;
 } Rule;

typedef struct lang_s 
 {
  char **char_name;
  char *point;
  int  (*vowel)(int chr,int maybe);
  int  (*consonant)(int chr);
  unsigned (*ordinal)(long int value,darray_ptr phone);
  unsigned (*cardinal)(long int value,darray_ptr phone);
  Rule *(*rules)(int ch);
  unsigned char *(*lookup)(const char *s, unsigned n);
 } lang_t;


extern unsigned char *dict_find(const char *s, unsigned n);

/* FIXME - the following was the original code
           I guess it needs to be defined in SpeakJet ??
           For now, it's defined in english.h
  extern lang_t *lang;
*/
static lang_t *lang = 0;


#define isvowel(c,m) (*lang->vowel)(c,m)
#define isconsonant(c) (*lang->consonant)(c)
#define xlate_ordinal(v,p) (*lang->ordinal)(v,p)
#define xlate_cardinal(v,p) (*lang->cardinal)(v,p)
#define dict_find(s,n) ((lang->lookup) ? (*lang->lookup)(s,n) : 0)
#define have_dict (lang->lookup != 0)

#ifdef __cplusplus
}
#endif

#endif /* LANG_H */
