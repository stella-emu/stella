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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef STELLA_LIBRETRO_HXX
#define STELLA_LIBRETRO_HXX

#include "bspf.hxx"
#include "OSystemLIBRETRO.hxx"

#include "Console.hxx"
#include "ConsoleTiming.hxx"
#include "Control.hxx"
#include "EmulationTiming.hxx"
#include "EventHandler.hxx"
#include "M6532.hxx"
#include "Paddles.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "TIASurface.hxx"


/**
  This class wraps Stella core for easier libretro maintenance
*/
class StellaLIBRETRO
{
  public:
    StellaLIBRETRO();
    virtual ~StellaLIBRETRO() = default;

  public:
    OSystemLIBRETRO& osystem() { return *myOSystem; }

    bool create(bool logging);
    void destroy();
    void reset() { myOSystem->console().system().reset(); }

    void runFrame();

    bool loadState(const void* data, size_t size);
    bool saveState(void* data, size_t size);

  public:
    const char* getCoreName() { return "Stella"; }
    const char* getROMExtensions() { return "a26|bin"; }

    void*  getROM() { return rom_image.get(); }
    uInt32 getROMSize() { return rom_size; }
    uInt32 getROMMax() { return 512 * 1024; }

    //uInt8* getRAM() { return myOSystem->console().system().m6532().getRAM(); }
    //uInt32 getRAMSize() { return 128; }

    size_t getStateSize();

    bool   getConsoleNTSC() { return console_timing == ConsoleTiming::ntsc; }

    float  getVideoAspect();
    bool   getVideoNTSC();
    float  getVideoRate() { return getVideoNTSC() ? 60.0 : 50.0; }

    bool   getVideoReady() { return video_ready; }
    uInt32 getVideoZoom() { return myOSystem->frameBuffer().tiaSurface().ntscEnabled() ? 2 : 1; }
    bool   getVideoResize();

    void*  getVideoBuffer();
    uInt32 getVideoWidth() { return getVideoZoom()==1 ? myOSystem->console().tia().width() : getVideoWidthMax(); }
    uInt32 getVideoHeight() { return myOSystem->console().tia().height(); }
    uInt32 getVideoPitch() { return getVideoWidthMax() * 4; }

    uInt32 getVideoWidthMax() { return AtariNTSC::outWidth(160); }
    uInt32 getVideoHeightMax() { return 312; }

    uInt32 getRenderWidth() { return getVideoZoom()==1 ? myOSystem->console().tia().width() * 2 : getVideoWidthMax(); }
    uInt32 getRenderHeight() { return myOSystem->console().tia().height() * getVideoZoom(); }

    float  getAudioRate() { return getConsoleNTSC() ? (262 * 76 * 60) / 38.0 : (312 * 76 * 50) / 38.0; }
    bool   getAudioReady() { return audio_samples > 0; }
    uInt32 getAudioSize() { return audio_samples; }

    Int16* getAudioBuffer() { return audio_buffer.get(); }

  public:
    void   setROM(const void* data, size_t size);

    void   setConsoleFormat(uInt32 mode);

    void   setVideoAspectNTSC(uInt32 value) { video_aspect_ntsc = value; };
    void   setVideoAspectPAL(uInt32 value) { video_aspect_pal = value; };

    void   setVideoFilter(uInt32 mode);
    void   setVideoPalette(uInt32 mode);
    void   setVideoPhosphor(uInt32 mode, uInt32 blend);

    void   setAudioStereo(int mode);

    void   setInputEvent(Event::Type type, Int32 state) { myOSystem->eventHandler().handleEvent(type, state); }

	Controller::Type   getLeftControllerType() { return myOSystem->console().leftController().type(); }
	Controller::Type   getRightControllerType() { return myOSystem->console().rightController().type(); }

	void   setPaddleJoypadSensitivity(int sensitivity)
	{
		if(getLeftControllerType() == Controller::Type::Paddles)
			static_cast<Paddles&>(myOSystem->console().leftController()).setDigitalSensitivity(sensitivity);
		if(getRightControllerType() == Controller::Type::Paddles)
			static_cast<Paddles&>(myOSystem->console().rightController()).setDigitalSensitivity(sensitivity);
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
    uInt32 system_ready;

    unique_ptr<uInt8[]> rom_image;
    uInt32 rom_size;

    ConsoleTiming console_timing;
    string console_format;

    uInt32 render_width, render_height;

    bool video_ready;

    unique_ptr<Int16[]> audio_buffer;
    uInt32 audio_samples;

	// (31440 rate / 50 Hz) * 16-bit stereo * 1.25x padding
    static const uInt32 audio_buffer_max = (31440 / 50 * 4 * 5) / 4;

  private:
    string video_palette;
    string video_phosphor;
    uInt32 video_phosphor_blend;

    uInt32 video_aspect_ntsc;
    uInt32 video_aspect_pal;
    NTSCFilter::Preset video_filter;

    string audio_mode;

    bool phosphor_default;
};

#endif
