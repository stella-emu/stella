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

#ifndef CONVOLUTION_BUFFER_HXX
#define CONVOLUTION_BUFFER_HXX

#include "bspf.hxx"

class ConvolutionBuffer
{
  public:
    // The buffer is mirrored: each value is stored twice, mySize apart, so
    // the convolution can always read a contiguous window without wrapping.
    explicit ConvolutionBuffer(uInt32 size)
      : myData{std::make_unique<float[]>(static_cast<size_t>(size) * 2)},
        mySize{size} { }

    ~ConvolutionBuffer() = default;

    void shift(float nextValue) {
      myData[myFirstIndex] = myData[myFirstIndex + mySize] = nextValue;
      if(++myFirstIndex == mySize) myFirstIndex = 0;
    }

    float convoluteWith(const float* kernel) const {
      const float* data = myData.get() + myFirstIndex;
      float result = 0.F;

      for(uInt32 i = 0; i < mySize; ++i)
        result += kernel[i] * data[i];

      return result;
    }

  private:
    unique_ptr<float[]> myData;
    uInt32 myFirstIndex{0};
    uInt32 mySize{0};

  private:
    ConvolutionBuffer() = delete;
    ConvolutionBuffer(const ConvolutionBuffer&) = delete;
    ConvolutionBuffer(ConvolutionBuffer&&) = delete;
    ConvolutionBuffer& operator=(const ConvolutionBuffer&) = delete;
    ConvolutionBuffer& operator=(ConvolutionBuffer&&) = delete;
};

#endif  // CONVOLUTION_BUFFER_HXX
