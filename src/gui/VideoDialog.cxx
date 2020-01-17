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

#include <cmath>

#include "bspf.hxx"
#include "Control.hxx"
#include "Dialog.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "TabWidget.hxx"
#include "NTSCFilter.hxx"
#include "TIASurface.hxx"

#include "VideoDialog.hxx"

namespace {
  // Emulation speed is a positive float that multiplies the framerate. However, the UI controls
  // adjust speed in terms of a speedup factor (1/10, 1/9 .. 1/2, 1, 2, 3, .., 10). The following
  // mapping and formatting functions implement this conversion. The speedup factor is represented
  // by an integer value between -900 and 900 (0 means no speedup).

  constexpr int MAX_SPEED = 900;
  constexpr int MIN_SPEED = -900;
  constexpr int SPEED_STEP = 10;

  int mapSpeed(float speed)
  {
    speed = std::abs(speed);

    return BSPF::clamp(
      static_cast<int>(round(100 * (speed >= 1 ? speed - 1 : -1 / speed + 1))),
      MIN_SPEED, MAX_SPEED
    );
  }

  float unmapSpeed(int speed)
  {
    float f_speed = static_cast<float>(speed) / 100;

    return speed < 0 ? -1 / (f_speed - 1) : 1 + f_speed;
  }

  string formatSpeed(int speed) {
    stringstream ss;

    ss
      << std::setw(3) << std::fixed << std::setprecision(0)
      << (unmapSpeed(speed) * 100);

    return ss.str();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::VideoDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Video settings")
{
  const int VGAP = 4;
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int INDENT = 20;
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos, tabID;
  int lwidth = font.getStringWidth("V-Size adjust "),
    pwidth = font.getStringWidth("XXXXxXXXX"),
    swidth = font.getMaxCharWidth() * 10 - 2;

  WidgetArray wid;
  VariantList items;

  // Set real dimensions
  setSize(60 * fontWidth + HBORDER * 2, 14 * (lineHeight + VGAP) + 14 + _th, max_w, max_h);

  // The tab widget
  xpos = 2;  ypos = 4;
  myTab = new TabWidget(this, font, xpos, ypos + _th, _w - 2*xpos, _h - _th - buttonHeight - 20);
  addTabWidget(myTab);

  xpos = HBORDER;  ypos = VBORDER;
  //////////////////////////////////////////////////////////
  // 1) General options
  tabID = myTab->addTab(" General ");

  // Video renderer
  myRenderer = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                               instance().frameBuffer().supportedRenderers(),
                               "Renderer ", lwidth);
  wid.push_back(myRenderer);
  ypos += lineHeight + VGAP;

  // TIA Palette
  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Z26", "z26");
  VarList::push_back(items, "User", "user");
  myTIAPalette = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                                 lineHeight, items, "Palette ", lwidth);
  wid.push_back(myTIAPalette);
  ypos += lineHeight + VGAP;

  // TIA interpolation
  myTIAInterpolate = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Interpolation ");
  wid.push_back(myTIAInterpolate);  ypos += lineHeight + VGAP;


  // TIA zoom levels (will be dynamically filled later)
  myTIAZoom = new SliderWidget(myTab, font, xpos, ypos - 1, swidth, lineHeight,
    "Zoom ", lwidth, 0, fontWidth * 4, "%");
  myTIAZoom->setMinValue(200); myTIAZoom->setStepValue(FrameBuffer::ZOOM_STEPS * 100);
  wid.push_back(myTIAZoom);
  ypos += lineHeight + VGAP;

  // Aspect ratio (NTSC mode)
  myVSizeAdjust =
    new SliderWidget(myTab, font, xpos, ypos-1, swidth, lineHeight,
                     "V-Size adjust", lwidth, kVSizeChanged, fontWidth * 7, "%", 0, true);
  myVSizeAdjust->setMinValue(-5); myVSizeAdjust->setMaxValue(5);
  myVSizeAdjust->setTickmarkIntervals(2);
  wid.push_back(myVSizeAdjust);
  ypos += lineHeight + VGAP;

