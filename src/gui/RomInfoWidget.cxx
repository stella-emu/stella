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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
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
#include "Bankswitch.hxx"
#include "CartDetector.hxx"
#include "Logger.hxx"
#include "Props.hxx"
#include "PNGLibrary.hxx"
#include "PropsSet.hxx"
#include "Rect.hxx"
#include "Widget.hxx"
#include "RomInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomInfoWidget::RomInfoWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, int w, int h,
                             const Common::Size& imgSize)
  : Widget(boss, font, x, y, w, h),
    myAvail{imgSize}
{
  _flags = Widget::FLAG_ENABLED;
  _bgcolor = kDlgColor;
  _bgcolorlo = kBGColorLo;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::setProperties(const FilesystemNode& node, const string& md5)
{
  myHaveProperties = true;

  // Make sure to load a per-ROM properties entry, if one exists
  instance().propSet().loadPerROM(node, md5);

  // And now get the properties for this ROM
  instance().propSet().getMD5(md5, myProperties);

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
void RomInfoWidget::reloadProperties(const FilesystemNode& node)
{
  // The ROM may have changed since we were last in the browser, either
  // by saving a different image or through a change in video renderer,
  // so we reload the properties
  if(myHaveProperties)
    parseProperties(node);
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
        myAvail.w, myAvail.h, ScalingInterpolation::blur);
    mySurface->applyAttributes();

    dialog().addRenderCallback([this]() {
      if(mySurfaceIsValid)
        mySurface->render();
      }
    );
  }

  // Initialize to empty properties entry
  mySurfaceErrorMsg = "";
  mySurfaceIsValid = false;
  myRomInfo.clear();

#ifdef PNG_SUPPORT
  // Get a valid filename representing a snapshot file for this rom
  const string& filename = instance().snapshotLoadDir().getPath() +
      myProperties.get(PropType::Cart_Name) + ".png";

  // Read the PNG file
  mySurfaceIsValid = loadPng(filename);

  // Try to load a default image if not ROM image exists
  if(!mySurfaceIsValid)
  {
    mySurfaceIsValid = loadPng(instance().snapshotLoadDir().getPath() +
                               "default_snapshot.png");
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

  // Load the image for controller and bankswitch type auto detection
  string left = myProperties.get(PropType::Controller_Left);
  string right = myProperties.get(PropType::Controller_Right);
  Controller::Type leftType = Controller::getType(left);
  Controller::Type rightType = Controller::getType(right);
  string bsDetected = myProperties.get(PropType::Cart_Type);
  try
  {
    ByteBuffer image;
    string md5 = "";  size_t size = 0;

    if(node.exists() && !node.isDirectory() &&
      (image = instance().openROM(node, md5, size)) != nullptr)
    {
      Logger::debug(myProperties.get(PropType::Cart_Name) + ":");
      left = ControllerDetector::detectName(image, size, leftType,
          !swappedPorts ? Controller::Jack::Left : Controller::Jack::Right,
          instance().settings());
      right = ControllerDetector::detectName(image, size, rightType,
          !swappedPorts ? Controller::Jack::Right : Controller::Jack::Left,
          instance().settings());
      if (bsDetected == "AUTO")
        bsDetected = Bankswitch::typeToName(CartDetector::autodetectType(image, size));
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
  if (bsDetected != "")
    myRomInfo.push_back("Type: " + Bankswitch::typeToDesc(Bankswitch::nameToType(bsDetected)));

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::resetSurfaces()
{
  if(mySurface)
    mySurface->reload();
}

#ifdef PNG_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomInfoWidget::loadPng(const string& filename)
{
  try
  {
    instance().png().loadImage(filename, *mySurface);

    // Scale surface to available image area
    const Common::Rect& src = mySurface->srcRect();
    float scale = std::min(float(myAvail.w) / src.w(), float(myAvail.h) / src.h()) *
      instance().frameBuffer().hidpiScaleFactor();
    mySurface->setDstSize(uInt32(src.w() * scale), uInt32(src.h() * scale));

    return true;
  }
  catch(const runtime_error& e)
  {
    mySurfaceErrorMsg = e.what();
  }
  return false;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();
  const int yoff = myAvail.h + 10;

  s.fillRect(_x+2, _y+2, _w-4, _h-4, _bgcolor);
  s.frameRect(_x, _y, _w, _h, kColor);
  s.frameRect(_x, _y+yoff, _w, _h-yoff, kColor);

  if(!myHaveProperties) return;

  if(mySurfaceIsValid)
  {
    const Common::Rect& dst = mySurface->dstRect();
    const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
    uInt32 x = _x*scale + ((_w*scale - dst.w()) >> 1);
    uInt32 y = _y*scale + ((yoff*scale - dst.h()) >> 1);

    // Make sure when positioning the snapshot surface that we take
    // the dialog surface position into account
    const Common::Rect& s_dst = s.dstRect();
    mySurface->setDstPos(x + s_dst.x(), y + s_dst.y());
  }
  else if(mySurfaceErrorMsg != "")
  {
    uInt32 x = _x + ((_w - _font.getStringWidth(mySurfaceErrorMsg)) >> 1);
    uInt32 y = _y + ((yoff - _font.getLineHeight()) >> 1);
    s.drawString(_font, mySurfaceErrorMsg, x, y, _w - 10, _textcolor);
  }

  int xpos = _x + 8, ypos = _y + yoff + 5;
  for(const auto& info : myRomInfo)
  {
    if(info.length() * _font.getMaxCharWidth() <= uInt64(_w - 16))

    {
      // 1 line for next entry
      if(ypos + _font.getFontHeight() > _h + _y)
        break;
    }
    else
    {
      // assume 2 lines for next entry
      if(ypos + _font.getLineHeight() + _font.getFontHeight() > _h + _y )
        break;
    }
    int lines = s.drawString(_font, info, xpos, ypos, _w - 16, _font.getFontHeight() * 3,
                             _textcolor);
    ypos += _font.getLineHeight() + (lines - 1) * _font.getFontHeight();
  }
  clearDirty();
}
