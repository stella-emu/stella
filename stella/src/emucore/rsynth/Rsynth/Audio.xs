/*
  Copyright (c) 1996, 2001 Nick Ing-Simmons. All rights reserved.
  This program is free software; you can redistribute it and/or
  modify it under the same terms as Perl itself.
*/
#define PERL_NO_GET_CONTEXT

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include "Audio.m"

AudioVtab *AudioVptr;

#include "rsynth.h"
#include "english.h"

lang_t *lang = &English;

static void *
Rsynth_sample(void *user_data,float sample,unsigned nsamp, rsynth_t *rsynth)
{
 dTHX;
 Audio *au = (Audio *) user_data;
 float *p  = Audio_more(aTHX_ au,1);
 /* FIXME - avoid this divide my adjusting gain on create */
 *p = sample/32768;
 return user_data;
}

static Elm_t *
SVtoElm(pTHX_ SV *sv)
{
 if (sv_isobject(sv) && SvTYPE(SvRV(sv)) == SVt_PVHV)
  {
   HV *hv = (HV *) SvRV(sv);
   SV **svp = hv_fetch(hv,"idx",3,0);
   if (svp)
    {
     Elm_t *elm = &Elements[SvIV(*svp)];
     return elm;
    }
  }
 croak("%_ is not an Element",sv);
 return NULL;
}

MODULE = Rsynth::Audio	PACKAGE = Rsynth::Element PREFIX = elm_

void
elm_update(SV *sv)
CODE:
 {
  if (sv_isobject(sv) && SvTYPE(SvRV(sv)) == SVt_PVHV)
   {
    HV *hv = (HV *) SvRV(sv);
    SV **svp = hv_fetch(hv,"idx",3,0);
    if (svp)
     {
      Elm_t *elm = &Elements[SvIV(*svp)];
      unsigned p;
      for (p = 0; p < nEparm; p++)
       {
        char *pname = Ep_name[p];
        svp = hv_fetch(hv,pname,strlen(pname),0);
        if (svp)
         {
          if (SvROK(*svp) && SvTYPE(SvRV(*svp)) == SVt_PVAV)
           {
            AV *av = (AV *) SvRV(*svp);
            if (av_len(av) != 4)
             {
              croak("%_->{%s} has %d values",sv,pname,av_len(av)+1);
             }
            svp = av_fetch(av, 0, 0);
            if (svp)
             {
              elm->p[p].stdy = (float) SvNV(*svp);
             }
            svp = av_fetch(av, 1, 0);
            if (svp)
             {
              elm->p[p].prop = (char) SvIV(*svp);
             }
            svp = av_fetch(av, 2, 0);
            if (svp)
             {
              elm->p[p].ed = (char) SvIV(*svp);
             }
            svp = av_fetch(av, 3, 0);
            if (svp)
             {
              elm->p[p].id = (char) SvIV(*svp);
             }
            svp = av_fetch(av, 4, 0);
            if (svp)
             {
              elm->p[p].rk = (char) SvIV(*svp);
             }
           }
          else
           {
            croak("%_->{%s} isn't an array",sv,pname);
           }
         }
       }
      XSRETURN(0);
     }
   }
  croak("%_ is not an Element",sv);
  XSRETURN(0);
 }


MODULE = Rsynth::Audio	PACKAGE = Rsynth::Audio PREFIX = rsynth_

PROTOTYPES: ENABLE

rsynth_t *
rsynth_new(char *Class, Audio *au, float F0Hz = 133.3, float ms_per_frame = 10.0, long gain = 57)
CODE:
 {
  speaker_t *speaker = rsynth_speaker(F0Hz, gain, Elements);
  RETVAL = rsynth_init(au->rate,ms_per_frame, speaker,Rsynth_sample,0,au);
 }
OUTPUT:
 RETVAL

void
rsynth_interpolate(rsynth_t *rsynth,SV *elm,SV *f0ref = 0)
CODE:
 {
  STRLEN len = 0;
  unsigned char *eptr = (unsigned char *) SvPV(elm,len);
  float *f0 = 0;
  unsigned nf0 = 0;
  rsynth_flush(rsynth,rsynth_interpolate(rsynth, eptr, len, f0, nf0));
  XSRETURN(0);
 }

void
rsynth_pho(rsynth_t *rsynth, char *file, int dodur = 1, char *phones = "sampa")

void
rsynth_phones(rsynth_t *rsynth, char *s, int len = strlen(s))

void
say_string(rsynth_t *rsynth, char *s)

void
rsynth_term(rsynth_t *rsynth)

BOOT:
 {
  AudioVptr = (AudioVtab *) SvIV(perl_get_sv("Audio::Data::AudioVtab",0));
 }
