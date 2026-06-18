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

#ifndef VERSION_HXX
#define VERSION_HXX

#include <string_view>

// The build number is normally generated from the current git commit count
// (see the 'VersionBuild.hxx' rule in the Makefile). When building outside of
// git (e.g. from a release tarball) or with a build system that doesn't
// generate it, the fallback value below is used instead.
#if __has_include("VersionBuild.hxx")
  #include "VersionBuild.hxx"
#endif
#ifndef STELLA_BUILD_INCLUDED
  static constexpr std::string_view STELLA_BUILD_NUMBER = "8005";
#endif

static constexpr std::string_view STELLA_FULL_TITLE = "Stella 8.0_pre";
static constexpr std::string_view STELLA_VERSION = "8.0_pre";
static constexpr std::string_view STELLA_BUILD = STELLA_BUILD_NUMBER;

#endif  // VERSION_HXX
