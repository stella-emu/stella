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

#ifndef TELEVISION_HXX
#define TELEVISION_HXX

class TIA;
class Console;
class OSystem;
class FBSurface;
class PaletteHandler;

#include <thread>

#include "Rect.hxx"
#include "FrameBuffer.hxx"
#include "NTSCSignal.hxx"
#include "PALSignal.hxx"
#include "PhosphorHandler.hxx"
#include "TVMode.hxx"
#include "TVSignal.hxx"
#include "bspf.hxx"
#include "TIAConstants.hxx"

/**
  Renders the TIA image to FBSurface's and presents the results to the screen.

  Television owns Stella's TIA→screen pipeline.  The three stages below model
  distinct physical domains and are applied in physical signal order; this
  class is the single place that holds them and sequences them (see render()).
  They communicate one-way, downstream — each stage's output is the next
  stage's input.

    1. COLORIMETRY  (PaletteHandler) — "what colour is each TIA value?"
       Owns the source palette and the per-colour controls applied once to the
       256-entry palette when it changes: phase shift, RGB calibration, hue,
       saturation, contrast, brightness, gamma.  Output: an adjusted RGB
       palette, the input to stage 2.

    2. SIGNAL TRANSPORT  (TVSignal → NTSC/PAL/SECAM) — "what do the wire and
       receiver do to it?"  Takes the adjusted palette and simulates the analog
       encode → modulate → demodulate → comb → bandwidth chain per pixel; the
       artifacts (dot crawl, fringing, colour loss) emerge from that physics.
       Receiver-side controls live here: sharpness, comb blend, colour-killer.
       Output: a decoded RGB frame, the input to stage 3.

    3. DISPLAY DEVICE  (PhosphorHandler + scanline surface + shade/mask) —
       "what does the tube do over time and space?"  Inter-frame phosphor
       persistence plus the scanline/aperture overlays, applied to the decoded
       RGB frame on its way to the screen.

  Boundary rule, when adding a new control: a knob describing the *source
  colour* of a TIA value is colorimetry (stage 1); a knob modelling the *wire
  or receiver* is signal (stage 2); a knob modelling the *display tube* is
  device (stage 3).  Keeping a control in its own domain is what stops the
  stages from quietly duplicating each other's colour/decode work.

  @author  Stephen Anthony
*/
class Television
{
  public:
    // Setting names of palette types
    static constexpr string_view SETTING_STANDARD = "standard";
    static constexpr string_view SETTING_THIN     = "thin";
    static constexpr string_view SETTING_PIXELS   = "pixels";
    static constexpr string_view SETTING_APERTURE = "aperture";
    static constexpr string_view SETTING_MAME     = "mame";

    /**
      Creates a new Television object
    */
    explicit Television(OSystem& system);
    ~Television();

    /**
      Set the TIA object, which is needed for actually rendering the TIA image.
    */
    void initialize(const Console& console, const VideoModeHandler::Mode& mode);

    /**
      Set the palette for TIA rendering.  This currently consists of two
      components: the actual TIA palette, and a mixed TIA palette used
      in phosphor mode.  The latter may eventually disappear once a better
      phosphor emulation is developed.

      @param tia_palette  An actual TIA palette, converted to data values
                          that are actually usable by the framebuffer
      @param rgb_palette  The RGB components of the palette, needed for
                          calculating a phosphor palette
    */
    void setPalette(const PaletteArray& tia_palette,
                    const PaletteArray& rgb_palette);

    /**
      Get a TIA surface that has no post-processing whatsoever.  This is
      currently used to save PNG image in the so-called '1x mode'.

      @param rect   Specifies the area in which the surface data is valid
    */
    const FBSurface& baseSurface(Common::Rect& rect) const;

    /**
      Get a underlying FBSurface that the TIA is being rendered into.
    */
    const FBSurface& tiaSurface() const { return *myTiaSurface; }

    /**
      Use the palette to map a single indexed pixel color. This is used by the
      TIA output widget.
     */
    uInt32 mapIndexedPixel(uInt8 indexedColor, uInt8 shift = 0) const;

    /**
      Use the TV filtering effects specified by the given preset.
    */
    void setTVMode(TVMode type, bool show = true);

    /**
      Switch to next/previous TV filtering effect.

      @param direction  +1 indicates increase, -1 indicates decrease.
    */
    void changeTVEffect(int direction = +1);

    /**
      Switch to next/previous TV filtering adjustable for the current timing.

      @param direction  +1 indicates increase, -1 indicates decrease.
    */
    void selectTVAdjustable(int direction = +1);

    /**
      Increase/decrease a specific TV filtering adjustable.
      Routes to the active timing's adjustable table (NTSC or PAL).

      @param adjustable  Index into the current timing's adjustable table
      @param direction   +1 indicates increase, -1 indicates decrease.
    */
    void changeTVAdjustable(int adjustable, int direction);

