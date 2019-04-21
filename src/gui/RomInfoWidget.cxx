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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "ControllerDetector.hxx"
#include "Props.hxx"
#include "PNGLibrary.hxx"
#include "Rect.hxx"
#include "Widget.hxx"
#include "TIAConstants.hxx"
#include "RomInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomInfoWidget::RomInfoWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    mySurfaceIsValid(false),
    myHaveProperties(false),
    myAvail(w > 400 ?
      GUI::Size(TIAConstants::viewableWidth*2, TIAConstants::viewableHeight*2) :
      GUI::Size(TIAConstants::viewableWidth, TIAConstants::viewableHeight))
{
  _flags = WIDGET_ENABLED;
  _bgcolor = kDlgColor;
  _bgcolorlo = kBGColorLo;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::reloadProperties(const FilesystemNode& node)
{
  // The ROM may have changed since we were last in the browser, either
  // by saving a different image or through a change in video renderer,
  // so we reload the properties
  if(myHaveProperties)
    parseProperties(node);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::setProperties(const Properties& props, const FilesystemNode& node)
{
  myHaveProperties = true;
  myProperties = props;

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandlerState::LAUNCHER)
    parseProperties(node);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::clearProperties()
{
  myHaveProperties = mySurfaceIsValid = false;
  if(mySurface)
    mySurface->setVisible(mySurfaceIsValid);

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandlerState::LAUNCHER)
    setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::parseProperties(const FilesystemNode& node)
{
  // Check if a surface has ever been created; if so, we use it
  // The surface will always be the maximum size, but sometimes we'll
  // only draw certain parts of it
  if(mySurface == nullptr)
  {
    mySurface = instance().frameBuffer().allocateSurface(
        TIAConstants::viewableWidth*2, TIAConstants::viewableHeight*2);
    mySurface->attributes().smoothing = true;
    mySurface->applyAttributes();

    dialog().addSurface(mySurface);
  }

  // Initialize to empty properties entry
  mySurfaceErrorMsg = "";
  mySurfaceIsValid = false;
  myRomInfo.clear();

#ifdef PNG_SUPPORT
  // Get a valid filename representing a snapshot file for this rom
  const string& filename = instance().snapshotLoadDir() +
      myProperties.get(PropType::Cart_Name) + ".png";

  // Read the PNG file
  try
  {
    instance().png().loadImage(filename, *mySurface);

    // Scale surface to available image area
    const GUI::Rect& src = mySurface->srcRect();
    float scale = std::min(float(myAvail.w) / src.width(), float(myAvail.h) / src.height());
    mySurface->setDstSize(uInt32(src.width() * scale), uInt32(src.height() * scale));
    mySurfaceIsValid = true;
  }
  catch(const runtime_error& e)
  {
    mySurfaceErrorMsg = e.what();
  }
#else
  mySurfaceErrorMsg = "PNG image loading not supported";
#endif
  if(mySurface)
    mySurface->setVisible(mySurfaceIsValid);

  // Now add some info for the message box below the image
  myRomInfo.push_back("Name: " + myProperties.get(PropType::Cart_Name));
  myRomInfo.push_back("Manufacturer: " + myProperties.get(PropType::Cart_Manufacturer));
  myRomInfo.push_back("Model: " + myProperties.get(PropType::Cart_ModelNo));
  myRomInfo.push_back("Rarity: " + myProperties.get(PropType::Cart_Rarity));
  myRomInfo.push_back("Note: " + myProperties.get(PropType::Cart_Note));
  bool swappedPorts = myProperties.get(PropType::Console_SwapPorts) == "YES";

  // Load the image for controller auto detection
  string left = myProperties.get(PropType::Controller_Left);
  string right = myProperties.get(PropType::Controller_Right);
  try
  {
    BytePtr image;
    string md5 = myProperties.get(PropType::Cart_MD5);
    uInt32 size = 0;

    if(node.exists() && !node.isDirectory() &&
      (image = instance().openROM(node, md5, size)) != nullptr)
    {
      left = ControllerDetector::detectName(image.get(), size, left,
          !swappedPorts ? Controller::Jack::Left : Controller::Jack::Right,
          instance().settings());
      right = ControllerDetector::detectName(image.get(), size, right,
          !swappedPorts ? Controller::Jack::Right : Controller::Jack::Left,
          instance().settings());
    }
  }
  catch(const runtime_error&)
  {
    // Do nothing; we simply don't update the controllers if openROM
    // failed for any reason
    left = right = "";
  }
  if(left != "" && right != "")
    myRomInfo.push_back("Controllers: " + (left + " (left), " + right + " (right)"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();
  bool onTop = _boss->dialog().isOnTop();

  const int yoff = myAvail.h + 10;

  s.fillRect(_x+2, _y+2, _w-4, _h-4, onTop ? _bgcolor : _bgcolorlo);
  s.frameRect(_x, _y, _w, _h, kColor);
  s.frameRect(_x, _y+yoff, _w, _h-yoff, kColor);

  if(!myHaveProperties) return;

  if(mySurfaceIsValid)
  {
    const GUI::Rect& dst = mySurface->dstRect();
    uInt32 x = _x + ((_w - dst.width()) >> 1);
    uInt32 y = _y + ((yoff - dst.height()) >> 1);

    // Make sure when positioning the snapshot surface that we take
    // the dialog surface position into account
    const GUI::Rect& s_dst = s.dstRect();
    mySurface->setDstPos(x + s_dst.x(), y + s_dst.y());
  }
  else if(mySurfaceErrorMsg != "")
  {
    const GUI::Font& font = instance().frameBuffer().font();
    uInt32 x = _x + ((_w - font.getStringWidth(mySurfaceErrorMsg)) >> 1);
    uInt32 y = _y + ((yoff - font.getLineHeight()) >> 1);
    s.drawString(font, mySurfaceErrorMsg, x, y, _w - 10, onTop ? _textcolor : _shadowcolor);
  }

  int xpos = _x + 8, ypos = _y + yoff + 10;
  for(const auto& info: myRomInfo)
  {
    s.drawString(_font, info, xpos, ypos, _w - 16, onTop ? _textcolor : _shadowcolor);
    ypos += _font.getLineHeight();
  }
}
