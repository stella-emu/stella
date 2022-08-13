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
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_TRACK_MOUSE;
  _bgcolor = kDlgColor;
  _bgcolorlo = kBGColorLo;
  myImageHeight = _h - labelHeight(font);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::setProperties(const FSNode& node, const string& md5, bool complete)
{
  myHaveProperties = true;

  // Make sure to load a per-ROM properties entry, if one exists
  instance().propSet().loadPerROM(node, md5);

  // And now get the properties for this ROM
  instance().propSet().getMD5(md5, myProperties);

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandlerState::LAUNCHER)
    parseProperties(node, complete);
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
    parseProperties(node, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::parseProperties(const FSNode& node, bool complete)
{
  if(myNavSurface == nullptr)
  {
    // Create navigation surface
    myNavSurface = instance().frameBuffer().allocateSurface(
      _w, myImageHeight);

    FBSurface::Attributes& attr = myNavSurface->attributes();

    attr.blending = true;
    attr.blendalpha = 60;
    myNavSurface->applyAttributes();
  }

  // Check if a surface has ever been created; if so, we use it
  // The surface will always be the maximum size, but sometimes we'll
  // only draw certain parts of it
  if(mySurface == nullptr)
  {
    mySurface = instance().frameBuffer().allocateSurface(
        _w, myImageHeight, ScalingInterpolation::blur);
    mySurface->applyAttributes();

    dialog().addRenderCallback([this]() {
      if(mySurfaceIsValid)
      {
        mySurface->render();
        if(isHighlighted())
          myNavSurface->render();
      }
    });
  }

  // Initialize to empty properties entry
  mySurfaceErrorMsg = "";
  mySurfaceIsValid = false;

#ifdef PNG_SUPPORT
  // TODO: RETRON_77

  // Get a valid filename representing a snapshot file for this rom and load the snapshot
  myImageList.clear();
  myImageIdx = 0;

  if(complete)
  {
    // Try to load snapshots by property name and ROM file name
    getImageList(myProperties.get(PropType::Cart_Name), node.getNameWithExt());

    if(myImageList.size())
      mySurfaceIsValid = loadPng(myImageList[0].getPath());
  }
  else
  {
    const string& path = instance().snapshotLoadDir().getPath();
    string filename = path + myProperties.get(PropType::Cart_Name) + ".png";

    mySurfaceIsValid = loadPng(filename);
    if(!mySurfaceIsValid)
    {
      filename = path + node.getNameWithExt("png");
      mySurfaceIsValid = loadPng(filename);
    }
    if(mySurfaceIsValid)
      myImageList.emplace_back(filename);
  }

  if(!mySurfaceIsValid)
  {
    // If no ROM snapshots exist, try to load a default snapshot
    mySurfaceIsValid = loadPng(instance().snapshotLoadDir().getPath() + "default_snapshot.png");
  }

#else
  mySurfaceErrorMsg = "PNG image loading not supported";
#endif
  if(mySurface)
    mySurface->setVisible(mySurfaceIsValid);

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::changeImage(int direction)
{
  if(direction == -1 && myImageIdx)
    return loadPng(myImageList[--myImageIdx].getPath());
  else if(direction == 1 && myImageIdx < myImageList.size() - 1)
    return loadPng(myImageList[++myImageIdx].getPath());

  return false;
}

#ifdef PNG_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::getImageList(const string& propname, const string& filename)
{
  // TODO: search for consecutive digits and letters instead of getChildren
  //std::ifstream in(filename, std::ios_base::binary);
  //if(!in.is_open())
  //  loadImageERROR("No snapshot found");
  // or use a timer before updating the images

  const string pngPropName = propname + ".png";
  const string pngFileName = filename + ".png";
  FSNode::NameFilter filter = ([&](const FSNode& node)
    {
      const string& nodeName = node.getName();
      return
        (nodeName == pngPropName || nodeName == pngFileName ||
        (nodeName.find(propname + " #") == 0 &&
         nodeName.find(".png") == nodeName.length() - 4) ||
        (nodeName.find(filename + " #") == 0 &&
         nodeName.find(".png") == nodeName.length() - 4));
    }
  );

  // Find all images matching the filename and the extension
  FSNode node(instance().snapshotLoadDir().getPath());
  node.getChildren(myImageList, FSNode::ListMode::FilesOnly, filter, false, false);

  // Sort again, not considering extensions, else <filename.png> would be at
  // the end of the list
  std::sort(myImageList.begin(), myImageList.end(),
            [](const FSNode& node1, const FSNode& node2)
    {
      return BSPF::compareIgnoreCase(node1.getNameWithExt(), node2.getNameWithExt()) < 0;
    }
  );
  return myImageList.size() > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::loadPng(const string& filename)
{
  try
  {
    VariantList comments;
    instance().png().loadImage(filename, *mySurface, comments);

    // Scale surface to available image area
    const Common::Rect& src = mySurface->srcRect();
    const float scale = std::min(float(_w) / src.w(), float(myImageHeight) / src.h()) *
      instance().frameBuffer().hidpiScaleFactor();
    mySurface->setDstSize(static_cast<uInt32>(src.w() * scale), static_cast<uInt32>(src.h() * scale));

    // Retrieve label for loaded image
    myLabel = "";
    for(auto comment = comments.begin(); comment != comments.end(); ++comment)
    {
      if(comment->first == "Title")
      {
        myLabel = comment->second.toString();
        break;
      }
      if(comment->first == "Software"
          && comment->second.toString().find("Stella") == 0)
        myLabel = "Snapshot"; // default for Stella snapshots with missing "Title" comment
    }

    setDirty();
    return true;
  }
  catch(const runtime_error& e)
  {
    mySurfaceErrorMsg = e.what();
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && x >= 0 && x < _w && y >= 0 && y < myImageHeight)
    changeImage(x < _w / 2 ? -1 : 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::handleMouseMoved(int x, int y)
{
  if(x < _w / 2 != myMouseX < _w / 2)
    setDirty();
  myMouseX = x;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();
  const int yoff = myImageHeight;

  s.fillRect(_x+1, _y+1, _w-2, _h-1, _bgcolor);
  s.frameRect(_x, _y, _w, myImageHeight, kColor);

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
    const uInt32 y = _y * scale + ((myImageHeight * scale - dst.h()) >> 1);

    // Make sure when positioning the snapshot surface that we take
    // the dialog surface position into account
    const Common::Rect& s_dst = s.dstRect();
    mySurface->setDstPos(x + s_dst.x(), y + s_dst.y());

    // Draw the image label and counter
    ostringstream buf;
    buf << myImageIdx + 1 << "/" << myImageList.size();
    const int yText = _y + myImageHeight + _font.getFontHeight() / 8;
    const int wText = _font.getStringWidth(buf.str());

    if(myLabel.length())
      s.drawString(_font, myLabel, _x, yText, _w - wText - _font.getMaxCharWidth() * 2, _textcolor);
    if(myImageList.size())
      s.drawString(_font, buf.str(), _x + _w - wText, yText, wText, _textcolor);

    // Draw the navigation arrows
    const bool leftArrow = myMouseX < _w / 2;

    myNavSurface->invalidate();
    if(isHighlighted() &&
      ((leftArrow && myImageIdx) || (!leftArrow && myImageIdx < myImageList.size() - 1)))
    {
      const int w = _w / 64;
      const int w2 = 1; // w / 2;
      const int ax = leftArrow ? _w / 12 - w / 2 : _w - _w / 12 - w / 2;
      const int ay = myImageHeight >> 1;
      const int dx = (_w / 32) * (leftArrow ? 1 : -1);
      const int dy = myImageHeight / 16;

      for(int i = 0; i < w; ++i)
      {
        myNavSurface->line(ax + dx + i + w2, ay - dy, ax + i + w2, ay, kBGColor);
        myNavSurface->line(ax + dx + i + w2, ay + dy, ax + i + w2, ay, kBGColor);
        myNavSurface->line(ax + dx + i, ay - dy + w2, ax + i, ay + w2, kBGColor);
        myNavSurface->line(ax + dx + i, ay + dy + w2, ax + i, ay + w2, kBGColor);
        myNavSurface->line(ax + dx + i + w2, ay - dy + w2, ax + i + w2, ay + w2, kBGColor);
        myNavSurface->line(ax + dx + i + w2, ay + dy + w2, ax + i + w2, ay + w2, kBGColor);
      }
      for(int i = 0; i < w; ++i)
      {
        myNavSurface->line(ax + dx + i, ay - dy, ax + i, ay, kColorInfo);
        myNavSurface->line(ax + dx + i, ay + dy, ax + i, ay, kColorInfo);
      }
      myNavSurface->setDstRect(mySurface->dstRect());
    }
  }
  else if(mySurfaceErrorMsg != "")
  {
    const uInt32 x = _x + ((_w - _font.getStringWidth(mySurfaceErrorMsg)) >> 1);
    const uInt32 y = _y + ((yoff - _font.getLineHeight()) >> 1);
    s.drawString(_font, mySurfaceErrorMsg, x, y, _w - 10, _textcolor);
  }
  clearDirty();
}
