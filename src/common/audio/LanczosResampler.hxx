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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
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
      const Resampler::NextFragmentCallback& nextFragmentCallback,
      uInt32 kernelParameter
    );

    ~LanczosResampler() override = default;

    void fillFragment(float* fragment, uInt32 length) override;

  private:

    void precomputeKernels();

    void shiftSamples(uInt32 samplesToShift);

  private:

    uInt32 myPrecomputedKernelCount{0};
    uInt32 myKernelSize{0};
    uInt32 myKernelParameter{0};
    uInt32 myCurrentKernelIndex{0};
    uInt32 myFragmentIndex{0};
    uInt32 myTimeIndex{0};

    unique_ptr<float[]> myPrecomputedKernels;

    unique_ptr<ConvolutionBuffer> myBuffer;
    unique_ptr<ConvolutionBuffer> myBufferL;
    unique_ptr<ConvolutionBuffer> myBufferR;

    Int16* myCurrentFragment{nullptr};
    bool myIsUnderrun{true};

    HighPass myHighPassL;
    HighPass myHighPassR;
    HighPass myHighPass;

  private:
    LanczosResampler(const LanczosResampler&) = delete;
    LanczosResampler(LanczosResampler&&) = delete;
    LanczosResampler& operator=(const LanczosResampler&) = delete;
    LanczosResampler& operator=(LanczosResampler&&) = delete;
};

#endif  // LANCZOS_RESAMPLER_HXX
