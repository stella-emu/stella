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

#ifndef STELLA_LIBRETRO_HXX
#define STELLA_LIBRETRO_HXX

#include "bspf.hxx"
#include "OSystemLIBRETRO.hxx"
#include "SettingsLIBRETRO.hxx"

#include "Cart.hxx"
#include "Console.hxx"
#include "ConsoleTiming.hxx"
#include "Control.hxx"
#include "EmulationTiming.hxx"
#include "EventHandler.hxx"
#include "M6532.hxx"
#include "Paddles.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "Television.hxx"

/**
  This class wraps Stella core for easier libretro maintenance
*/
class StellaLIBRETRO
{
  public:
    StellaLIBRETRO();
    ~StellaLIBRETRO() = default;

  public:
    OSystemLIBRETRO& osystem() const { return *myOSystem; }

    bool create(const SettingsLIBRETRO& cfg, bool logging);
    void destroy();
    void reset() { myOSystem->console().system().reset(); }

    void runFrame();

    bool loadState(const void* data, size_t size);
    bool saveState(void* data, size_t size) const;

  public:
    static constexpr const char* getCoreName() { return "Stella"; }
    static constexpr const char* getROMExtensions() { return "a26|bin"; }

    const void*  getROM() const { return rom_image.data(); }
    uInt32 getROMSize() const { return rom_size; }
    static constexpr uInt32 getROMMax() {
      return static_cast<uInt32>(Cartridge::maxSize());
    }

    uInt8* getRAM() {
      return myOSystem->console().system().m6532().getRAM().data();
    }
    static constexpr uInt32 getRAMSize() { return 128; }

    size_t getStateSize() const;

    bool   getConsoleNTSC() const { return console_timing == ConsoleTiming::ntsc; }

    float  getVideoAspectPar(uInt32 aspect_ntsc, uInt32 aspect_pal) const;
    bool   getVideoNTSC() const;
    float  getVideoRate() const { return getVideoNTSC() ? 60.0 : 50.0; }

    bool   getVideoReady() const { return video_ready; }
    uInt32 getVideoZoom() const {
      return myOSystem->frameBuffer().television().tvEffectsEnabled() ? 2 : 1;
    }
    bool   getVideoResize();

    void*  getVideoBuffer() const;
    uInt32 getVideoWidth() const {
      return getVideoZoom() == 1 ? myOSystem->console().tia().width() : getVideoWidthMax();
    }
    uInt32 getVideoHeight() const {
      return myOSystem->console().tia().height();
    }
    static constexpr uInt32 getVideoPitch() { return getVideoWidthMax() * 4; }

    static constexpr uInt32 getVideoWidthMax()  { return AtariNTSC::outWidth(160); }
    static constexpr uInt32 getVideoHeightMax() { return 312; }

    uInt32 getRenderWidth() const {
      return getVideoZoom() == 1 ? myOSystem->console().tia().width() * 2
                                 : getVideoWidthMax();
    }
    uInt32 getRenderHeight() const {
      return myOSystem->console().tia().height() * getVideoZoom();
    }

    const Common::Rect& getImageRect() const {
      return myOSystem->frameBuffer().imageRect();
    }

    float  getAudioRate() const {
      return getConsoleNTSC() ? (262 * 76 * 60) / 38.0 : (312 * 76 * 50) / 38.0;
    }
    bool   getAudioReady() const { return audio_samples > 0; }
    uInt32 getAudioSize() const  { return audio_samples; }

    Int16* getAudioBuffer() { return audio_buffer.get(); }

  public:
    void   setROM(const char* path, const void* data, size_t size);

    void   setVideoFilter(TVMode mode);
    void   setVideoPalette(string_view mode);
    void   setVideoPhosphor(string_view phosphor, uInt32 blend);

    void   setAudioStereo(string_view mode);
    void   setMessages(bool enabled);

    void   setDpcPitch(uInt32 pitch);

    void   setPaletteAdjust(float contrast, float brightness, float hue,
                            float saturation, float gamma);

    void   setInputEvent(Event::Type type, Int32 state) {
             myOSystem->eventHandler().handleEvent(type, state);
    }

    // Drain input through the input window, so the controllers can replay it
    // within the window (the input slice of poll(), since libretro does its
    // own frame housekeeping)
    void   pollInput() {
             myOSystem->eventHandler().pollInput();
    }

    bool isSystemReady() const { return system_ready; }

    Controller::Type getLeftControllerType() const {
      return myOSystem->console().leftController().type();
    }
    Controller::Type getRightControllerType() const {
      return myOSystem->console().rightController().type();
    }

    void setPaddleJoypadSensitivity(int sensitivity) const
    {
      if(getLeftControllerType() == Controller::Type::Paddles ||
         getRightControllerType() == Controller::Type::Paddles)
        Paddles::setDigitalSensitivity(sensitivity);
    }

    void setPaddleAnalogSensitivity(int sensitivity) const
    {
      if(getLeftControllerType() == Controller::Type::Paddles ||
         getRightControllerType() == Controller::Type::Paddles)
        Paddles::setAnalogSensitivity(sensitivity);
    }

  protected:
    void   updateInput();
    void   updateVideo();
    void   updateAudio();

  private:
    // Following constructors and assignment operators not supported
    StellaLIBRETRO(const StellaLIBRETRO&) = delete;
    StellaLIBRETRO(StellaLIBRETRO&&) = delete;
    StellaLIBRETRO& operator=(const StellaLIBRETRO&) = delete;
    StellaLIBRETRO& operator=(StellaLIBRETRO&&) = delete;

    unique_ptr<OSystemLIBRETRO> myOSystem;
    uInt32 system_ready{false};

    ByteArray rom_image;
    uInt32 rom_size{0};
    string rom_path;

    ConsoleTiming console_timing{ConsoleTiming::ntsc};

    mutable uInt32* render_surface{nullptr};
    uInt32 render_width{0}, render_height{0};

    bool video_ready{false};

    unique_ptr<Int16[]> audio_buffer;
    uInt32 audio_samples{0};

    // (31440 rate / 50 Hz) * 16-bit stereo * 1.25x padding
    static constexpr uInt32 audio_buffer_max = (31440 / 50 * 4 * 5) / 4;

    bool phosphor_default{false};
};

#endif  // STELLA_LIBRETRO_HXX
