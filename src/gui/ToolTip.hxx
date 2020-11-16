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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TOOL_TIP_HXX
#define TOOL_TIP_HXX

/**
 * Class for providing tool tip functionality
 *
 * @author Thomas Jentzsch
 */

class OSystem;
class Dialog;
class Widget;
class FBSurface;

#include "Rect.hxx"

class ToolTip
{
public:
  // Maximum tooltip length
  static constexpr uInt32 MAX_LEN = 80;

  ToolTip(Dialog& dialog, const GUI::Font& font);
  ~ToolTip() = default;

  /**
    Request a tooltip display
  */
  void request(const Widget* widget);


  /**
    Hide an existing tooltip (if displayed)
  */
  void release();

  /**
    Update with current mouse position
  */
  void update(int x, int y);

  /*
    Render the tooltip
  */
  void render();

private:
  static constexpr uInt32 DELAY_TIME = 45; // display delay
  //static constexpr int TEXT_Y_OFS = 2;

  Dialog& myDialog;
  const GUI::Font& myFont;

  const Widget* myWidget{nullptr};
  uInt32 myTimer{0};
  Common::Point myPos;
  uInt32 myWidth{0};
  uInt32 myHeight{0};
  uInt32 myScale{1};
  uInt32 myTextXOfs{0};
  uInt32 myTextYOfs{0};
  shared_ptr<FBSurface> mySurface;
};

#endif
