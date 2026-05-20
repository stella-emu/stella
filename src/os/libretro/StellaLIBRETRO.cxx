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

#include "bspf.hxx"
#include "StellaLIBRETRO.hxx"
#include "SoundLIBRETRO.hxx"
#include "FBBackendLIBRETRO.hxx"
#include "FBSurfaceLIBRETRO.hxx"

#include "AtariNTSC.hxx"
#include "AudioSettings.hxx"
#include "CartDPC.hxx"
#include "PaletteHandler.hxx"
#include "Serializer.hxx"
#include "StateManager.hxx"
#include "Switches.hxx"
#include "TIA.hxx"
#include "TIASurface.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StellaLIBRETRO::StellaLIBRETRO()
  : rom_image(getROMMax()),
    audio_buffer{std::make_unique<Int16[]>(audio_buffer_max)}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StellaLIBRETRO::create(const SettingsLIBRETRO& cfg, bool logging)
{
  system_ready = false;

  // build play system
  destroy();

  myOSystem = std::make_unique<OSystemLIBRETRO>();

  const Settings::Options options;
  myOSystem->initialize(options);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  Settings& settings = myOSystem->settings();

  if(logging)
  {
    settings.setValue("loglevel", 999);
    settings.setValue("logtoconsole", true);
  }

  settings.setValue("speed", 1.0);

  settings.setValue("uimessages",       cfg.info_messages);
  settings.setValue("plr.extaccess",    cfg.info_messages);
  settings.setValue("plr.detectedinfo", cfg.info_messages);

  settings.setValue("detectpal60",  cfg.detect_pal60);
  settings.setValue("detectntsc50", cfg.detect_ntsc50);

  settings.setValue(AudioSettings::SETTING_DPC_PITCH, cfg.dpc_pitch);

  settings.setValue("pal.contrast",   cfg.pal_contrast);
  settings.setValue("pal.brightness", cfg.pal_brightness);
  settings.setValue("pal.hue",        cfg.pal_hue);
  settings.setValue("pal.saturation", cfg.pal_saturation);
  settings.setValue("pal.gamma",      cfg.pal_gamma);

  settings.setValue("format", cfg.console_format);
  settings.setValue("palette", cfg.video_palette);

  settings.setValue("tia.zoom", 1);
  settings.setValue("tia.vsizeadjust", 0);
  settings.setValue("tia.inter", false);

  //fastscbios
  // Fast loading of Supercharger BIOS

  settings.setValue("tv.filter", static_cast<int>(cfg.video_filter));

  settings.setValue("tv.phosphor", cfg.video_phosphor);
  settings.setValue("tv.phosblend", cfg.video_phosphor_blend);

  /*
  31440 rate

  fs:2 hz:50 bs:314.4 -- not supported,      0 frame lag ideal
  fs:128 hz:50 bs:4.9 -- lowest supported, 0-1 frame lag measured
  */
  settings.setValue(AudioSettings::SETTING_PRESET, static_cast<int>(AudioSettings::Preset::custom));
  settings.setValue(AudioSettings::SETTING_SAMPLE_RATE, getAudioRate());
  settings.setValue(AudioSettings::SETTING_BUFFER_SIZE, 8);
  settings.setValue(AudioSettings::SETTING_HEADROOM, 0);
  settings.setValue(AudioSettings::SETTING_RESAMPLING_QUALITY, static_cast<int>(AudioSettings::ResamplingQuality::nearestNeighbour));
  settings.setValue(AudioSettings::SETTING_VOLUME, 100);
  settings.setValue(AudioSettings::SETTING_STEREO, cfg.audio_mode);

  const FSNode rom(rom_path);

  if(!myOSystem->createConsole(rom).empty())
    return false;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  console_timing = myOSystem->console().timing();
  phosphor_default = myOSystem->frameBuffer().tiaSurface().phosphorEnabled();

  if(cfg.video_phosphor == "never") setVideoPhosphor("never", cfg.video_phosphor_blend);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  video_ready = false;
  audio_samples = 0;

  system_ready = true;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::destroy()
{
  system_ready = false;

  video_ready = false;
  audio_samples = 0;

  myOSystem.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::runFrame()
{
  // poll input right at vsync
  updateInput();

  // run vblank routine and draw frame
  updateVideo();

  // drain generated audio
  updateAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::updateInput()
{
  const Console& console = myOSystem->console();

  console.leftController().update();
  console.rightController().update();

  console.switches().update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::updateVideo()
{
  TIA& tia = myOSystem->console().tia();

  while (true)
  {
    tia.updateScanline();

    if(tia.scanlines() == 0) break;
  }

  video_ready = tia.newFramePending();

  if (video_ready)
  {
    FrameBuffer& frame = myOSystem->frameBuffer();

    tia.renderToFrameBuffer();
    frame.updateInEmulationMode(0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::updateAudio()
{
  static_cast<SoundLIBRETRO&>(myOSystem->sound()).dequeue(audio_buffer.get(), &audio_samples);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StellaLIBRETRO::loadState(const void* data, size_t size)
{
  Serializer state;

  state.putByteArray(std::span{reinterpret_cast<const uInt8*>(data), size});
  state.rewind();

  return myOSystem->state().loadState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StellaLIBRETRO::saveState(void* data, size_t size) const
{
  Serializer state;

  if (!myOSystem->state().saveState(state))
    return false;

  if (state.size() > size)
    return false;

  state.rewind();
  state.getByteArray(std::span{reinterpret_cast<uInt8*>(data), state.size()});
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t StellaLIBRETRO::getStateSize() const
{
  Serializer state;

  if (!myOSystem->state().saveState(state))
    return 0;

  return state.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float StellaLIBRETRO::getVideoAspectPar(uInt32 aspect_ntsc, uInt32 aspect_pal) const
{
  float par = 0.F;

  if (getVideoNTSC())
  {
    if (!aspect_ntsc)
      par = (6.1363635F / 3.579545454F) / 2.0;
    else
      par = aspect_ntsc / 100.0;
  }
  else
  {
    if (!aspect_pal)
      par = (7.3750000F / (4.43361875F * 4.F / 5.F)) / 2.F;
    else
      par = aspect_pal / 100.0;
  }

  return par;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void* StellaLIBRETRO::getVideoBuffer() const
{
  if (!render_surface)
  {
    const FBSurface& surface =
        myOSystem->frameBuffer().tiaSurface().tiaSurface();

    uInt32 pitch = 0;
    surface.basePtr(render_surface, pitch);
  }

  return render_surface;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StellaLIBRETRO::getVideoNTSC() const
{
  return myOSystem->console().gameRefreshRate() == 60;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StellaLIBRETRO::getVideoResize()
{
  if (render_width != getRenderWidth() || render_height != getRenderHeight())
  {
    render_width = getRenderWidth();
    render_height = getRenderHeight();

    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setROM(const char* path, const void* data, size_t size)
{
  rom_path = path;

  memcpy(rom_image.data(), data, size);

  rom_size = static_cast<uInt32>(size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setVideoFilter(NTSCFilter::Preset mode)
{
  if (system_ready)
  {
    myOSystem->settings().setValue("tv.filter", static_cast<int>(mode));
    myOSystem->frameBuffer().tiaSurface().setNTSC(mode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setVideoPalette(string_view mode)
{
  if (system_ready)
  {
    myOSystem->settings().setValue("palette", mode);
    myOSystem->frameBuffer().tiaSurface().paletteHandler().setPalette(string{mode});
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setVideoPhosphor(string_view phosphor, uInt32 blend)
{
  if (system_ready)
  {
    myOSystem->settings().setValue("tv.phosphor", phosphor);
    myOSystem->settings().setValue("tv.phosblend", blend);

    if(phosphor == "byrom")
      myOSystem->frameBuffer().tiaSurface().enablePhosphor(phosphor_default, blend);
    else if(phosphor == "never")
      myOSystem->frameBuffer().tiaSurface().enablePhosphor(false, blend);
    else
      myOSystem->frameBuffer().tiaSurface().enablePhosphor(true, blend);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setMessages(bool enabled)
{
  if(system_ready)
  {
    Settings& settings = myOSystem->settings();
    settings.setValue("uimessages",       enabled);
    settings.setValue("plr.extaccess",    enabled);
    settings.setValue("plr.detectedinfo", enabled);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setPaletteAdjust(float contrast, float brightness, float hue,
                                       float saturation, float gamma)
{
  if(system_ready)
  {
    Settings& settings = myOSystem->settings();
    settings.setValue("pal.contrast",   contrast);
    settings.setValue("pal.brightness", brightness);
    settings.setValue("pal.hue",        hue);
    settings.setValue("pal.saturation", saturation);
    settings.setValue("pal.gamma",      gamma);

    PaletteHandler& ph = myOSystem->frameBuffer().tiaSurface().paletteHandler();
    ph.loadConfig(settings);
    ph.setPalette();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setDpcPitch(uInt32 pitch)
{
  if(system_ready)
  {
    myOSystem->settings().setValue(AudioSettings::SETTING_DPC_PITCH, pitch);

    if(myOSystem->console().cartridge().name() == "CartridgeDPC")
      static_cast<CartridgeDPC&>(myOSystem->console().cartridge()).setDpcPitch(pitch);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaLIBRETRO::setAudioStereo(string_view mode)
{
  if (system_ready)
  {
    myOSystem->settings().setValue(AudioSettings::SETTING_STEREO, mode);
    myOSystem->console().initializeAudio();
  }
}
