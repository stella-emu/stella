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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "TIA.hxx"

#include "TIASurface.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASurface::TIASurface(FrameBuffer& buffer, OSystem& system)
  : myFB(buffer),
    myOSystem(system),
    myTIA(NULL),
    myTiaSurface(NULL),
    mySLineSurface(NULL),
    myFilterType(kNormal),
    myUsePhosphor(false),
    myPhosphorBlend(77),
    myScanlinesEnabled(false)
{
  // Load NTSC filter settings
  myNTSCFilter.loadConfig(myOSystem.settings());

  // Create a surface for the TIA image and scanlines; we'll need them eventually
  uInt32 tiaID = myFB.allocateSurface(ATARI_NTSC_OUT_WIDTH(160), 320);
  myTiaSurface = myFB.surface(tiaID);
  uInt32 scanID = myFB.allocateSurface(1, 320);
  mySLineSurface = myFB.surface(scanID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASurface::~TIASurface()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::initialize(const Console& console, const VideoMode& mode)
{
  myTIA = &(console.tia());


#if 0
// FIX HERE ///////////////////////////////////




#if 0
  bool enable = myProperties.get(Display_Phosphor) == "YES";
  int blend = atoi(myProperties.get(Display_PPBlend).c_str());
  myOSystem.frameBuffer().tiaSurface().enablePhosphor(enable, blend);
  myOSystem.frameBuffer().tiaSurface().setNTSC(
    (NTSCFilter::Preset)myOSystem.settings().getInt("tv.filter"), false);
#endif





  // Grab the initial height before it's updated below
  // We need it for the creating the TIA surface
  uInt32 baseHeight = mode.image.height() / mode.zoom;

  // The framebuffer only takes responsibility for TIA surfaces
  // Other surfaces (such as the ones used for dialogs) are allocated
  // in the Dialog class
  if(inTIAMode)
  {
    // Since we have free hardware stretching, the base TIA surface is created
    // only once, and its texture coordinates changed when we want to draw a
    // smaller or larger image
    if(!myTiaSurface)
      myTiaSurface = new FBSurfaceTIA(*this);

    myTiaSurface->updateCoords(baseHeight, mode.image.x(), mode.image.y(),
                               mode.image.width(), mode.image.height());

    myTiaSurface->enableScanlines(ntscEnabled());
    myTiaSurface->setTexInterpolation(myOSystem.settings().getBool("tia.inter"));
    myTiaSurface->setScanIntensity(myOSystem.settings().getInt("tv.scanlines"));
    myTiaSurface->setScanInterpolation(myOSystem.settings().getBool("tv.scaninter"));
    myTiaSurface->setTIA(myOSystem.console().tia());
  }
/////////////////////////////////////////////
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setPalette(const uInt32* tia_palette, const uInt32* rgb_palette)
{
  myPalette = tia_palette;

cerr << "TIASurface::setPalette\n";
  for(int i = 0; i < 256; ++i)
    cerr << myPalette[i] << " ";
  cerr << endl;

  // Set palette for phosphor effect
  for(int i = 0; i < 256; ++i)
  {
    for(int j = 0; j < 256; ++j)
    {
      uInt8 ri = (rgb_palette[i] >> 16) & 0xff;
      uInt8 gi = (rgb_palette[i] >> 8) & 0xff;
      uInt8 bi = rgb_palette[i] & 0xff;
      uInt8 rj = (rgb_palette[j] >> 16) & 0xff;
      uInt8 gj = (rgb_palette[j] >> 8) & 0xff;
      uInt8 bj = rgb_palette[j] & 0xff;

      Uint8 r = (Uint8) getPhosphor(ri, rj);
      Uint8 g = (Uint8) getPhosphor(gi, gj);
      Uint8 b = (Uint8) getPhosphor(bi, bj);

      myPhosphorPalette[i][j] = myFB.mapRGB(r, g, b);
    }
  }

  // The NTSC filtering needs access to the raw RGB data, since it calculates
  // its own internal palette
  myNTSCFilter.setTIAPalette(*this, rgb_palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIASurface::pixel(uInt32 idx, uInt8 shift) const
{
  uInt8 c = *(myTIA->currentFrameBuffer() + idx) | shift;
  uInt8 p = *(myTIA->previousFrameBuffer() + idx) | shift;

  return (!myUsePhosphor ? myPalette[c] : myPhosphorPalette[c][p]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setNTSC(NTSCFilter::Preset preset, bool show)
{
  ostringstream buf;
  if(preset == NTSCFilter::PRESET_OFF)
  {
    enableNTSC(false);
    buf << "TV filtering disabled";
  }
  else
  {
    enableNTSC(true);
    const string& mode = myNTSCFilter.setPreset(preset);
    buf << "TV filtering (" << mode << " mode)";
  }
  myOSystem.settings().setValue("tv.filter", (int)preset);

  if(show) myFB.showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setScanlineIntensity(int amount)
{
  ostringstream buf;
  if(ntscEnabled())
  {
    uInt32 intensity = enableScanlines(amount);
    buf << "Scanline intensity at " << intensity  << "%";
    myOSystem.settings().setValue("tv.scanlines", intensity);
  }
  else
    buf << "Scanlines only available in TV filtering mode";

  myFB.showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::toggleScanlineInterpolation()
{
  ostringstream buf;
  if(ntscEnabled())
  {
    bool enable = !myOSystem.settings().getBool("tv.scaninter");
    enableScanlineInterpolation(enable);
    buf << "Scanline interpolation " << (enable ? "enabled" : "disabled");
    myOSystem.settings().setValue("tv.scaninter", enable);
  }
  else
    buf << "Scanlines only available in TV filtering mode";

  myFB.showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIASurface::enableScanlines(int relative, int absolute)
{
  uInt32& intensity = mySLineSurface->myAttributes.blendalpha;
  if(relative == 0)  intensity = absolute;
  else               intensity += relative;
  intensity = BSPF_max(0u, intensity);
  intensity = BSPF_min(100u, intensity);

  mySLineSurface->applyAttributes();
//FIXSDL  myRedrawEntireFrame = true;
  return intensity;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enableScanlineInterpolation(bool enable)
{
  mySLineSurface->myAttributes.smoothing = enable;
  mySLineSurface->applyAttributes();
//FIXSDL  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;
  myFilterType = FilterType(enable ? myFilterType | 0x01 : myFilterType & 0x10);
//FIXSDL  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIASurface::getPhosphor(uInt8 c1, uInt8 c2) const
{
  if(c2 > c1)
    BSPF_swap(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enableNTSC(bool enable)
{
  myFilterType = FilterType(enable ? myFilterType | 0x10 : myFilterType & 0x01);

  // Normal vs NTSC mode uses different source widths
  const GUI::Rect& src = myTiaSurface->srcRect();
  myTiaSurface->setSrcSize(enable ? ATARI_NTSC_OUT_WIDTH(160) : 160, src.height());

  myTiaSurface->myAttributes.smoothing =
      myOSystem.settings().getBool("tia.inter");
  myTiaSurface->applyAttributes();

  myScanlinesEnabled = enable;
  mySLineSurface->myAttributes.smoothing =
      myOSystem.settings().getBool("tv.scaninter");
  mySLineSurface->myAttributes.blendalpha =
      myOSystem.settings().getInt("tv.scanlines");
  mySLineSurface->applyAttributes();

//FIXSDL  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIASurface::effectsInfo() const
{
  ostringstream buf;
  switch(myFilterType)
  {
    case kNormal:
      buf << "Disabled, normal mode";
      break;
    case kPhosphor:
      buf << "Disabled, phosphor mode";
      break;
    case kBlarggNormal:
      buf << myNTSCFilter.getPreset() << ", scanlines="
          << mySLineSurface->myAttributes.blendalpha << "/"
          << (mySLineSurface->myAttributes.smoothing ? "inter" : "nointer");
      break;
    case kBlarggPhosphor:
      buf << myNTSCFilter.getPreset() << ", phosphor, scanlines="
          << mySLineSurface->myAttributes.blendalpha << "/"
          << (mySLineSurface->myAttributes.smoothing ? "inter" : "nointer");
      break;
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::render()
{
#if 0
cerr << "src: " << myTiaSurface->srcRect() << endl
     << "dst: " << myTiaSurface->dstRect() << endl
     << endl;
#endif
  // Copy the mediasource framebuffer to the RGB texture
  // In hardware rendering mode, it's faster to just assume that the screen
  // is dirty and always do an update

  uInt8* currentFrame  = myTIA->currentFrameBuffer();
  uInt8* previousFrame = myTIA->previousFrameBuffer();
  uInt32 width         = myTIA->width();
  uInt32 height        = myTIA->height();

  uInt32 *buffer, pitch;
  myTiaSurface->basePtr(buffer, pitch);
//cerr << "buffer=" << buffer << ", pitch=" << pitch << endl;

  // TODO - Eventually 'phosphor' won't be a separate mode, and will become
  //        a post-processing filter by blending several frames.
  switch(myFilterType)
  {
    case kNormal:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
          buffer[pos++] = (uInt32) myPalette[currentFrame[bufofsY + x]];

        bufofsY    += width;
        screenofsY += pitch;
      }
      break;
    }
    case kPhosphor:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
        {
          const uInt32 bufofs = bufofsY + x;
          buffer[pos++] = (uInt32)
            myPhosphorPalette[currentFrame[bufofs]][previousFrame[bufofs]];
        }
        bufofsY    += width;
        screenofsY += pitch;
      }
      break;
    }
    case kBlarggNormal:
    {
      myNTSCFilter.blit_single(currentFrame, width, height,
                               buffer, pitch);
      break;
    }
    case kBlarggPhosphor:
    {
      myNTSCFilter.blit_double(currentFrame, previousFrame, width, height,
                               buffer, pitch);
      break;
    }
  }

  // Draw TIA image
  myTiaSurface->render();

  // Draw overlaying scanlines
  if(myScanlinesEnabled)
    mySLineSurface->render();
}
