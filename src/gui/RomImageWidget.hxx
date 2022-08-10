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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef ROM_IMAGE_WIDGET_HXX
#define ROM_IMAGE_WIDGET_HXX

class FBSurface;
class Properties;

#include "Widget.hxx"
#include "bspf.hxx"

class RomImageWidget : public Widget, public CommandSender
{
  public:
    RomImageWidget(GuiObject *boss, const GUI::Font& font,
                  int x, int y, int w, int h);
    ~RomImageWidget() override = default;

    void setProperties(const FSNode& node, const string& md5);
    void clearProperties();
    void reloadProperties(const FSNode& node);

  protected:
    void drawWidget(bool hilite) override;
#ifdef PNG_SUPPORT
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseMoved(int x, int y) override;
#endif

  private:
    void parseProperties(const FSNode& node);
  #ifdef PNG_SUPPORT
    bool getImageList(const string& filename);
    bool loadPng(const string& filename);
  #endif

  private:
    // Surface pointer holding the PNG image
    shared_ptr<FBSurface> mySurface;

    // Whether the surface should be redrawn by drawWidget()
    bool mySurfaceIsValid{false};

    // The properties for the currently selected ROM
    Properties myProperties;

    // Indicates if the current properties should actually be used
    bool myHaveProperties{false};

    // Indicates if an error occurred in creating/displaying the surface
    string mySurfaceErrorMsg;

#ifdef PNG_SUPPORT
    // Contains the list of image names for the current ROM
    FSList myImageList;

    // Index of currently displayed image
    int myImageIdx{0};

    // Current x-position of the mouse
    int myMouseX{0};
#endif

  private:
    // Following constructors and assignment operators not supported
    RomImageWidget() = delete;
    RomImageWidget(const RomImageWidget&) = delete;
    RomImageWidget(RomImageWidget&&) = delete;
    RomImageWidget& operator=(const RomImageWidget&) = delete;
    RomImageWidget& operator=(RomImageWidget&&) = delete;
};

#endif
