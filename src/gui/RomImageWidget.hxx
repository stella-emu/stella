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

class RomImageWidget : public Widget
{
  public:
    RomImageWidget(GuiObject *boss, const GUI::Font& font,
                  int x, int y, int w, int h);
    ~RomImageWidget() override = default;

    static int labelHeight(const GUI::Font& font)
    {
      return font.getFontHeight() * 9 / 8;
    }

    void setProperties(const FSNode& node, const Properties properties, bool full = true);
    void clearProperties();
    void reloadProperties(const FSNode& node);
    bool changeImage(int direction = 1);

    uInt64 pendingLoadTime() { return myMaxLoadTime * timeFactor; }

  protected:
    void drawWidget(bool hilite) override;
#ifdef IMAGE_SUPPORT
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseMoved(int x, int y) override;
#endif

  private:
    void parseProperties(const FSNode& node, bool full = true);
  #ifdef IMAGE_SUPPORT
    bool getImageList(const string& propName, const string& romName);
    bool tryImageFormats(string& fileName);
    bool loadImage(const string& fileName);
    bool loadPng(const string& fileName);
    bool loadJpg(const string& fileName);
  #endif

  private:
    // Pending load time safety factor
    static constexpr double timeFactor = 1.2;

  private:
    // Surface pointer holding the image
    shared_ptr<FBSurface> mySurface;

    // Surface pointer holding the navigation elements
    shared_ptr<FBSurface> myNavSurface;

    // Whether the surface should be redrawn by drawWidget()
    bool mySurfaceIsValid{false};

    // The properties for the currently selected ROM
    Properties myProperties;

    // Indicates if the current properties should actually be used
    bool myHaveProperties{false};

    // Indicates if an error occurred in creating/displaying the surface
    string mySurfaceErrorMsg;

    // Height of the image area
    int myImageHeight{0};

    // Contains the list of image names for the current ROM
    FSList myImageList;

    // Index of currently displayed image
    size_t myImageIdx{0};

    // Current x-position of the mouse
    bool myMouseLeft{true};

    // Label for the loaded image
    string myLabel;

    // Maximum load time, for adapting pending loads delay
    uInt64 myMaxLoadTime{0};

  private:
    // Following constructors and assignment operators not supported
    RomImageWidget() = delete;
    RomImageWidget(const RomImageWidget&) = delete;
    RomImageWidget(RomImageWidget&&) = delete;
    RomImageWidget& operator=(const RomImageWidget&) = delete;
    RomImageWidget& operator=(RomImageWidget&&) = delete;
};

#endif
