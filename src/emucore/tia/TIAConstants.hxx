#ifndef TIA_CONSTANTS_HXX
#define TIA_CONSTANTS_HXX

#include "bspf.hxx"

namespace TIAConstants {

  constexpr uInt32 frameBufferHeight = 320;
  constexpr uInt32 minYStart = 1, maxYStart = 64;
  constexpr uInt32 minViewableHeight = 210, maxViewableHeight = 256;
  constexpr uInt32 initialGarbageFrames = 10;

}

#endif // TIA_CONSTANTS_HXX