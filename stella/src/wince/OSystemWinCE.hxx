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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// Windows CE Port by Kostas Nakos
//============================================================================

#ifndef OSYSTEM_WINCE_HXX
#define OSYSTEM_WINCE_HXX

#include "bspf.hxx"
#include "OSystem.hxx"

class OSystemWinCE : public OSystem
{
  public:
    OSystemWinCE(const string& path);
    virtual ~OSystemWinCE();

  public:
    virtual void mainLoop();
	virtual uInt32 getTicks(void);
    virtual void setFramerate(uInt32 framerate);
	// ok, we lie a bit to Stephen here, but it's for a good purpose :)
	// we can always display these resolutions anyway
	virtual void getScreenDimensions(int& width, int& height) { width = 320; height = 240; };
	inline const GUI::Font& launcherFont() const;
};

#endif
