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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef LANCZOS_RESAMPLER_HXX
#define LANCZOS_RESAMPLER_HXX

#include "bspf.hxx"
#include "Resampler.hxx"
#include "ConvolutionBuffer.hxx"
#include "HighPass.hxx"

class LanczosResampler : public Resampler
{
  public:
    LanczosResampler(
      Resampler::Format formatFrom,
      Resampler::Format formatTo,
      Resampler::NextFragmentCallback nextFragmentCallback,
      uInt32 kernelParameter
    );

    void fillFragment(float* fragment, uInt32 length) override;

  private:

    void precomputeKernels();

    void shiftSamples(uInt32 samplesToShift);

  private:

    uInt32 myPrecomputedKernelCount;
    uInt32 myKernelSize;
    uInt32 myCurrentKernelIndex;
    unique_ptr<float[]> myPrecomputedKernels;

    uInt32 myKernelParameter;

    unique_ptr<ConvolutionBuffer> myBuffer;
    unique_ptr<ConvolutionBuffer> myBufferL;
    unique_ptr<ConvolutionBuffer> myBufferR;

    Int16* myCurrentFragment;
    uInt32 myFragmentIndex;
    bool myIsUnderrun;

    HighPass myHighPassL;
    HighPass myHighPassR;
    HighPass myHighPass;

    uInt32 myTimeIndex;
};

#endif // LANCZOS_RESAMPLER_HXX
