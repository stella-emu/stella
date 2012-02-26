//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "NTSCFilter.hxx"

#include <cstring>
#include <cmath>
#include <cstdlib>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NTSCFilter::NTSCFilter()
  : mySetup(atari_ntsc_composite),
    myCurrentModeNum(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NTSCFilter::~NTSCFilter()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setTIAPalette(const uInt32* palette)
{
  // The TIA palette consists of 128 colours, but the palette array actually
  // contains 256 entries, where only every second value is a valid colour
  uInt8* ptr = myTIAPalette;
  for(int i = 0; i < 256; i+=2)
  {
    *ptr++ = (palette[i] >> 16) & 0xff;
    *ptr++ = (palette[i] >> 8) & 0xff;
    *ptr++ = palette[i] & 0xff;
  }
  updateFilter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::updateFilter()
{
  double yiq_table[384];

  updateYIQTable(yiq_table, mySetup.burst_phase * M_PI);

  /* The gamma setting is not used in atari_ntsc (palette generation is
     placed in another module), so below we do not set the gamma field
     of the mySetup structure. */

  // According to how Atari800 defines 'external', we're using an external
  // palette (since it is generated outside of the COLOUR_NTSC routines)
  /* External palette must not be adjusted, so FILTER_NTSC
     settings are set to defaults so they don't change the source
     palette in any way. */
  mySetup.hue = 0.0;
  mySetup.saturation = 0.0;
  mySetup.contrast = 0.0;
  mySetup.brightness = 0.0;

  mySetup.yiq_palette = yiq_table;
  atari_ntsc_init(&myFilter, &mySetup);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::restoreDefaults()
{
  mySetup = atari_ntsc_composite;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setPreset(int preset)
{
  if (preset < PRESET_CUSTOM) {
    mySetup = *presets[preset];
    updateFilter();

#if 0 // FIXME - what are these items for??
    /* Copy settings from the preset to NTSC setup. */
    COLOURS_NTSC_specific_setup.hue = mySetup.hue;
    COLOURS_NTSC_setup.saturation = mySetup.saturation;
    COLOURS_NTSC_setup.contrast = mySetup.contrast;
    COLOURS_NTSC_setup.brightness = mySetup.brightness;
    COLOURS_NTSC_setup.gamma = mySetup.gamma;
#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int NTSCFilter::getPreset()
{
// FIXME - for now just return composite
return 0;
#if 0
  int i;

  for (i = 0; i < PRESET_SIZE; i ++) {
    if (Util_almostequal(mySetup.sharpness, presets[i]->sharpness, 0.001) &&
        Util_almostequal(mySetup.resolution, presets[i]->resolution, 0.001) &&
        Util_almostequal(mySetup.artifacts, presets[i]->artifacts, 0.001) &&
        Util_almostequal(mySetup.fringing, presets[i]->fringing, 0.001) &&
        Util_almostequal(mySetup.bleed, presets[i]->bleed, 0.001) &&
        Util_almostequal(mySetup.burst_phase, presets[i]->burst_phase, 0.001) &&
        Util_almostequal(COLOURS_NTSC_specific_setup.hue, presets[i]->hue, 0.001) &&
        Util_almostequal(COLOURS_NTSC_setup.saturation, presets[i]->saturation, 0.001) &&
        Util_almostequal(COLOURS_NTSC_setup.contrast, presets[i]->contrast, 0.001) &&
        Util_almostequal(COLOURS_NTSC_setup.brightness, presets[i]->brightness, 0.001) &&
        Util_almostequal(COLOURS_NTSC_setup.gamma, presets[i]->gamma, 0.001))
      return i; 
  }
  return PRESET_CUSTOM;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::nextPreset()
{
  int preset = getPreset();
  
  if (preset == PRESET_CUSTOM)
    preset = PRESET_COMPOSITE;
  else
    preset = (preset + 1) % PRESET_SIZE;
  setPreset(preset);
}

#if 0 // FIXME
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int NTSCFilter::FILTER_NTSC_ReadConfig(char *option, char *ptr)
{
  if (strcmp(option, "FILTER_NTSC_SHARPNESS") == 0)
    return Util_sscandouble(ptr, &mySetup.sharpness);
  else if (strcmp(option, "FILTER_NTSC_RESOLUTION") == 0)
    return Util_sscandouble(ptr, &mySetup.resolution);
  else if (strcmp(option, "FILTER_NTSC_ARTIFACTS") == 0)
    return Util_sscandouble(ptr, &mySetup.artifacts);
  else if (strcmp(option, "FILTER_NTSC_FRINGING") == 0)
    return Util_sscandouble(ptr, &mySetup.fringing);
  else if (strcmp(option, "FILTER_NTSC_BLEED") == 0)
    return Util_sscandouble(ptr, &mySetup.bleed);
  else if (strcmp(option, "FILTER_NTSC_BURST_PHASE") == 0)
    return Util_sscandouble(ptr, &mySetup.burst_phase);
  else
    return FALSE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::FILTER_NTSC_WriteConfig(FILE *fp)
{
  fprintf(fp, "FILTER_NTSC_SHARPNESS=%g\n", mySetup.sharpness);
  fprintf(fp, "FILTER_NTSC_RESOLUTION=%g\n", mySetup.resolution);
  fprintf(fp, "FILTER_NTSC_ARTIFACTS=%g\n", mySetup.artifacts);
  fprintf(fp, "FILTER_NTSC_FRINGING=%g\n", mySetup.fringing);
  fprintf(fp, "FILTER_NTSC_BLEED=%g\n", mySetup.bleed);
  fprintf(fp, "FILTER_NTSC_BURST_PHASE=%g\n", mySetup.burst_phase);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int NTSCFilter::FILTER_NTSC_Initialise(int *argc, char *argv[])
{
  int i;
  int j;

  for (i = j = 1; i < *argc; i++) {
    int i_a = (i + 1 < *argc);    /* is argument available? */
    int a_m = FALSE;      /* error, argument missing! */
    
    if (strcmp(argv[i], "-ntsc-sharpness") == 0) {
      if (i_a)
        mySetup.sharpness = atof(argv[++i]);
      else a_m = TRUE;
    }
    else if (strcmp(argv[i], "-ntsc-resolution") == 0) {
      if (i_a)
        mySetup.resolution = atof(argv[++i]);
      else a_m = TRUE;
    }
    else if (strcmp(argv[i], "-ntsc-artifacts") == 0) {
      if (i_a)
        mySetup.artifacts = atof(argv[++i]);
      else a_m = TRUE;
    }
    else if (strcmp(argv[i], "-ntsc-fringing") == 0) {
      if (i_a)
        mySetup.fringing = atof(argv[++i]);
      else a_m = TRUE;
    }
    else if (strcmp(argv[i], "-ntsc-bleed") == 0) {
      if (i_a)
        mySetup.bleed = atof(argv[++i]);
      else a_m = TRUE;
    }
    else if (strcmp(argv[i], "-ntsc-burstphase") == 0) {
      if (i_a)
        mySetup.burst_phase = atof(argv[++i]);
      else a_m = TRUE;
    }
    else if (strcmp(argv[i], "-ntsc-filter-preset") == 0) {
      if (i_a) {
        int idx = CFG_MatchTextParameter(argv[++i], preset_cfg_strings, FILTER_NTSC_PRESET_SIZE);
        if (idx < 0) {
          Log_print("Invalid value for -ntsc-filter-preset");
          return FALSE;
        }
        setPreset(idx);
      } else a_m = TRUE;
    }
    else {
      if (strcmp(argv[i], "-help") == 0) {
        Log_print("\t-ntsc-sharpness <n>   Set sharpness for NTSC filter (default %.2g)", mySetup.sharpness);
        Log_print("\t-ntsc-resolution <n>  Set resolution for NTSC filter (default %.2g)", mySetup.resolution);
        Log_print("\t-ntsc-artifacts <n>   Set luma artifacts ratio for NTSC filter (default %.2g)", mySetup.artifacts);
        Log_print("\t-ntsc-fringing <n>    Set chroma fringing ratio for NTSC filter (default %.2g)", mySetup.fringing);
        Log_print("\t-ntsc-bleed <n>       Set bleed for NTSC filter (default %.2g)", mySetup.bleed);
        Log_print("\t-ntsc-burstphase <n>  Set burst phase (artifact colours) for NTSC filter (default %.2g)", mySetup.burst_phase);
        Log_print("\t-ntsc-filter-preset composite|svideo|rgb|monochrome");
        Log_print("\t                      Use one of predefined NTSC filter adjustments");
      }
      argv[j++] = argv[i];
    }

    if (a_m) {
      Log_print("Missing argument for '%s'", argv[i]);
      return FALSE;
    }
  }
  *argc = j;

  return TRUE;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::updateYIQTable(double yiq_table[384], double start_angle)
{
  const double start_saturation = 0.0; // calculated internally
	const double gamma = 1; // 1 - COLOURS_NTSC_setup.gamma / 2.0;
  uInt8* ext_ptr = myTIAPalette;
	int n;

	start_angle = - ((213.0f) * M_PI / 180.0f) - start_angle;

	for (n = 0; n < 128; n ++) {
		/* Convert RGB values from external palette to YIQ. */
		double r = (double)*ext_ptr++ / 255.0;
		double g = (double)*ext_ptr++ / 255.0;
		double b = (double)*ext_ptr++ / 255.0;
		double y = 0.299 * r + 0.587 * g + 0.114 * b;
		double i = 0.595716 * r - 0.274453 * g - 0.321263 * b;
		double q = 0.211456 * r - 0.522591 * g + 0.311135 * b;
		double s = sin(start_angle);
		double c = cos(start_angle);
		double tmp_i = i;
		i = tmp_i * c - q * s;
		q = tmp_i * s + q * c;
#if 0
		/* Optionally adjust external palette. */
		if (COLOURS_NTSC_external.adjust) {
			y = pow(y, gamma);
			y *= COLOURS_NTSC_setup.contrast * 0.5 + 1;
			y += COLOURS_NTSC_setup.brightness * 0.5;
			if (y > 1.0)
				y = 1.0;
			else if (y < 0.0)
				y = 0.0;
			i *= start_saturation + 1;
			q *= start_saturation + 1;
		}
#endif
		*yiq_table++ = y;
		*yiq_table++ = i;
		*yiq_table++ = q;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
atari_ntsc_setup_t const * const NTSCFilter::presets[NTSCFilter::PRESET_SIZE] = {
  &atari_ntsc_composite,
  &atari_ntsc_svideo,
  &atari_ntsc_rgb,
  &atari_ntsc_monochrome
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static char const * const preset_cfg_strings[NTSCFilter::PRESET_SIZE] = {
  "COMPOSITE",
  "SVIDEO",
  "RGB",
  "MONOCHROME"
};