  // Speed
  mySpeed =
    new SliderWidget(myTab, font, xpos, ypos-1, swidth, lineHeight,
                     "Emul. speed ", lwidth, kSpeedupChanged, fontWidth * 5, "%");
  mySpeed->setMinValue(MIN_SPEED); mySpeed->setMaxValue(MAX_SPEED);
  mySpeed->setStepValue(SPEED_STEP);
  mySpeed->setTickmarkIntervals(2);
  wid.push_back(mySpeed);
  ypos += lineHeight + VGAP;

  // Use sync to vblank
  myUseVSync = new CheckboxWidget(myTab, font, xpos, ypos + 1, "VSync");
  wid.push_back(myUseVSync);

  // Move over to the next column
  xpos += mySpeed->getWidth() + 44;
  ypos = VBORDER;

  // Fullscreen
  myFullscreen = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Fullscreen", kFullScreenChanged);
  wid.push_back(myFullscreen);
  ypos += lineHeight + VGAP;

  /*pwidth = font.getStringWidth("0: 3840x2860@120Hz");
  myFullScreenMode = new PopUpWidget(myTab, font, xpos + INDENT + 2, ypos, pwidth, lineHeight,
  instance().frameBuffer().supportedScreenModes(), "Mode ");
  wid.push_back(myFullScreenMode);
  ypos += lineHeight + VGAP;*/

  // FS stretch
  myUseStretch = new CheckboxWidget(myTab, font, xpos + INDENT, ypos + 1, "Stretch");
  wid.push_back(myUseStretch);
  ypos += lineHeight + VGAP;

  // FS overscan
  myTVOverscan = new SliderWidget(myTab, font, xpos + INDENT, ypos - 1, swidth, lineHeight,
    "Overscan", font.getStringWidth("Overscan "), kOverscanChanged, fontWidth * 3, "%");
  myTVOverscan->setMinValue(0); myTVOverscan->setMaxValue(10);
  myTVOverscan->setTickmarkIntervals(2);
  wid.push_back(myTVOverscan);
  ypos += (lineHeight + VGAP) * 2;

  // Skip progress load bars for SuperCharger ROMs
  // Doesn't really belong here, but I couldn't find a better place for it
  myFastSCBios = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Fast SuperCharger load");
  wid.push_back(myFastSCBios);
  ypos += lineHeight + VGAP;

  // Show UI messages onscreen
  myUIMessages = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Show UI messages");
  wid.push_back(myUIMessages);
  ypos += lineHeight + VGAP;

  // Center window (in windowed mode)
  myCenter = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Center window");
  wid.push_back(myCenter);
  ypos += (lineHeight + VGAP) * 2;

  // Use multi-threading
  myUseThreads = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Multi-threading");
  wid.push_back(myUseThreads);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////
  // 2) TV effects options
  wid.clear();
  tabID = myTab->addTab(" TV Effects ");
  xpos = HBORDER;
  ypos = VBORDER;
  swidth = font.getMaxCharWidth() * 8 - 4;

  // TV Mode
  items.clear();
  VarList::push_back(items, "Disabled", static_cast<uInt32>(NTSCFilter::Preset::OFF));
  VarList::push_back(items, "RGB", static_cast<uInt32>(NTSCFilter::Preset::RGB));
  VarList::push_back(items, "S-Video", static_cast<uInt32>(NTSCFilter::Preset::SVIDEO));
  VarList::push_back(items, "Composite", static_cast<uInt32>(NTSCFilter::Preset::COMPOSITE));
  VarList::push_back(items, "Bad adjust", static_cast<uInt32>(NTSCFilter::Preset::BAD));
  VarList::push_back(items, "Custom", static_cast<uInt32>(NTSCFilter::Preset::CUSTOM));
  lwidth = font.getStringWidth("TV Mode ");
  pwidth = font.getStringWidth("Bad adjust");
  myTVMode =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                    items, "TV mode ", lwidth, kTVModeChanged);
  wid.push_back(myTVMode);
  ypos += lineHeight + VGAP;

  // Custom adjustables (using macro voodoo)
  xpos += INDENT - 2; ypos += 0;
  lwidth = font.getStringWidth("Saturation ");

