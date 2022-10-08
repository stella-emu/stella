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

#include <regex>
#include "EventHandler.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "JPGLibrary.hxx"
#include "OSystem.hxx"
#include "PNGLibrary.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "TimerManager.hxx"
#include "RomImageWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomImageWidget::RomImageWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_TRACK_MOUSE;
  _bgcolor = kDlgColor;
  _bgcolorlo = kBGColorLo;
  myImageHeight = _h - labelHeight(font);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::setProperties(const FSNode& node,
                                   const Properties& properties, bool full)
{
  myHaveProperties = true;
  myProperties = properties;

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandlerState::LAUNCHER)
    parseProperties(node, full);
#ifdef DEBUGGER_SUPPORT
  else
  {
    cerr << "RomImageWidget::setProperties: else!" << endl;
    Logger::debug("RomImageWidget::setProperties: else!");
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::clearProperties()
{
  myHaveProperties = mySurfaceIsValid = false;
  if(mySurface)
    mySurface->setVisible(false);

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandlerState::LAUNCHER)
    setDirty();
#ifdef DEBUGGER_SUPPORT
  else
  {
    cerr << "RomImageWidget::clearProperties: else!" << endl;
    Logger::debug("RomImageWidget::clearProperties: else!");
  }
#endif
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
void RomImageWidget::parseProperties(const FSNode& node, bool full)
{
  const uInt64 startTime = TimerManager::getTicks() / 1000;

  if(myNavSurface == nullptr)
  {
    // Create navigation surface
    myNavSurface = instance().frameBuffer().allocateSurface(
      _w, myImageHeight);

    const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
    myNavSurface->setDstRect(
      Common::Rect(_x * scale, _y * scale,
                   (_x + _w) * scale, (_y + myImageHeight) * scale));

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
        mySurface->render();
      if(isHighlighted())
        myNavSurface->render();
    });
  }

#ifdef IMAGE_SUPPORT
  if(!full)
  {
    myImageIdx = 0;
    myImageList.clear();
    myLabel.clear();

    // Get a valid filename representing a snapshot file for this rom and load the snapshot
    const string& path = instance().snapshotLoadDir().getPath();

    // 1. Try to load first snapshot by property name
    string fileName = path + myProperties.get(PropType::Cart_Name);
    tryImageFormats(fileName);
    if(!mySurfaceIsValid)
    {
      // 2. If none exists, try to load first snapshot by ROM file name
      fileName = path + node.getName();
      tryImageFormats(fileName);
    }
    if(mySurfaceIsValid)
      myImageList.emplace_back(fileName);
    else
    {
      // 3. If no ROM snapshots exist, try to load a default snapshot
      fileName = path + "default_snapshot";
      tryImageFormats(fileName);
    }
  }
  else
  {
    const string oldFileName = !myImageList.empty()
        ? myImageList[0].getPath() : EmptyString;

    // Try to find all snapshots by property and ROM file name
    myImageList.clear();
    getImageList(myProperties.get(PropType::Cart_Name), node.getNameWithExt(),
      oldFileName);

    // The first file found before must not be the first file now, if files by
    // property *and* ROM name are found (TODO: fix that!)
    if(!myImageList.empty() && myImageList[0].getPath() != oldFileName)
      loadImage(myImageList[0].getPath());
    else
      setDirty(); // update the counter display
  }
#else
  mySurfaceIsValid = false;
  mySurfaceErrorMsg = "Image loading not supported";
  setDirty();
#endif
  if(mySurface)
    mySurface->setVisible(mySurfaceIsValid);

  // Update maximum load time
  myMaxLoadTime = std::min(
    static_cast<uInt64>(500ULL / timeFactor),
    std::max(myMaxLoadTime, TimerManager::getTicks() / 1000 - startTime));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::changeImage(int direction)
{
#ifdef IMAGE_SUPPORT
  if(direction == -1 && myImageIdx)
    return loadImage(myImageList[--myImageIdx].getPath());
  else if(direction == 1 && myImageIdx + 1 < myImageList.size())
    return loadImage(myImageList[++myImageIdx].getPath());
#endif
  return false;
}