    /**
      Increase/decrease the currently-selected TV filtering adjustable.

      @param direction  +1 indicates increase, -1 indicates decrease.
    */
    void changeCurrentTVAdjustable(int direction = +1);

    /**
      Retrieve palette handler.
    */
    PaletteHandler& paletteHandler() const { return *myPaletteHandler; }

    /**
      Increase/decrease current scanline intensity by given relative amount.

      @param direction  +1 indicates increase, -1 indicates decrease.
    */
    void changeScanlineIntensity(int direction = +1);

    /**
      Cycle through available scanline masks.

      @param direction  +1 next mask, -1 mask.
    */
    void cycleScanlineMask(int direction = +1);

    /**
      Enable/disable/query phosphor effect.
    */
    void enablePhosphor(bool enable, int blend = -1);
    bool phosphorEnabled() const { return myPhosphorHandler.phosphorEnabled(); }

    /**
      Creates a scanline surface for the current TIA resolution
    */
    void createScanlineSurface();

    /**
      Resize/reset the TIA surface for the current signal quality setting,
      and apply the current scanline blend level.  Call after any change to
      the active preset or timing.
    */
    void enableTVEffects();
    bool tvEffectsEnabled() const {
      return myTVSignal->outputWidth() != TIAConstants::frameBufferWidth;
    }

    // Width produced by the active signal this frame; always <= maxRenderWidth().
    // Front-ends that read the raw TIA surface need this: the surface is
    // allocated at maxRenderWidth(), but only this many columns are filled.
    uInt32 outputWidth() const { return myTVSignal->outputWidth(); }

    // The widest output across all timings/modes; also the row stride (in
    // pixels) of the TIA render surface.
    static constexpr uInt32 maxRenderWidth() { return maxOutputWidth; }

    string effectsInfo() const;

    /**
      This method should be called to draw the TIA image(s) to the screen.
    */
    void render(bool shade = false);

    /**
      Update surface settings.
     */
    void updateSurfaceSettings();

    /**
      Prepare the current frame for taking a snapshot.
      In phosphor modes, blends current and previous frames for a better image.
     */
    void renderForSnapshot();

    /**
      Signal that a snapshot should be taken on the next render.
     */
    void saveSnapShot() { mySaveSnapFlag = true; }

  private:
    enum class ScanlineMask: uInt8 {
      Standard,
      Thin,
      Pixels,
      Aperture,
      Mame,
      NumMasks
    };

  private:
    // Is plain video mode enabled?
    bool correctAspect() const;

    // Convert scanline mask setting name into type
    ScanlineMask scanlineMaskType(int direction = 0);

  private:
    OSystem& myOSystem;
    FrameBuffer& myFB;
    TIA* myTIA{nullptr};

    shared_ptr<FBSurface> myTiaSurface, mySLineSurface,
                          myBaseTiaSurface, myShadeSurface;

    /////////////////////////////////////////////////////////////
    // Pipeline stage 3 (display device): phosphor persistence (also reduces
    // flicker on 30Hz screens), plus the scanline/shade surfaces above.
    PhosphorHandler myPhosphorHandler;

    // Phosphor blend
    int myPBlend{0};

    // The render surface and phosphor buffers must accommodate the widest
    // filter output across timings: NTSC Blargg (≈568) or PAL composite,
    // which renders to its 5× oversampled grid (800).
    static constexpr uInt32 maxOutputWidth =
      AtariNTSC::outWidth(TIAConstants::frameBufferWidth) >
        PALSignal::outWidth(TIAConstants::frameBufferWidth)
          ? AtariNTSC::outWidth(TIAConstants::frameBufferWidth)
          : PALSignal::outWidth(TIAConstants::frameBufferWidth);

    std::array<uInt32, static_cast<size_t>
      (maxOutputWidth * TIAConstants::frameBufferHeight)> myRGBFramebuffer0{};
    std::array<uInt32, static_cast<size_t>
      (maxOutputWidth * TIAConstants::frameBufferHeight)> myRGBFramebuffer1{};
    uInt32* myRGBFramebuffer{myRGBFramebuffer0.data()};
    uInt32* myPrevRGBFramebuffer{myRGBFramebuffer1.data()};
    /////////////////////////////////////////////////////////////

    // Use scanlines in TIA rendering mode
    bool myScanlinesEnabled{false};

    // Palette for normal TIA rendering mode
    PaletteArray myPalette{};

    // Pipeline stage 1 (colorimetry): source palette + per-colour controls
    unique_ptr<PaletteHandler> myPaletteHandler;

    // Pipeline stage 2 (signal transport): NTSC/PAL/SECAM signal simulation
    unique_ptr<TVSignal> myTVSignal;

    // Flag for saving a snapshot
    bool mySaveSnapFlag{false};

  private:
    // Following constructors and assignment operators not supported
    Television() = delete;
    Television(const Television&) = delete;
    Television(Television&&) = delete;
    Television& operator=(const Television&) = delete;
    Television& operator=(Television&&) = delete;
};

#endif  // TELEVISION_HXX