#define CREATE_CUSTOM_SLIDERS(obj, desc, cmd)                            \
  myTV ## obj =                                                          \
    new SliderWidget(myTab, font, xpos, ypos-1, swidth, lineHeight,      \
                     desc, lwidth, cmd, fontWidth*4, "%");               \
  myTV ## obj->setMinValue(0); myTV ## obj->setMaxValue(100);            \
  myTV ## obj->setTickmarkIntervals(2);                                  \
  wid.push_back(myTV ## obj);                                            \
  ypos += lineHeight + VGAP;

  CREATE_CUSTOM_SLIDERS(Contrast, "Contrast ", 0)
  CREATE_CUSTOM_SLIDERS(Bright, "Brightness ", 0)
  CREATE_CUSTOM_SLIDERS(Hue, "Hue ", 0)
  CREATE_CUSTOM_SLIDERS(Satur, "Saturation ", 0)
  CREATE_CUSTOM_SLIDERS(Gamma, "Gamma ", 0)
  CREATE_CUSTOM_SLIDERS(Sharp, "Sharpness ", 0)
  CREATE_CUSTOM_SLIDERS(Res, "Resolution ", 0)
  CREATE_CUSTOM_SLIDERS(Artifacts, "Artifacts ", 0)
  CREATE_CUSTOM_SLIDERS(Fringe, "Fringing ", 0)
  CREATE_CUSTOM_SLIDERS(Bleed, "Bleeding ", 0)

  xpos += myTVContrast->getWidth() + 30;
  ypos = VBORDER;

  lwidth = font.getStringWidth("Intensity ");

  // TV Phosphor effect
  myTVPhosphor = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Phosphor for all ROMs", kPhosphorChanged);
  wid.push_back(myTVPhosphor);
  ypos += lineHeight + VGAP / 2;

  // TV Phosphor blend level
  xpos += INDENT;
  swidth = font.getMaxCharWidth() * 10;
  CREATE_CUSTOM_SLIDERS(PhosLevel, "Blend     ", kPhosBlendChanged)
  ypos += 8;

  // Scanline intensity and interpolation
  xpos -= INDENT;
  myTVScanLabel = new StaticTextWidget(myTab, font, xpos, ypos, "Scanlines:");
  ypos += lineHeight + VGAP / 2;

  xpos += INDENT;
  CREATE_CUSTOM_SLIDERS(ScanIntense, "Intensity ", kScanlinesChanged)
  ypos += lineHeight + 2;

  // Adjustable presets
  xpos -= INDENT;
  int cloneWidth = font.getStringWidth("Clone Bad Adjust") + 20;
#define CREATE_CLONE_BUTTON(obj, desc)                                 \
  myClone ## obj =                                                     \
    new ButtonWidget(myTab, font, xpos, ypos, cloneWidth, buttonHeight,\
                     desc, kClone ## obj ##Cmd);                       \
  wid.push_back(myClone ## obj);                                       \
  ypos += lineHeight + 4 + VGAP;

  ypos += VGAP;
  CREATE_CLONE_BUTTON(RGB, "Clone RGB")
  CREATE_CLONE_BUTTON(Svideo, "Clone S-Video")
  CREATE_CLONE_BUTTON(Composite, "Clone Composite")
  CREATE_CLONE_BUTTON(Bad, "Clone Bad adjust")
  CREATE_CLONE_BUTTON(Custom, "Revert")

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  // Disable certain functions when we know they aren't present
#ifndef WINDOWED_SUPPORT
  myFullscreen->clearFlags(Widget::FLAG_ENABLED);
  myCenter->clearFlags(Widget::FLAG_ENABLED);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::loadConfig()
{
  // Renderer settings
  myRenderer->setSelected(instance().settings().getString("video"), "default");

  // TIA zoom levels
  // These are dynamically loaded, since they depend on the size of
  // the desktop and which renderer we're using
  float minZoom = instance().frameBuffer().supportedTIAMinZoom(); // or 2 if we allow lower values
  float maxZoom = instance().frameBuffer().supportedTIAMaxZoom();

  myTIAZoom->setMinValue(minZoom * 100);
  myTIAZoom->setMaxValue(maxZoom * 100);
  myTIAZoom->setTickmarkIntervals((maxZoom - minZoom) * 2); // every ~50%
  myTIAZoom->setValue(instance().settings().getFloat("tia.zoom") * 100);

  // TIA Palette
  myTIAPalette->setSelected(
    instance().settings().getString("palette"), "standard");

  // TIA interpolation
  myTIAInterpolate->setState(instance().settings().getBool("tia.inter"));

  // Aspect ratio setting (NTSC and PAL)
  myVSizeAdjust->setValue(instance().settings().getInt("tia.vsizeadjust"));

  // Emulation speed
  int speed = mapSpeed(instance().settings().getFloat("speed"));
  mySpeed->setValue(speed);
  mySpeed->setValueLabel(formatSpeed(speed));

  // Fullscreen
  myFullscreen->setState(instance().settings().getBool("fullscreen"));
  /*string mode = instance().settings().getString("fullscreenmode");
  myFullScreenMode->setSelected(mode);*/
  // Fullscreen stretch setting
  myUseStretch->setState(instance().settings().getBool("tia.fs_stretch"));
  // Fullscreen overscan setting
  myTVOverscan->setValue(instance().settings().getInt("tia.fs_overscan"));
  handleFullScreenChange();

  // Use sync to vertical blank
  myUseVSync->setState(instance().settings().getBool("vsync"));

  // Show UI messages
  myUIMessages->setState(instance().settings().getBool("uimessages"));

  // Center window
  myCenter->setState(instance().settings().getBool("center"));

  // Fast loading of Supercharger BIOS
  myFastSCBios->setState(instance().settings().getBool("fastscbios"));

  // Multi-threaded rendering
  myUseThreads->setState(instance().settings().getBool("threads"));

  // TV Mode
  myTVMode->setSelected(
    instance().settings().getString("tv.filter"), "0");
  int preset = instance().settings().getInt("tv.filter");
  handleTVModeChange(NTSCFilter::Preset(preset));

  // TV Custom adjustables
  loadTVAdjustables(NTSCFilter::Preset::CUSTOM);

  // TV phosphor mode
  myTVPhosphor->setState(instance().settings().getString("tv.phosphor") == "always");

  // TV phosphor blend
  myTVPhosLevel->setValue(instance().settings().getInt("tv.phosblend"));
  handlePhosphorChange();

  // TV scanline intensity and interpolation
  myTVScanIntense->setValue(instance().settings().getInt("tv.scanlines"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::saveConfig()
{
  // Renderer setting
  instance().settings().setValue("video",
                                 myRenderer->getSelectedTag().toString());

  // TIA zoom levels
  instance().settings().setValue("tia.zoom", myTIAZoom->getValue() / 100.0);

  // TIA Palette
  instance().settings().setValue("palette",
                                 myTIAPalette->getSelectedTag().toString());

  // TIA interpolation
  instance().settings().setValue("tia.inter", myTIAInterpolate->getState());

  // Aspect ratio setting (NTSC and PAL)
  int oldAdjust = instance().settings().getInt("tia.vsizeadjust");
  int newAdjust = myVSizeAdjust->getValue();
  bool initializeVideo = oldAdjust != newAdjust;

  instance().settings().setValue("tia.vsizeadjust", newAdjust);

  // Speed
  int speedup = mySpeed->getValue();
  instance().settings().setValue("speed", unmapSpeed(speedup));
  if (instance().hasConsole()) instance().console().initializeAudio();

  // Fullscreen
  instance().settings().setValue("fullscreen", myFullscreen->getState());
  // Fullscreen stretch setting
  instance().settings().setValue("tia.fs_stretch", myUseStretch->getState());
  // Fullscreen overscan
  instance().settings().setValue("tia.fs_overscan", myTVOverscan->getValueLabel());

  // Use sync to vertical blank
  instance().settings().setValue("vsync", myUseVSync->getState());

  // Show UI messages
  instance().settings().setValue("uimessages", myUIMessages->getState());

  // Center window
  instance().settings().setValue("center", myCenter->getState());

  // Fast loading of Supercharger BIOS
  instance().settings().setValue("fastscbios", myFastSCBios->getState());

  // Multi-threaded rendering
  instance().settings().setValue("threads", myUseThreads->getState());
  if (instance().hasConsole())
    instance().frameBuffer().tiaSurface().ntsc().enableThreading(myUseThreads->getState());

  // TV Mode
  instance().settings().setValue("tv.filter",
                                 myTVMode->getSelectedTag().toString());

  // TV Custom adjustables
  NTSCFilter::Adjustable adj;
  adj.hue = myTVHue->getValue();
  adj.saturation = myTVSatur->getValue();
  adj.contrast = myTVContrast->getValue();
  adj.brightness = myTVBright->getValue();
  adj.sharpness = myTVSharp->getValue();
  adj.gamma = myTVGamma->getValue();
  adj.resolution = myTVRes->getValue();
  adj.artifacts = myTVArtifacts->getValue();
  adj.fringing = myTVFringe->getValue();
  adj.bleed = myTVBleed->getValue();
  instance().frameBuffer().tiaSurface().ntsc().setCustomAdjustables(adj);

  // TV phosphor mode
  instance().settings().setValue("tv.phosphor",
                                 myTVPhosphor->getState() ? "always" : "byrom");
  // TV phosphor blend
  instance().settings().setValue("tv.phosblend", myTVPhosLevel->getValueLabel());

  // TV scanline intensity
  instance().settings().setValue("tv.scanlines", myTVScanIntense->getValueLabel());

  if (instance().hasConsole())
  {
    instance().console().setTIAProperties();
    // TODO: display the new screen (currently all blank)
    if (initializeVideo)
      instance().console().initializeVideo();
  }

  // Finally, issue a complete framebuffer re-initialization...
  instance().createFrameBuffer();

  // ... and apply potential setting changes to the TIA surface
  instance().frameBuffer().tiaSurface().updateSurfaceSettings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // General
    {
      myRenderer->setSelectedIndex(0);
      myTIAZoom->setValue(300);
      myTIAPalette->setSelected("standard", "");
      myTIAInterpolate->setState(false);
      myVSizeAdjust->setValue(0);
      mySpeed->setValue(0);

      myFullscreen->setState(false);
      //myFullScreenMode->setSelectedIndex(0);
      myUseStretch->setState(false);
      myUseVSync->setState(true);
      myUIMessages->setState(true);
      myCenter->setState(false);
      myFastSCBios->setState(true);
      myUseThreads->setState(false);
      break;
    }

    case 1:  // TV effects
    {
      myTVMode->setSelected("0", "0");

      // TV phosphor mode
      myTVPhosphor->setState(false);

      // TV phosphor blend
      myTVPhosLevel->setValue(50);

      // TV scanline intensity and interpolation
      myTVScanIntense->setValue(25);

      // Make sure that mutually-exclusive items are not enabled at the same time
      handleTVModeChange(NTSCFilter::Preset::OFF);
      handlePhosphorChange();
      loadTVAdjustables(NTSCFilter::Preset::CUSTOM);
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleTVModeChange(NTSCFilter::Preset preset)
{
  bool enable = preset == NTSCFilter::Preset::CUSTOM;

  myTVSharp->setEnabled(enable);
  myTVHue->setEnabled(enable);
  myTVRes->setEnabled(enable);
  myTVArtifacts->setEnabled(enable);
  myTVFringe->setEnabled(enable);
  myTVBleed->setEnabled(enable);
  myTVBright->setEnabled(enable);
  myTVContrast->setEnabled(enable);
  myTVSatur->setEnabled(enable);
  myTVGamma->setEnabled(enable);
  myCloneComposite->setEnabled(enable);
  myCloneSvideo->setEnabled(enable);
  myCloneRGB->setEnabled(enable);
  myCloneBad->setEnabled(enable);
  myCloneCustom->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::loadTVAdjustables(NTSCFilter::Preset preset)
{
  NTSCFilter::Adjustable adj;
  instance().frameBuffer().tiaSurface().ntsc().getAdjustables(
      adj, NTSCFilter::Preset(preset));
  myTVSharp->setValue(adj.sharpness);
  myTVHue->setValue(adj.hue);
  myTVRes->setValue(adj.resolution);
  myTVArtifacts->setValue(adj.artifacts);
  myTVFringe->setValue(adj.fringing);
  myTVBleed->setValue(adj.bleed);
  myTVBright->setValue(adj.brightness);
  myTVContrast->setValue(adj.contrast);
  myTVSatur->setValue(adj.saturation);
  myTVGamma->setValue(adj.gamma);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleFullScreenChange()
{
  bool enable = myFullscreen->getState();
  myUseStretch->setEnabled(enable);
  myTVOverscan->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleOverscanChange()
{
  if (myTVOverscan->getValue() == 0)
  {
    myTVOverscan->setValueLabel("Off");
    myTVOverscan->setValueUnit("");
  }
  else
    myTVOverscan->setValueUnit("%");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handlePhosphorChange()
{
  myTVPhosLevel->setEnabled(myTVPhosphor->getState());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kVSizeChanged:
    {
      int adjust = myVSizeAdjust->getValue();

      if (!adjust)
      {
        myVSizeAdjust->setValueLabel("Default");
        myVSizeAdjust->setValueUnit("");
      }
      else
        myVSizeAdjust->setValueUnit("%");
      break;
    }

    case kSpeedupChanged:
      mySpeed->setValueLabel(formatSpeed(mySpeed->getValue()));
      break;

    case kFullScreenChanged:
      handleFullScreenChange();
      break;

    case kOverscanChanged:
      handleOverscanChange();
      break;

    case kTVModeChanged:
      handleTVModeChange(NTSCFilter::Preset(myTVMode->getSelectedTag().toInt()));
      break;

    case kCloneCompositeCmd: loadTVAdjustables(NTSCFilter::Preset::COMPOSITE);
      break;
    case kCloneSvideoCmd: loadTVAdjustables(NTSCFilter::Preset::SVIDEO);
      break;
    case kCloneRGBCmd: loadTVAdjustables(NTSCFilter::Preset::RGB);
      break;
    case kCloneBadCmd: loadTVAdjustables(NTSCFilter::Preset::BAD);
      break;
    case kCloneCustomCmd: loadTVAdjustables(NTSCFilter::Preset::CUSTOM);
      break;

    case kScanlinesChanged:
      if (myTVScanIntense->getValue() == 0)
      {
        myTVScanIntense->setValueLabel("Off");
        myTVScanIntense->setValueUnit("");
      }
      else
        myTVScanIntense->setValueUnit("%");
      break;

    case kPhosphorChanged:
      handlePhosphorChange();
      break;

    case kPhosBlendChanged:
      if (myTVPhosLevel->getValue() == 0)
      {
        myTVPhosLevel->setValueLabel("Off");
        myTVPhosLevel->setValueUnit("");
      }
      else
        myTVPhosLevel->setValueUnit("%");
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
