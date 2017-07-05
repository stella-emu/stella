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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "AtariNTSC.hxx"

// blitter related
#ifndef restrict
  #if defined (__GNUC__)
    #define restrict __restrict__
  #elif defined (_MSC_VER) && _MSC_VER > 1300
    #define restrict __restrict
  #else
    /* no support for restricted pointers */
    #define restrict
  #endif
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariNTSC::initialize(const Setup& setup, const uInt8* palette)
{
  init(myImpl, setup);
  initializePalette(palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariNTSC::initializePalette(const uInt8* palette)
{
  // Palette stores R/G/B data for 'palette_size' entries
  for ( uInt32 entry = 0; entry < palette_size; ++entry )
  {
    float r = myImpl.to_float [*palette++];
    float g = myImpl.to_float [*palette++];
    float b = myImpl.to_float [*palette++];

    float y, i, q = RGB_TO_YIQ( r, g, b, y, i );

    // Generate kernel
    int ir, ig, ib = YIQ_TO_RGB( y, i, q, myImpl.to_rgb, int, ir, ig );
    uInt32 rgb = PACK_RGB( ir, ig, ib );

    uInt32* kernel = myColorTable[entry];
    genKernel(myImpl, y, i, q, kernel);

    for ( uInt32 i = 0; i < rgb_kernel_size / 2; i++ )
    {
      uInt32 error = rgb -
          kernel [i    ] - kernel [(i+10)%14+14] -
          kernel [i + 7] - kernel [i + 3    +14];
      kernel [i + 3 + 14] += error;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariNTSC::render(const uInt8* atari_in, uInt32 in_width,
                       uInt32 in_height, void* rgb_out, uInt32 out_pitch)
{
  uInt32 const chunk_count = (in_width - 1) / PIXEL_in_chunk;
  while ( in_height-- )
  {
    const uInt8* line_in = atari_in;
    ATARI_NTSC_BEGIN_ROW( NTSC_black, line_in[0] );
    uInt32* restrict line_out = static_cast<uInt32*>(rgb_out);
    ++line_in;

    for ( uInt32 n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, line_in[0] );
      ATARI_NTSC_RGB_OUT_8888( 0, line_out[0] );
      ATARI_NTSC_RGB_OUT_8888( 1, line_out[1] );
      ATARI_NTSC_RGB_OUT_8888( 2, line_out[2] );
      ATARI_NTSC_RGB_OUT_8888( 3, line_out[3] );

      ATARI_NTSC_COLOR_IN( 1, line_in[1] );
      ATARI_NTSC_RGB_OUT_8888( 4, line_out[4] );
      ATARI_NTSC_RGB_OUT_8888( 5, line_out[5] );
      ATARI_NTSC_RGB_OUT_8888( 6, line_out[6] );

      line_in  += 2;
      line_out += 7;
    }

    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, NTSC_black );
    ATARI_NTSC_RGB_OUT_8888( 0, line_out[0] );
    ATARI_NTSC_RGB_OUT_8888( 1, line_out[1] );
    ATARI_NTSC_RGB_OUT_8888( 2, line_out[2] );
    ATARI_NTSC_RGB_OUT_8888( 3, line_out[3] );

    ATARI_NTSC_COLOR_IN( 1, NTSC_black );
    ATARI_NTSC_RGB_OUT_8888( 4, line_out[4] );
    ATARI_NTSC_RGB_OUT_8888( 5, line_out[5] );
    ATARI_NTSC_RGB_OUT_8888( 6, line_out[6] );

    atari_in += in_width;
    rgb_out = static_cast<char*>(rgb_out) + out_pitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariNTSC::init(init_t& impl, const Setup& setup)
{
  impl.brightness = float(setup.brightness) * (0.5f * rgb_unit) + rgb_offset;
  impl.contrast   = float(setup.contrast)   * (0.5f * rgb_unit) + rgb_unit;

  impl.artifacts = float(setup.artifacts);
  if ( impl.artifacts > 0 )
    impl.artifacts *= artifacts_max - artifacts_mid;
  impl.artifacts = impl.artifacts * artifacts_mid + artifacts_mid;

  impl.fringing = float(setup.fringing);
  if ( impl.fringing > 0 )
    impl.fringing *= fringing_max - fringing_mid;
  impl.fringing = impl.fringing * fringing_mid + fringing_mid;

  initFilters(impl, setup);

  /* generate gamma table */
  if ( gamma_size > 1 )
  {
    float const to_float = 1.0f / (gamma_size - (gamma_size > 1));
    float const gamma = 1.1333f - float(setup.gamma) * 0.5f;
    /* match common PC's 2.2 gamma to TV's 2.65 gamma */
    int i;
    for ( i = 0; i < gamma_size; i++ )
      impl.to_float [i] =
          float(pow( i * to_float, gamma )) * impl.contrast + impl.brightness;
  }

  /* setup decoder matricies */
  {
    float hue = float(setup.hue) * PI + PI / 180 * ext_decoder_hue;
    float sat = float(setup.saturation) + 1;
    hue += PI / 180 * (std_decoder_hue - ext_decoder_hue);

    float s = float(sin( hue )) * sat;
    float c = float(cos( hue )) * sat;
    float* out = impl.to_rgb;
    int n;

    n = burst_count;
    do
    {
      float const* in = default_decoder;
      int n2 = 3;
      do
      {
        float i = *in++;
        float q = *in++;
        *out++ = i * c - q * s;
        *out++ = i * s + q * c;
      }
      while ( --n2 );
      if ( burst_count <= 1 )
        break;
      ROTATE_IQ( s, c, 0.866025f, -0.5f ); /* +120 degrees */
    }
    while ( --n );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariNTSC::initFilters(init_t& impl, const Setup& setup)
{
  float kernels [kernel_size * 2];

  /* generate luma (y) filter using sinc kernel */
  {
    /* sinc with rolloff (dsf) */
    float const rolloff = 1 + float(setup.sharpness) * 0.032;
    float const maxh = 32;
    float const pow_a_n = float(pow( rolloff, maxh ));
    float sum;
    int i;
    /* quadratic mapping to reduce negative (blurring) range */
    float to_angle = float(setup.resolution) + 1;
    to_angle = PI / maxh * float(LUMA_CUTOFF) * (to_angle * to_angle + 1);

    kernels [kernel_size * 3 / 2] = maxh; /* default center value */
    for ( i = 0; i < kernel_half * 2 + 1; i++ )
    {
      int x = i - kernel_half;
      float angle = x * to_angle;
      /* instability occurs at center point with rolloff very close to 1.0 */
      if ( x || pow_a_n > 1.056 || pow_a_n < 0.981 )
      {
        float rolloff_cos_a = rolloff * float(cos( angle ));
        float num = 1 - rolloff_cos_a -
            pow_a_n * float(cos( maxh * angle )) +
            pow_a_n * rolloff * float(cos( (maxh - 1) * angle ));
        float den = 1 - rolloff_cos_a - rolloff_cos_a + rolloff * rolloff;
        float dsf = num / den;
        kernels [kernel_size * 3 / 2 - kernel_half + i] = dsf - 0.5;
      }
    }

    /* apply blackman window and find sum */
    sum = 0;
    for ( i = 0; i < kernel_half * 2 + 1; i++ )
    {
      float x = PI * 2 / (kernel_half * 2) * i;
      float blackman = 0.42f - 0.5f * float(cos( x )) + 0.08f * float(cos( x * 2 ));
      sum += (kernels [kernel_size * 3 / 2 - kernel_half + i] *= blackman);
    }

    /* normalize kernel */
    sum = 1.0f / sum;
    for ( i = 0; i < kernel_half * 2 + 1; i++ )
    {
      int x = kernel_size * 3 / 2 - kernel_half + i;
      kernels [x] *= sum;
    }
  }

  /* generate chroma (iq) filter using gaussian kernel */
  {
    float const cutoff_factor = -0.03125f;
    float cutoff = float(setup.bleed);
    int i;

    if ( cutoff < 0 )
    {
      /* keep extreme value accessible only near upper end of scale (1.0) */
      cutoff *= cutoff;
      cutoff *= cutoff;
      cutoff *= cutoff;
      cutoff *= -30.0f / 0.65f;
    }
    cutoff = cutoff_factor - 0.65f * cutoff_factor * cutoff;

    for ( i = -kernel_half; i <= kernel_half; i++ )
      kernels [kernel_size / 2 + i] = float(exp( i * i * cutoff ));

    /* normalize even and odd phases separately */
    for ( i = 0; i < 2; i++ )
    {
      float sum = 0;
      int x;
      for ( x = i; x < kernel_size; x += 2 )
        sum += kernels [x];

      sum = 1.0f / sum;
      for ( x = i; x < kernel_size; x += 2 )
      {
        kernels [x] *= sum;
      }
    }
  }

  /* generate linear rescale kernels */
  float weight = 1.0f;
  float* out = impl.kernel;
  int n = rescale_out;
  do
  {
    float remain = 0;
    int i;
    weight -= 1.0f / rescale_in;
    for ( i = 0; i < kernel_size * 2; i++ )
    {
      float cur = kernels [i];
      float m = cur * weight;
      *out++ = m + remain;
      remain = cur - m;
    }
  }
  while ( --n );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Generate pixel at all burst phases and column alignments
void AtariNTSC::genKernel(init_t& impl, float y, float i, float q, uInt32* out)
{
  /* generate for each scanline burst phase */
  float const* to_rgb = impl.to_rgb;
  int burst_remain = burst_count;
  y -= rgb_offset;
  do
  {
    /* Encode yiq into *two* composite signals (to allow control over artifacting).
    Convolve these with kernels which: filter respective components, apply
    sharpening, and rescale horizontally. Convert resulting yiq to rgb and pack
    into integer. Based on algorithm by NewRisingSun. */
    pixel_info_t const* pixel = atari_ntsc_pixels;
    int alignment_remain = alignment_count;
    do
    {
      /* negate is -1 when composite starts at odd multiple of 2 */
      float const yy = y * impl.fringing * pixel->negate;
      float const ic0 = (i + yy) * pixel->kernel [0];
      float const qc1 = (q + yy) * pixel->kernel [1];
      float const ic2 = (i - yy) * pixel->kernel [2];
      float const qc3 = (q - yy) * pixel->kernel [3];

      float const factor = impl.artifacts * pixel->negate;
      float const ii = i * factor;
      float const yc0 = (y + ii) * pixel->kernel [0];
      float const yc2 = (y - ii) * pixel->kernel [2];

      float const qq = q * factor;
      float const yc1 = (y + qq) * pixel->kernel [1];
      float const yc3 = (y - qq) * pixel->kernel [3];

      float const* k = &impl.kernel [pixel->offset];
      int n;
      ++pixel;
      for ( n = rgb_kernel_size; n; --n )
      {
        float fi = k[0]*ic0 + k[2]*ic2;
        float fq = k[1]*qc1 + k[3]*qc3;
        float fy = k[kernel_size+0]*yc0 + k[kernel_size+1]*yc1 +
                  k[kernel_size+2]*yc2 + k[kernel_size+3]*yc3 + rgb_offset;
        if ( k < &impl.kernel [kernel_size * 2 * (rescale_out - 1)] )
          k += kernel_size * 2 - 1;
        else
          k -= kernel_size * 2 * (rescale_out - 1) + 2;
        {
          int r, g, b = YIQ_TO_RGB( fy, fi, fq, to_rgb, int, r, g );
          *out++ = PACK_RGB( r, g, b ) - rgb_bias;
        }
      }
    }
    while ( alignment_count > 1 && --alignment_remain );
  }
  while ( --burst_remain );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AtariNTSC::Setup AtariNTSC::TV_Composite = {
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.15, 0.0, 0.0, 0.0
};
const AtariNTSC::Setup AtariNTSC::TV_SVideo = {
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.45, -1.0, -1.0, 0.0
};
const AtariNTSC::Setup AtariNTSC::TV_RGB = {
  0.0, 0.0, 0.0, 0.0, 0.2, 0.0, 0.70, -1.0, -1.0, -1.0
};
const AtariNTSC::Setup AtariNTSC::TV_Bad = {
  0.1, -0.3, 0.3, 0.25, 0.2, 0.0, 0.1, 0.5, 0.5, 0.5
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AtariNTSC::pixel_info_t AtariNTSC::atari_ntsc_pixels[alignment_count] = {
  { PIXEL_OFFSET( -4, -9 ), { 1, 1, 1, 1            } },
  { PIXEL_OFFSET(  0, -5 ), {            1, 1, 1, 1 } },
};

const float AtariNTSC::default_decoder[6] = {
  0.9563f, 0.6210f, -0.2721f, -0.6474f, -1.1070f, 1.7046f
};
