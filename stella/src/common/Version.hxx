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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Version.hxx,v 1.22 2007-01-01 18:04:40 stephena Exp $
//============================================================================

#ifndef VERSION_HXX
#define VERSION_HXX

#define STELLA_BASE_VERSION "2.3.01"

#ifdef NIGHTLY_BUILD
#define STELLA_VERSION STELLA_BASE_VERSION "pre-" NIGHTLY_BUILD
#else
#define STELLA_VERSION STELLA_BASE_VERSION
#endif

// Specifies the minimum version of the settings file that's valid
// for this version of Stella.  If the settings file is too old,
// the internal defaults are used.
// For each new release, this should only be bumped if there have been
// major changes in some settings; changes which could stop Stella from
// actually working.
#define STELLA_SETTINGS_VERSION "2.3"

#endif
