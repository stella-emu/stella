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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "OSystem.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "FBSurface.hxx"
#include "PNGLibrary.hxx"
#include "PropsSet.hxx"

#include "Bezel.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Bezel::Bezel(OSystem& osystem)
  : myOSystem{osystem},
    myFB{osystem.frameBuffer()}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Bezel::getName(const string& path, const Properties& props)
{
  string imageName;
  int index = 1; // skip property name

  do
  {
    imageName = getName(props, index);
    if(!imageName.empty())
    {
      // Note: JPG does not support transparency
      const string imagePath = path + imageName + ".png";
      const FSNode node(imagePath);
      if(node.exists())
        break;
    }
  } while(index != -1);

  return imageName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Bezel::getName(const Properties& props, int& index)
{
  if(++index == 1)
    return props.get(PropType::Bezel_Name);

  // Try to generate bezel name from cart name
  const string_view cartName = props.get(PropType::Cart_Name);
  size_t pos = cartName.find_first_of('(');
  if(pos == string_view::npos)
    pos = cartName.length() + 1;

  // The following suffixes are from "The Official No-Intro Convention",
  // covering all used combinations by "The Bezel Project" (except single ones)
  // (Unl) = unlicensed (Homebrews)
  static constexpr std::array<string_view, 8> suffixes = {
    " (USA)", " (USA) (Proto)", " (USA) (Unl)", " (USA) (Hack)",
    " (Europe)", " (Germany)", " (France) (Unl)", " (Australia)"
  };
  static constexpr int SUFFIX_START = 2;
  static constexpr int SUFFIX_END = SUFFIX_START + static_cast<int>(suffixes.size());

  if(index < SUFFIX_END && pos != string_view::npos && pos > 0)
  {
    string result{cartName.substr(0, pos - 1)};
    result += suffixes[index - SUFFIX_START];
    return result;
  }

  if(index == SUFFIX_END)
    return "Atari-2600";

  if(index == SUFFIX_END + 1)
  {
    index = -1;
    return "default";
  }
  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Bezel::getName(int& index) const
{
  return getName(myOSystem.console().properties(), index);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Bezel::borderSize(uInt32 x, uInt32 y, uInt32 size, Int32 step) const
{
  uInt32* pixels{nullptr};
  uInt32  pitch{0};
  mySurface->basePtr(pixels, pitch);

  pixels += x + y * pitch;

  const uInt32 aMask = myFB.aMask();
  for(uInt32 i = 0; i < size; ++i, pixels += step)
  {
    if((*pixels & aMask) != aMask)  // transparent pixel?
      return i;
  }
  return size - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Bezel::load()
{
  const Settings& settings = myOSystem.settings();
  bool isValid = false;
  string imageName;

#ifdef IMAGE_SUPPORT
  const bool show = myOSystem.eventHandler().inTIAMode() &&
    settings.getBool("bezel.show") &&
    (settings.getBool("fullscreen") ||
     settings.getBool("bezel.windowed"));

  if(show)
  {
    if(!mySurface)
      mySurface = myFB.allocateSurface(1, 1); // dummy size
    try
    {
      const string& path = myOSystem.bezelDir().getPath();
      VariantList metaData;
      int index = 0;

      do
      {
        imageName = getName(index);
        if(!imageName.empty())
        {
          // Note: JPG does not support transparency
          const string imagePath = path + imageName + ".png";
          const FSNode node(imagePath);
          if(node.exists())
          {
            isValid = true;
            PNGLibrary::loadImage(imagePath, *mySurface, metaData);
            break;
          }
        }
      } while(index != -1);
    }
    catch(const std::runtime_error& e) {
      cerr << "ERROR: Bezel load: " << e.what() << '\n';
    }
  }
#else
  const bool show = false;
#endif
  if(isValid)
  {
    const Int32 w = mySurface->width();
    const Int32 h = mySurface->height();
    uInt32 top{0}, bottom{0}, left{0}, right{0};

    if(settings.getBool("bezel.win.auto"))
    {
      // Determine transparent window inside bezel image
      const uInt32 xCenter = w >> 1;
      top = borderSize(xCenter, 0, h, w);
      bottom = h - 1 - borderSize(xCenter, h - 1, h, -w);
      const uInt32 yCenter = (bottom + top) >> 1;
      left = borderSize(0, yCenter, w, 1);
      right = w - 1 - borderSize(w - 1, yCenter, w, -1);
    }
    else
    {
      // BP: 13, 13,  0,  0%
      // HY: 12, 12,  0,  0%
      // P1: 25, 25, 11, 22%
      // P2: 23, 23,  7, 20%
      const auto bezelBorder = [&](Int32 dim, string_view key) {
        return std::min(dim - 1, static_cast<Int32>(
            std::lround(dim * settings.getInt(key) / 100.0)));
      };
      left   = bezelBorder(w, "bezel.win.left");
      right  = w - 1 - bezelBorder(w, "bezel.win.right");
      top    = bezelBorder(h, "bezel.win.top");
      bottom = h - 1 - bezelBorder(h, "bezel.win.bottom");
    }

    //cerr << (int)(right - left + 1) << " x " << (int)(bottom - top + 1) << " = "
    //  << double((int)(right - left + 1)) / double((int)(bottom - top + 1));

    // Disable bezel is no transparent window was found
    if(left < right && top < bottom)
      myInfo = Info(Common::Size(w, h), Common::Rect(left, top, right, bottom));
    else
    {
      if(mySurface)
        myFB.deallocateSurface(mySurface);
      mySurface = nullptr;
      myInfo = Info();
      myFB.showTextMessage("Invalid bezel image ('" + imageName + "')!");
      isValid = false;
    }
  }
  else
  {
    myInfo = Info();
    if(show)
      myFB.showTextMessage("No bezel image found");
  }
  return isValid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Bezel::apply()
{
  if(isShown())
  {
    const uInt32 bezelW =
      std::min(myFB.screenSize().w,
      static_cast<uInt32>(std::round(myFB.imageRect().w() * myInfo.ratioW())));
    const uInt32 bezelH =
      std::min(myFB.screenSize().h,
      static_cast<uInt32>(std::round(myFB.imageRect().h() * myInfo.ratioH())));

    // Position and scale bezel
    mySurface->setDstSize(bezelW, bezelH);
    mySurface->setDstPos((myFB.screenSize().w - bezelW) / 2, // center
                         (myFB.screenSize().h - bezelH) / 2);
    mySurface->setScalingInterpolation(ScalingInterpolation::sharp);
    // Note: Variable bezel window positions are handled in VideoModeHandler::Mode

    // Enable blending to allow overlaying the bezel over the TIA output
    mySurface->enableBlend(true);
    mySurface->setBlendLevel(100);
    mySurface->setVisible(true);
  }
  else
    if(mySurface)
      mySurface->setVisible(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Bezel::render()
{
  if(mySurface)
    mySurface->render();
}
