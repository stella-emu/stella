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

#import <Foundation/Foundation.h>

#include "MacOSUtils.hxx"

namespace MacOSUtils
{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string applicationSupportPath()
{
  @autoreleasepool {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory, NSUserDomainMask, YES);
    if(paths.count > 0)
    {
      const char* s = [paths[0] UTF8String];
      if(s) return std::string{s};
    }
  }
  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string desktopPath()
{
  @autoreleasepool {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSDesktopDirectory, NSUserDomainMask, YES);
    if(paths.count > 0)
    {
      const char* s = [paths[0] UTF8String];
      if(s) return std::string{s};
    }
  }
  return {};
}

}  // namespace MacOSUtils
