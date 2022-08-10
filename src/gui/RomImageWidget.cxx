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

#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "PNGLibrary.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "bspf.hxx"
#include "RomImageWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomImageWidget::RomImageWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss)
{
  _flags = Widget::FLAG_ENABLED;
  _bgcolor = kDlgColor;
  _bgcolorlo = kBGColorLo;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::setProperties(const FSNode& node, const string& md5)
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
void RomImageWidget::clearProperties()
{
  myHaveProperties = mySurfaceIsValid = false;
  if(mySurface)
    mySurface->setVisible(mySurfaceIsValid);

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandlerState::LAUNCHER)
    setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::reloadProperties(const FSNode& node)
{
  // The ROM may have changed since we were last in the browser, either
  // by saving a different image or through a change in video renderer,
  // so we reload the properties
  if(myHaveProperties)
    parseProperties(node);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::parseProperties(const FSNode& node)
{
  // Check if a surface has ever been created; if so, we use it
  // The surface will always be the maximum size, but sometimes we'll
  // only draw certain parts of it
  if(mySurface == nullptr)
  {
    mySurface = instance().frameBuffer().allocateSurface(
        _w, _h, ScalingInterpolation::blur);
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

#ifdef PNG_SUPPORT
  // TODO: RETRON_77

  // Get a valid filename representing a snapshot file for this rom and load the snapshot
  const string& path = instance().snapshotLoadDir().getPath();

  // 1. Try to load snapshots by property name
  if(getImageList(path + myProperties.get(PropType::Cart_Name)))
    mySurfaceIsValid = loadPng(myImageList[0].getPath());
  //mySurfaceIsValid = loadPng(path + myProperties.get(PropType::Cart_Name) + ".png");

  if(!mySurfaceIsValid)
  {
    // 2. If no snapshots with property name exists, try to load snapshot images by filename
    if(getImageList(path + node.getNameWithExt("")))
      mySurfaceIsValid = loadPng(myImageList[0].getPath());
    //mySurfaceIsValid = loadPng(path + node.getNameWithExt("") + ".png");

    if(!mySurfaceIsValid)
    {
      // 3. If no ROM snapshots exist, try to load a default snapshot
      mySurfaceIsValid = loadPng(path + "default_snapshot.png");
    }
  }
#else
  mySurfaceErrorMsg = "PNG image loading not supported";
#endif
  if(mySurface)
    mySurface->setVisible(mySurfaceIsValid);

  setDirty();
}

#ifdef PNG_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::getImageList(const string& filename)
{
  FSNode::NameFilter filter = ([&](const FSNode& node) {
    return (!node.isDirectory() &&
      (node.getPath() == filename + ".png" ||
       BSPF::matchWithWildcards(node.getPath(), filename + "#*.png")));
  });

  FSNode node(instance().snapshotLoadDir().getPath());

  myImageList.clear();
  node.getChildren(myImageList, FSNode::ListMode::FilesOnly, filter, false, false);
  return myImageList.size() > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::loadPng(const string& filename)
{
  try
  {
    instance().png().loadImage(filename, *mySurface);

    // Scale surface to available image area
    const Common::Rect& src = mySurface->srcRect();
    const float scale = std::min(float(_w) / src.w(), float(_h) / src.h()) *
      instance().frameBuffer().hidpiScaleFactor();
    mySurface->setDstSize(static_cast<uInt32>(src.w() * scale), static_cast<uInt32>(src.h() * scale));

    setDirty();
    return true;
  }
  catch(const runtime_error& e)
  {
    mySurfaceErrorMsg = e.what();
  }
  return false;
}
#endif

#ifdef PNG_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    if(x < _w/2)
    {
      if(myImageIdx)
      {
        loadPng(myImageList[--myImageIdx].getPath());
      }
    }
    else if(myImageIdx < myImageList.size())
    {
      loadPng(myImageList[++myImageIdx].getPath());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::handleMouseMoved(int x, int y)
{
  myMouseX = x;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();
  const int yoff = _h + _font.getFontHeight() / 2;

  s.fillRect(_x+2, _y+2, _w-4, _h-4, _bgcolor);
  s.frameRect(_x, _y, _w, _h, kColor);

  if(!myHaveProperties)
  {
    clearDirty();
    return;
  }

  if(mySurfaceIsValid)
  {
    const Common::Rect& dst = mySurface->dstRect();
    const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
    const uInt32 x = _x * scale + ((_w * scale - dst.w()) >> 1);
    const uInt32 y = _y * scale + ((_h * scale - dst.h()) >> 1);

    // Make sure when positioning the snapshot surface that we take
    // the dialog surface position into account
    const Common::Rect& s_dst = s.dstRect();
    mySurface->setDstPos(x + s_dst.x(), y + s_dst.y());
  }
  else if(mySurfaceErrorMsg != "")
  {
    const uInt32 x = _x + ((_w - _font.getStringWidth(mySurfaceErrorMsg)) >> 1);
    const uInt32 y = _y + ((yoff - _font.getLineHeight()) >> 1);
    s.drawString(_font, mySurfaceErrorMsg, x, y, _w - 10, _textcolor);
  }

#ifdef PNG_SUPPORT
  if(isHighlighted())
  {
    // TODO: need another surface
    const int xOfs = myMouseX < _w / 2 ? 10 : _w - 50;

    s.line(xOfs, _h / 2 - 10, xOfs + 20, _h / 2, kBtnTextColorHi);
  }
#endif

  clearDirty();
}