#ifdef IMAGE_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::getImageList(const string& propName, const string& romName,
                                  const string& oldFileName)
{
  const std::regex symbols{R"([-[\]{}()*+?.,\^$|#])"}; // \s
  const string rgxPropName = std::regex_replace(propName, symbols, R"(\$&)");
  const string rgxRomName  = std::regex_replace(romName,  symbols, R"(\$&)");
  // Look for <name.png|jpg> or <name_#.png|jpg> (# is a number)
  const std::regex rgx("^(" + rgxPropName + "|" + rgxRomName + ")(_\\d+)?\\.(png|jpg)$");

  const FSNode::NameFilter filter = ([&](const FSNode& node) {
      return std::regex_match(node.getName(), rgx);
    }
  );

  // Find all images matching the given names and the extension
  const FSNode node(instance().snapshotLoadDir().getPath());
  node.getChildren(myImageList, FSNode::ListMode::FilesOnly, filter, false, false);

  // Sort again, not considering extensions, else <filename.png|jpg> would be at
  // the end of the list
  std::sort(myImageList.begin(), myImageList.end(),
            [oldFileName](const FSNode& node1, const FSNode& node2)
    {
      const int compare = BSPF::compareIgnoreCase(
        node1.getNameWithExt(), node2.getNameWithExt());
      return
        compare < 0 ||
        // PNGs first!
        (compare == 0 &&
          node1.getName().substr(node1.getName().find_last_of('.') + 1) >
          node2.getName().substr(node2.getName().find_last_of('.') + 1)) ||
        // Make sure that first image found in initial load is first image now too
        node1.getName() == oldFileName;
    }
  );
  return !myImageList.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::tryImageFormats(string& fileName)
{
  if(loadImage(fileName + ".png"))
  {
    fileName += ".png";
    return true;
  }
  if(loadImage(fileName + ".jpg"))
  {
    fileName += ".jpg";
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::loadImage(const string& fileName)
{
  mySurfaceErrorMsg.clear();

  const string::size_type idx = fileName.find_last_of('.');

  if(idx != string::npos && fileName.substr(idx + 1) == "png")
    mySurfaceIsValid = loadPng(fileName);
  else
    mySurfaceIsValid = loadJpg(fileName);

  if(mySurfaceIsValid)
  {
    // Scale surface to available image area
    const Common::Rect& src = mySurface->srcRect();
    const float scale = std::min(
      static_cast<float>(_w) / src.w(),
      static_cast<float>(myImageHeight) / src.h()) *
        instance().frameBuffer().hidpiScaleFactor();
    mySurface->setDstSize(static_cast<uInt32>(src.w() * scale), static_cast<uInt32>(src.h() * scale));
  }

  if(mySurface)
    mySurface->setVisible(mySurfaceIsValid);

  setDirty();
  return mySurfaceIsValid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::loadPng(const string& fileName)
{
  try
  {
    VariantList metaData;
    instance().png().loadImage(fileName, *mySurface, metaData);

    // Retrieve label for loaded image
    myLabel.clear();
    for(const auto& data: metaData)
    {
      if(data.first == "Title")
      {
        myLabel = data.second.toString();
        break;
      }
      if(data.first == "Software"
          && data.second.toString().find("Stella") == 0)
        myLabel = "Snapshot"; // default for Stella snapshots with missing "Title" meta data
    }
    return true;
  }
  catch(const runtime_error& e)
  {
    mySurfaceErrorMsg = e.what();
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomImageWidget::loadJpg(const string& fileName)
{
  try
  {
    VariantList metaData;
    instance().jpg().loadImage(fileName, *mySurface, metaData);

    // Retrieve label for loaded image
    myLabel.clear();
    for(const auto& data: metaData)
    {
      if(data.first == "ImageDescription")
      {
        myLabel = data.second.toString();
        break;
      }
    }
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
  if((x < _w / 2) != myMouseLeft)
    setDirty();
  myMouseLeft = x < _w / 2;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomImageWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();

  if(!myHaveProperties || !mySurfaceIsValid || !mySurfaceErrorMsg.empty())
  {
    s.fillRect(_x, _y + 1, _w, _h - 1, _bgcolor);
    s.frameRect(_x, _y, _w, myImageHeight, kColor);
  }

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

    s.fillRect(_x, _y, _w, myImageHeight, 0);
    // Make sure when positioning the snapshot surface that we take
    // the dialog surface position into account
    const Common::Rect& s_dst = s.dstRect();
    mySurface->setDstPos(x + s_dst.x(), y + s_dst.y());
  }
  else if(!mySurfaceErrorMsg.empty())
  {
    const uInt32 x = _x + ((_w - _font.getStringWidth(mySurfaceErrorMsg)) >> 1);
    const uInt32 y = _y + ((myImageHeight - _font.getLineHeight()) >> 1);
    s.drawString(_font, mySurfaceErrorMsg, x, y, _w - 10, _textcolor);
  }
  // Draw the image label and counter
  ostringstream buf;
  buf << myImageIdx + 1 << "/" << myImageList.size();
  const int yText = _y + myImageHeight + _font.getFontHeight() / 8;
  const int wText = _font.getStringWidth(buf.str());

  s.fillRect(_x, yText, _w, _font.getFontHeight(), _bgcolor);
  if(myLabel.length())
    s.drawString(_font, myLabel, _x, yText, _w - wText - _font.getMaxCharWidth() * 2, _textcolor);
  if(!myImageList.empty())
    s.drawString(_font, buf.str(), _x + _w - wText, yText, wText, _textcolor);

  // Draw the navigation arrows
  myNavSurface->invalidate();
  if(isHighlighted() &&
    ((myMouseLeft && myImageIdx) || (!myMouseLeft && myImageIdx + 1 < myImageList.size())))
  {
    const int w = _w / 64;
    const int w2 = 1; // w / 2;
    const int ax = myMouseLeft ? _w / 12 - w / 2 : _w - _w / 12 - w / 2;
    const int ay = myImageHeight >> 1;
    const int dx = (_w / 32) * (myMouseLeft ? 1 : -1);
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
  }
  clearDirty();
}
