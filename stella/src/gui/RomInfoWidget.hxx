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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RomInfoWidget.hxx,v 1.4 2008-03-13 22:58:07 stephena Exp $
//============================================================================

#ifndef ROM_INFO_WIDGET_HXX
#define ROM_INFO_WIDGET_HXX

#include <fstream>

class GUI::Surface;

#include "Props.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "StringList.hxx"
#include "bspf.hxx"


class RomInfoWidget : public Widget
{
  public:
    RomInfoWidget(GuiObject *boss, const GUI::Font& font,
                  int x, int y, int w, int h);
    virtual ~RomInfoWidget();

    void setProperties(const Properties& props);
    void clearProperties();
    void initialize();
    void loadConfig();

  protected:
    void drawWidget(bool hilite);

  private:
    void parseProperties();
    static bool isValidPNGHeader(uInt8* header);
    static void readPNGChunk(ifstream& in, string& type, uInt8** data, int& size);
    static bool parseIHDR(int& width, int& height, uInt8* data, int size);
    static bool parseIDATChunk(const FrameBuffer& fb, GUI::Surface* surface,
                               int width, int height, uInt8* data, int size);
    static string parseTextChunk(uInt8* data, int size);

  private:
    // Surface holding the scaled PNG image
    GUI::Surface* mySurface;

    // Whether the surface should be redrawn by drawWidget()
    bool myDrawSurface;

    // Some ROM properties info, as well as 'tEXt' chunks from the PNG image
    StringList myRomInfo;

    // The properties for the currently selected ROM
    Properties myProperties;

    // Indicates if the current properties should actually be used
    bool myHaveProperties;

    // Indicates if an error occurred in creating/displaying the surface
    string mySurfaceErrorMsg;
};

#endif
