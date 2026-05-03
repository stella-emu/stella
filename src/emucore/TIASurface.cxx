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

#include "FBSurface.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "PNGLibrary.hxx"
#include "PaletteHandler.hxx"
#include "TIASurface.hxx"

namespace {
  ScalingInterpolation interpolationModeFromSettings(const Settings& settings)
  {
    return settings.getBool("tia.inter")
      ? ScalingInterpolation::blur
      : ScalingInterpolation::sharp;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASurface::TIASurface(OSystem& system)
  : myOSystem{system},
    myFB{system.frameBuffer()}
{
  // Load NTSC filter settings
  NTSCFilter::loadConfig(myOSystem.settings());

  // Create a surface for the TIA image and scanlines; we'll need them eventually
  myTiaSurface = myFB.allocateSurface(
    AtariNTSC::outWidth(TIAConstants::frameBufferWidth),
    TIAConstants::frameBufferHeight,
    !correctAspect()
      ? ScalingInterpolation::none
      : interpolationModeFromSettings(myOSystem.settings())
  );

  // Base TIA surface for use in taking snapshots in 1x mode
  myBaseTiaSurface = myFB.allocateSurface(TIAConstants::frameBufferWidth*2,
                                          TIAConstants::frameBufferHeight);

  // Create shading surface
  static constexpr uInt32 data = 0xff000000;
  myShadeSurface = myFB.allocateSurface(1, 1, ScalingInterpolation::sharp, &data);
  myShadeSurface->enableBlend(true);
  myShadeSurface->setBlendLevel(35); // darken stopped emulation by 35%

  myRGBFramebuffer0.fill(0);
  myRGBFramebuffer1.fill(0);

  // Enable/disable threading in the NTSC TV effects renderer
  myNTSCFilter.enableThreading(myOSystem.settings().getBool("threads"));

  myPaletteHandler = std::make_unique<PaletteHandler>(myOSystem);
  myPaletteHandler->loadConfig(myOSystem.settings());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASurface::~TIASurface() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::initialize(const Console& console,
                            const VideoModeHandler::Mode& mode)
{
  myTIA = &(console.tia());

  myTiaSurface->setDstPos(mode.imageR.x(), mode.imageR.y());
  myTiaSurface->setDstSize(mode.imageR.w(), mode.imageR.h());

  myPaletteHandler->setPalette();

  createScanlineSurface();
  setNTSC(static_cast<NTSCFilter::Preset>(myOSystem.settings().getInt("tv.filter")), false);

#if 0
cerr << "INITIALIZE:\n"
     << "TIA:\n"
     << "src: " << myTiaSurface->srcRect() << '\n'
     << "dst: " << myTiaSurface->dstRect() << "\n\n"
     << "SLine:\n"
     << "src: " << mySLineSurface->srcRect() << '\n'
     << "dst: " << mySLineSurface->dstRect() << "\n\n";
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setPalette(const PaletteArray& tia_palette,
                            const PaletteArray& rgb_palette)
{
  myPalette = tia_palette;

  // The NTSC filtering needs access to the raw RGB data, since it calculates
  // its own internal palette
  myNTSCFilter.setPalette(rgb_palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FBSurface& TIASurface::baseSurface(Common::Rect& rect) const
{
  const uInt32 tiaw = myTIA->width(), width = tiaw * 2, height = myTIA->height();
  rect.setBounds(0, 0, width, height);

  // Fill the surface with pixels from the TIA, scaled 2x horizontally
  uInt32 *buf_ptr{nullptr}, pitch{0};  // NOLINT (erroneously marked as const)
  myBaseTiaSurface->basePtr(buf_ptr, pitch);

  for(size_t y = 0; y < height; ++y)
    for(size_t x = 0; x < width; ++x)
        *buf_ptr++ = myPalette[*(myTIA->frameBuffer() + y * tiaw + x / 2)];

  return *myBaseTiaSurface;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIASurface::mapIndexedPixel(uInt8 indexedColor, uInt8 shift) const
{
  return myPalette[indexedColor | shift];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setNTSC(NTSCFilter::Preset preset, bool show)
{
  if(preset == NTSCFilter::Preset::OFF)
  {
    enableNTSC(false);
    if(show) myFB.showTextMessage("TV filtering disabled");
  }
  else
  {
    enableNTSC(true);
    const string& mode = myNTSCFilter.setPreset(preset);
    if(show) myFB.showTextMessage(std::format("TV filtering ({} mode)", mode));
  }
  myOSystem.settings().setValue("tv.filter", static_cast<int>(preset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::changeNTSC(int direction)
{
  static constexpr std::array<NTSCFilter::Preset, 6> PRESETS = {
    NTSCFilter::Preset::OFF, NTSCFilter::Preset::RGB, NTSCFilter::Preset::SVIDEO,
    NTSCFilter::Preset::COMPOSITE, NTSCFilter::Preset::BAD, NTSCFilter::Preset::CUSTOM
  };
  int preset = myOSystem.settings().getInt("tv.filter");

  if(direction == +1)
  {
    if(preset == static_cast<int>(NTSCFilter::Preset::CUSTOM))
      preset = static_cast<int>(NTSCFilter::Preset::OFF);
    else
      preset++;
  }
  else if(direction == -1)
  {
    if(preset == static_cast<int>(NTSCFilter::Preset::OFF))
      preset = static_cast<int>(NTSCFilter::Preset::CUSTOM);
    else
      preset--;
  }
  setNTSC(PRESETS[preset], true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setNTSCAdjustable(int direction)
{
  string text, valueText;
  Int32 value{0};

  setNTSC(NTSCFilter::Preset::CUSTOM);
  ntsc().selectAdjustable(direction, text, valueText, value);
  myOSystem.frameBuffer().showGaugeMessage(text, valueText, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::changeNTSCAdjustable(int adjustable, int direction)
{
  string text, valueText;
  Int32 newValue{0};

  setNTSC(NTSCFilter::Preset::CUSTOM);
  ntsc().changeAdjustable(adjustable, direction, text, valueText, newValue);
  NTSCFilter::saveConfig(myOSystem.settings());
  myOSystem.frameBuffer().showGaugeMessage(text, valueText, newValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::changeCurrentNTSCAdjustable(int direction)
{
  string text, valueText;
  Int32 newValue{0};

  setNTSC(NTSCFilter::Preset::CUSTOM);
  ntsc().changeCurrentAdjustable(direction, text, valueText, newValue);
  NTSCFilter::saveConfig(myOSystem.settings());
  myOSystem.frameBuffer().showGaugeMessage(text, valueText, newValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::changeScanlineIntensity(int direction)
{
  const int intensity =
      BSPF::clamp<int>(mySLineSurface->blendLevel() + direction * 2, 0, 100);
  mySLineSurface->setBlendLevel(intensity);

  myOSystem.settings().setValue("tv.scanlines", intensity);
  enableNTSC(ntscEnabled());

  myFB.showGaugeMessage("Scanline intensity",
    intensity ? std::format("{}%", intensity) : "Off", intensity);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASurface::ScanlineMask TIASurface::scanlineMaskType(int direction)
{
  static constexpr
  std::array<string_view, static_cast<int>(ScanlineMask::NumMasks)> Masks = {
    SETTING_STANDARD,
    SETTING_THIN,
    SETTING_PIXELS,
    SETTING_APERTURE,
    SETTING_MAME
  };
  int i = 0;
  const string& name = myOSystem.settings().getString("tv.scanmask");

  for(const auto mask: Masks)
  {
    if(mask == name)
    {
      if(direction)
      {
        i = BSPF::clampw(i + direction, 0, static_cast<int>(ScanlineMask::NumMasks) - 1);
        myOSystem.settings().setValue("tv.scanmask", Masks[i]);
      }
      return static_cast<ScanlineMask>(i);
    }
    ++i;
  }
  return ScanlineMask::Standard;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::cycleScanlineMask(int direction)
{
  static constexpr
  std::array<string_view, static_cast<int>(ScanlineMask::NumMasks)> Names = {
    "Standard",
    "Thin lines",
    "Pixelated",
    "Aperture Grille",
    "MAME"
  };
  const int i = static_cast<int>(scanlineMaskType(direction));

  if(direction)
    createScanlineSurface();

  myOSystem.frameBuffer().showTextMessage(
    std::format("Scanline data '{}'", Names[i]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enablePhosphor(bool enable, int blend)
{
  if(myPhosphorHandler.initialize(enable, blend))
  {
    myPBlend = blend;
    myFilter = static_cast<Filter>(
        enable ? static_cast<uInt8>(myFilter) | 0x01
               : static_cast<uInt8>(myFilter) & 0x10);
    myRGBFramebuffer0.fill(0);
    myRGBFramebuffer1.fill(0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::createScanlineSurface()
{
  // Idea: Emulate
  // - Dot-Trio Shadow-Mask
  // - Slot-Mask (NEC 'CromaClear')
  // - Aperture-Grille (Sony 'Trinitron')

  using Data = std::vector<std::vector<uInt32>>;

  struct Pattern
  {
    uInt32 vRepeats{1}; // number of vertically repeated data blocks (adding a bit of variation)
    Data   data;

    explicit Pattern(uInt32 c_vRepeats, const Data& c_data)
      : vRepeats(c_vRepeats), data(c_data)
    {}
  };
  static const std::array<Pattern, static_cast<int>(ScanlineMask::NumMasks)> Patterns = {{
    Pattern(1,  // standard
    {
      { 0x00000000 },
      { 0xaa000000 } // 0xff decreased to 0xaa (67%) to match overall brightness of other pattern
    }),
    Pattern(1,  // thin
    {
      { 0x00000000 },
      { 0x00000000 },
      { 0xff000000 }
    }),
    Pattern(2,  // pixel
    {
      // orignal data from https://forum.arcadeotaku.com/posting.php?mode=quote&f=10&p=134359
      //{ 0x08ffffff, 0x02ffffff, 0x80e7e7e7 },
      //{ 0x08ffffff, 0x80e7e7e7, 0x40ffffff },
      //{ 0xff282828, 0xff282828, 0xff282828 },
      //{ 0x80e7e7e7, 0x04ffffff, 0x04ffffff },
      //{ 0x04ffffff, 0x80e7e7e7, 0x20ffffff },
      //{ 0xff282828, 0xff282828, 0xff282828 },
      // same but using RGB = 0,0,0
      //{ 0x08000000, 0x02000000, 0x80000000 },
      //{ 0x08000000, 0x80000000, 0x40000000 },
      //{ 0xff000000, 0xff000000, 0xff000000 },
      //{ 0x80000000, 0x04000000, 0x04000000 },
      //{ 0x04000000, 0x80000000, 0x20000000 },
      //{ 0xff000000, 0xff000000, 0xff000000 },
      // brightened
      { 0x06000000, 0x01000000, 0x5a000000 },
      { 0x06000000, 0x5a000000, 0x3d000000 },
      { 0xb4000000, 0xb4000000, 0xb4000000 },
      { 0x5a000000, 0x03000000, 0x03000000 },
      { 0x03000000, 0x5a000000, 0x17000000 },
      { 0xb4000000, 0xb4000000, 0xb4000000 }
    }),
    Pattern(1,  // aperture
    {
      //{ 0x2cf31d00, 0x1500ffce, 0x1c1200a4 },
      //{ 0x557e0f00, 0x40005044, 0x45070067 },
      //{ 0x800c0200, 0x89000606, 0x8d02000d },
      // (doubled & darkened, alpha */ 1.75)
      { 0x19f31d00, 0x0c00ffce, 0x101200a4, 0x19f31d00, 0x0c00ffce, 0x101200a4 },
      { 0x317e0f00, 0x25005044, 0x37070067, 0x317e0f00, 0x25005044, 0x37070067 },
      { 0xe00c0200, 0xf0000606, 0xf702000d, 0xe00c0200, 0xf0000606, 0xf702000d },
    }),
    Pattern(3,  // mame
    {
      // original tile data from https://wiki.arcadeotaku.com/w/MAME_CRT_Simulation
      //{ 0xffb4b4b4, 0xffa5a5a5, 0xffc3c3c3 },
      //{ 0xffffffff, 0xfff0f0f0, 0xfff0f0f0 },
      //{ 0xfff0f0f0, 0xffffffff, 0xffe1e1e1 },
      //{ 0xff000000, 0xff000000, 0xff000000 },
      //{ 0xffa5a5a5, 0xffc3c3c3, 0xffb4b4b4 },
      //{ 0xfff0f0f0, 0xfff0f0f0, 0xffffffff },
      //{ 0xffffffff, 0xffe1e1e1, 0xfff0f0f0 },
      //{ 0xff000000, 0xff000000, 0xff000000 },
      //{ 0xffc3c3c3, 0xffb4b4b4, 0xffa5a5a5 },
      //{ 0xfff0f0f0, 0xffffffff, 0xfff0f0f0 },
      //{ 0xffe1e1e1, 0xfff0f0f0, 0xffffffff },
      //{ 0xff000000, 0xff000000, 0xff000000 },
      // MAME tile RGB values inverted into alpha channel
      { 0x4b000000, 0x5a000000, 0x3c000000 },
      { 0x00000000, 0x0f000000, 0x0f000000 },
      { 0x0f000000, 0x00000000, 0x1e000000 },
      { 0xff000000, 0xff000000, 0xff000000 },
      { 0x5a000000, 0x3c000000, 0x4b000000 },
      { 0x0f000000, 0x0f000000, 0x00000000 },
      { 0x00000000, 0x1e000000, 0x0f000000 },
      { 0xff000000, 0xff000000, 0xff000000 },
      { 0x3c000000, 0x4b000000, 0x5a000000 },
      { 0x0f000000, 0x00000000, 0x0f000000 },
      { 0x1e000000, 0x0f000000, 0x00000000 },
      { 0xff000000, 0xff000000, 0xff000000 },
    }),
  }};
  const auto mask = static_cast<int>(scanlineMaskType());
  const auto pWidth = static_cast<uInt32>(Patterns[mask].data[0].size());
  const auto pHeight = static_cast<uInt32>(Patterns[mask].data.size() / Patterns[mask].vRepeats);
  const auto vRepeats = Patterns[mask].vRepeats;

  // Single width pattern need no horizontal repeats
  const uInt32 width = pWidth > 1 ? TIAConstants::frameBufferWidth * pWidth : 1;

  // TODO: Idea, alternative mask pattern if destination is scaled smaller than mask height?
  const uInt32 height = myTIA->height()* pHeight; // vRepeats are not used here

  // Copy repeated pattern into surface data
  std::vector<uInt32> data(static_cast<size_t>(width) * height);

  for(uInt32 i = 0; i < width * height; ++i)
    data[i] = Patterns[mask].data[(i / width) % (pHeight * vRepeats)][i % pWidth];

  myFB.deallocateSurface(mySLineSurface);
  mySLineSurface = myFB.allocateSurface(width, height,
    interpolationModeFromSettings(myOSystem.settings()), data.data());
  mySLineSurface->enableBlend(true);

  //mySLineSurface->setSrcSize(mySLineSurface->width(), height);
  mySLineSurface->setDstRect(myTiaSurface->dstRect());
  updateSurfaceSettings();

  enableNTSC(ntscEnabled());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enableNTSC(bool enable)
{
  myFilter = static_cast<Filter>(
      enable ? static_cast<uInt8>(myFilter) | 0x10
             : static_cast<uInt8>(myFilter) & 0x01);

  const uInt32 surfaceWidth = enable ?
    AtariNTSC::outWidth(TIAConstants::frameBufferWidth) : TIAConstants::frameBufferWidth;

  if(surfaceWidth != myTiaSurface->srcRect().w() || myTIA->height() != myTiaSurface->srcRect().h())
  {
    myTiaSurface->setSrcSize(surfaceWidth, myTIA->height());
    myTiaSurface->invalidate();
  }

  // Generate a scanline surface from current scanline pattern
  // Apply current blend to scan line surface
  const int scanlines = myOSystem.settings().getInt("tv.scanlines");
  myScanlinesEnabled = scanlines > 0;
  mySLineSurface->setBlendLevel(scanlines);

  myRGBFramebuffer0.fill(0);
  myRGBFramebuffer1.fill(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIASurface::effectsInfo() const
{
  string buf;
  switch(myFilter)
  {
    case Filter::Normal:
      buf = "Disabled, normal mode";
      break;
    case Filter::Phosphor:
      buf = std::format("Disabled, phosphor={}", myPBlend);
      break;
    case Filter::BlarggNormal:
      buf = myNTSCFilter.getPreset();
      break;
    case Filter::BlarggPhosphor:
      buf = std::format("{}, phosphor={}", myNTSCFilter.getPreset(), myPBlend);
      break;
    default:
      break;  // Not supposed to get here
  }
  if(mySLineSurface->blendLevel() > 0)
    buf += std::format(", scanlines={}/{}",
      mySLineSurface->blendLevel(),
      myOSystem.settings().getString("tv.scanmask"));

  buf += std::format(", inter={}, aspect correction={}, palette={}",
    myOSystem.settings().getBool("tia.inter") ? "enabled" : "disabled",
    correctAspect() ? "enabled" : "disabled",
    myOSystem.settings().getString("palette"));

  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::render(bool shade)
{
  const uInt32 width = myTIA->width(), height = myTIA->height();
  uInt32 *out{nullptr}, outPitch{0};
  myTiaSurface->basePtr(out, outPitch);

  switch(myFilter)
  {
    case Filter::Normal:
    {
      const uInt8* tiaIn = myTIA->frameBuffer();
      uInt32 bufofs = 0, screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = width / 2; x; --x)
        {
          out[pos++] = myPalette[tiaIn[bufofs++]];
          out[pos++] = myPalette[tiaIn[bufofs++]];
        }
        screenofsY += outPitch;
      }
      break;
    }

    case Filter::Phosphor:
    {
      const uInt8* tiaIn = myTIA->frameBuffer();

      if(mySaveSnapFlag)
        std::swap(myRGBFramebuffer, myPrevRGBFramebuffer);

      uInt32* rgbIn = myRGBFramebuffer;
      uInt32 bufofs = 0, screenofsY = 0;
      for(uInt32 y = height; y; --y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = width / 2; x; --x)
        {
          // Store back into displayed frame buffer (for next frame)
          rgbIn[bufofs] = out[pos++] =
            PhosphorHandler::getPixel(myPalette[tiaIn[bufofs]], rgbIn[bufofs]);
          ++bufofs;
          rgbIn[bufofs] = out[pos++] =
            PhosphorHandler::getPixel(myPalette[tiaIn[bufofs]], rgbIn[bufofs]);
          ++bufofs;
        }
        screenofsY += outPitch;
      }
      break;
    }

    case Filter::BlarggNormal:
    {
      myNTSCFilter.render(myTIA->frameBuffer(), width, height, out, outPitch << 2);
      break;
    }

    case Filter::BlarggPhosphor:
    {
      if(mySaveSnapFlag)
        std::swap(myRGBFramebuffer, myPrevRGBFramebuffer);

      myNTSCFilter.render(myTIA->frameBuffer(), width, height, out, outPitch << 2,
                          myRGBFramebuffer);
      break;
    }

    default:
      break;  // Not supposed to get here
  }

  // Draw TIA image
  myTiaSurface->render();

  // Draw overlaying scanlines
  if(myScanlinesEnabled)
    mySLineSurface->render();

  if(shade)
  {
    myShadeSurface->setDstRect(myTiaSurface->dstRect());
    myShadeSurface->render();
  }

  if(mySaveSnapFlag)
  {
    mySaveSnapFlag = false;
  #ifdef IMAGE_SUPPORT
    myOSystem.png().takeSnapshot();
  #endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::renderForSnapshot()
{
  const uInt32 width = myTIA->width(), height = myTIA->height();
  uInt32 pos{0};
  uInt32 *outPtr{nullptr}, outPitch{0};
  myTiaSurface->basePtr(outPtr, outPitch);

  mySaveSnapFlag = false;

  switch(myFilter)
  {
    case Filter::Normal:
    case Filter::BlarggNormal:
      render();
      break;

    case Filter::Phosphor:
    {
      uInt32 bufofs = 0, screenofsY = 0;
      for(uInt32 y = height; y; --y)
      {
        pos = screenofsY;
        for(uInt32 x = width / 2; x; --x)
        {
          outPtr[pos++] = averageBuffers(bufofs++);
          outPtr[pos++] = averageBuffers(bufofs++);
        }
        screenofsY += outPitch;
      }
      break;
    }

    case Filter::BlarggPhosphor:
    {
      uInt32 bufofs = 0;
      for(uInt32 y = height; y; --y)
        for(uInt32 x = outPitch; x; --x)
          outPtr[pos++] = averageBuffers(bufofs++);
      break;
    }

    default:
      break;  // Not supposed to get here
  }

  if(myPhosphorHandler.phosphorEnabled())
  {
    myTiaSurface->render();
    if(myScanlinesEnabled)
      mySLineSurface->render();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::updateSurfaceSettings()
{
  if(myTiaSurface != nullptr)
    myTiaSurface->setScalingInterpolation(
      interpolationModeFromSettings(myOSystem.settings()));

  if(mySLineSurface != nullptr)
    mySLineSurface->setScalingInterpolation(
      interpolationModeFromSettings(myOSystem.settings()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIASurface::correctAspect() const
{
  return myOSystem.settings().getBool("tia.correct_aspect");
}
