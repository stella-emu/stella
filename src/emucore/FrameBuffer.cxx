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
#include "Logger.hxx"

#include "Console.hxx"
#include "EventHandler.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"
#include "Sound.hxx"
#include "AudioSettings.hxx"
#include "MediaFactory.hxx"
#include "PNGLibrary.hxx"

#include "FBSurface.hxx"
#include "TIASurface.hxx"
#include "Bezel.hxx"
#include "FrameBuffer.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#ifdef GUI_SUPPORT
  #include "Font.hxx"
  #include "StellaFont.hxx"
  #include "ConsoleMediumFont.hxx"
  #include "ConsoleMediumBFont.hxx"
  #include "StellaMediumFont.hxx"
  #include "StellaLargeFont.hxx"
  #include "Stella12x24tFont.hxx"
  #include "Stella14x28tFont.hxx"
  #include "Stella16x32tFont.hxx"
  #include "ConsoleFont.hxx"
  #include "Launcher.hxx"
  #include "DialogContainer.hxx"
  #include "TimeMachine.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem& osystem)
  : myOSystem{osystem},
    myMsgHandler{*this, osystem}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::initialize()
{
  // First create the platform-specific backend; it is needed before anything
  // else can be used
  myBackend = MediaFactory::createVideoBackend(myOSystem);

  // Get desktop resolution and supported renderers
  myBackend->queryHardware(myFullscreenDisplays, myWindowedDisplays, myRenderers);

  for(const auto& display: myWindowedDisplays)
  {
    uInt32 query_w = display.second.w, query_h = display.second.h;

    // Check the 'maxres' setting, which is an undocumented developer feature
    // that specifies the desktop size (not normally set)
    const Common::Size& s = myOSystem.settings().getSize("maxres");
    if(s.valid())
    {
      query_w = s.w;
      query_h = s.h;
    }
    // Various parts of the codebase assume a minimum screen size
    Common::Size size(std::max(query_w, FBMinimum::Width), std::max(query_h, FBMinimum::Height));
    myAbsDesktopSize[display.first] = size;

    // Check for HiDPI mode (is it activated, and can we use it?)
    const bool hidpi = (((size.w / 2) >= FBMinimum::Width) &&
                        ((size.h / 2) >= FBMinimum::Height));
    myHiDPIAllowed[display.first] = hidpi;
    myHiDPIEnabled[display.first] = hidpi && myOSystem.settings().getBool("hidpi");

    // In HiDPI mode, the desktop resolution is essentially halved
    // Later, the output is scaled and rendered in 2x mode
    if(myHiDPIEnabled[display.first])
    {
      size.w /= hidpiScaleFactor();
      size.h /= hidpiScaleFactor();
    }
    myDesktopSize[display.first] = size;
  }

#ifdef GUI_SUPPORT
  setupFonts();
#endif

  updateTheme();
  setUIPalette();

  myGrabMouse = myOSystem.settings().getBool("grabmouse");

  // Create a TIA surface; we need it for rendering TIA images
  myTIASurface = std::make_unique<TIASurface>(myOSystem);
  // Create a bezel surface for TIA overlays
  myBezel = std::make_unique<Bezel>(myOSystem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::displayId(BufferType bufferType) const
{
  uInt32 display = 0;

  if(bufferType == myWindow.bufferType || bufferType == BufferType::None)
    display = myBackend->getCurrentDisplayID();
  else
    display = myOSystem.settings().getInt(
      getDisplayKey(bufferType != BufferType::None
        ? bufferType
        : myWindow.bufferType)
    );

  // If the requested display ID is not available, default to the first one
  // in the container (normally the primary display)
  if(!myWindowedDisplays.contains(display))
    display = myWindowedDisplays.begin()->first;

  return display;
}

#ifdef GUI_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setupFonts()
{
  ////////////////////////////////////////////////////////////////////
  // Create fonts to draw text
  // NOTE: the logic determining appropriate font sizes is done here,
  //       so that the UI classes can just use the font they expect,
  //       and not worry about it
  //       This logic should also take into account the size of the
  //       framebuffer, and try to be intelligent about font sizes
  //       We can probably add ifdefs to take care of corner cases,
  //       but that means we've failed to abstract it enough ...
  ////////////////////////////////////////////////////////////////////

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = std::make_unique<GUI::Font>(GUI::stellaDesc); // 6x10

  const string_view dialogFont = myOSystem.settings().getString("dialogfont");
  const FontDesc fd = getFontDesc(dialogFont);

  // The general font used in all UI elements
  myFont = std::make_unique<GUI::Font>(fd);                                //  default: 9x18
  // The info font used in all UI elements,
  //  automatically determined aiming for 1 / 1.4 (~= 18 / 13) size
  myInfoFont = std::make_unique<GUI::Font>(infoFontDesc(fd));             //  default 8x13

  // Determine minimal zoom level based on the default font
  //  So what fits with default font should fit for any font.
  //  However, we have to make sure all Dialogs are sized using the fontsize.
  const int zoom_h = (fd.height * 4 * 2) / GUI::stellaMediumDesc.height;
  const int zoom_w = (fd.maxwidth * 4 * 2) / GUI::stellaMediumDesc.maxwidth;
  // round to 25% steps, >= 200%
  myTIAMinZoom = std::max(std::max(zoom_w, zoom_h) / 4., 2.);

  // The font used by the ROM launcher
  const string_view lf = myOSystem.settings().getString("launcherfont");

  myLauncherFont = std::make_unique<GUI::Font>(getFontDesc(lf));       //  8x13
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FontDesc FrameBuffer::getFontDesc(string_view name)
{
  if(name == "small")
    return GUI::consoleDesc;        //  8x13
  else if(name == "low_medium")
    return GUI::consoleMediumBDesc; //  9x15
  else if(name == "medium")
    return GUI::stellaMediumDesc;   //  9x18
  else if(name == "large" || name == "large10")
    return GUI::stellaLargeDesc;    // 10x20
  else if(name == "large12")
    return GUI::stella12x24tDesc;   // 12x24
  else if(name == "large14")
    return GUI::stella14x28tDesc;   // 14x28
  else // "large16"
    return GUI::stella16x32tDesc;   // 16x32
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FontDesc FrameBuffer::infoFontDesc(const FontDesc& fd)
{
  constexpr int NUM_FONTS = 7;
  const FontDesc FONT_DESC[NUM_FONTS] = {
    GUI::consoleDesc, GUI::consoleMediumDesc, GUI::stellaMediumDesc,
    GUI::stellaLargeDesc, GUI::stella12x24tDesc, GUI::stella14x28tDesc,
    GUI::stella16x32tDesc};

  int fontIdx = 0;
  for(int i = 0; i < NUM_FONTS; ++i)
  {
    if(fd.height <= FONT_DESC[i].height * 1.4)
    {
      fontIdx = i;
      break;
    }
  }
  return FONT_DESC[fontIdx];
}
#endif  // GUI_SUPPORT

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::createDisplay(string_view title, BufferType type,
                                        Common::Size size, bool honourHiDPI)
{
  ++myInitializedCount;
  myBackend->setTitle(title);

  // Always save, maybe only the mode of the window has changed
  saveCurrentWindowPosition();
  myWindow.bufferType = type;

  // In HiDPI mode, all created displays must be scaled appropriately
  if(honourHiDPI && hidpiEnabled())
  {
    size.w *= hidpiScaleFactor();
    size.h *= hidpiScaleFactor();
  }

  // A 'windowed' system is defined as one where the window size can be
  // larger than the screen size, as there's some sort of window manager
  // that takes care of it (all current desktop systems fall in this category)
  // However, some systems have no concept of windowing, and have hard limits
  // on how large a window can be (ie, the size of the 'desktop' is the
  // absolute upper limit on window size)
  //
  // If the WINDOWED_SUPPORT macro is defined, we treat the system as the
  // former type; if not, as the latter type

  const int display = displayId();
#ifdef WINDOWED_SUPPORT
  // We assume that a desktop of at least minimum acceptable size means that
  // we're running on a 'large' system, and the window size requirements
  // can be relaxed
  // Otherwise, we treat the system as if WINDOWED_SUPPORT is not defined
  if(myDesktopSize[display].w < FBMinimum::Width &&
     myDesktopSize[display].h < FBMinimum::Height &&
     size > myDesktopSize[display])
    return FBInitStatus::FailTooLarge;
#else
  // Make sure this mode is even possible
  // We only really need to worry about it in non-windowed environments,
  // where requesting a window that's too large will probably cause a crash
  if(size > myDesktopSize[display])
    return FBInitStatus::FailTooLarge;
#endif

  if(myWindow.bufferType == BufferType::Emulator)
  {
    myBezel->load(); // make sure we have the correct bezel size

    // Determine possible TIA windowed zoom levels
    const auto currentTIAZoom =
      static_cast<double>(myOSystem.settings().getFloat("tia.zoom"));
    myOSystem.settings().setValue("tia.zoom",
      BSPF::clamp(currentTIAZoom, supportedTIAMinZoom(), supportedTIAMaxZoom()));
  }

  myMsgHandler.init();

  // Initialize video mode handler, so it can know what video modes are
  // appropriate for the requested image size
  myVidModeHandler.setImageSize(size);

  // Initialize video subsystem
  const string pre_about = myBackend->about();
  const FBInitStatus status = applyVideoMode();

  // Only set phosphor once when ROM is started
  if(myOSystem.eventHandler().inTIAMode())
  {
    // Phosphor mode can be enabled either globally or per-ROM
    int p_blend = 0;
    bool enable = false;
    const int phosphorMode = PhosphorHandler::toPhosphorMode(
      myOSystem.settings().getString(PhosphorHandler::SETTING_MODE));

    switch(phosphorMode)
    {
      case PhosphorHandler::Always:
        enable = true;
        p_blend = myOSystem.settings().getInt(PhosphorHandler::SETTING_BLEND);
        myOSystem.console().tia().enableAutoPhosphor(false);
        break;

      case PhosphorHandler::Auto_on:
      case PhosphorHandler::Auto:
        enable = false;
        p_blend = myOSystem.settings().getInt(PhosphorHandler::SETTING_BLEND);
        myOSystem.console().tia().enableAutoPhosphor(true, phosphorMode == PhosphorHandler::Auto_on);
        break;

      default: // PhosphorHandler::ByRom
        enable = myOSystem.console().properties().get(PropType::Display_Phosphor) == "YES";
        p_blend = BSPF::stoi(myOSystem.console().properties().get(PropType::Display_PPBlend));
        myOSystem.console().tia().enableAutoPhosphor(false);
        break;
    }
    myTIASurface->enablePhosphor(enable, p_blend);
  }

  if(status != FBInitStatus::Success)
    return status;

  // The launcher, debugger and companion TIA windows may be freely resized by
  // the user; all other UI/TIA windows keep their fixed size.  Each resizeable
  // window's owner applies its own minimum, right after this, via
  // setWindowMinSize(); imposing one here would enlarge a window the user had
  // dragged smaller.  (FBMinimum is the TIA emulation-mode floor, sized so the
  // dialogs fit over the image — not a UI window minimum.)
  myBackend->setWindowResizable(isResizable(myWindow.bufferType));

  // setVideoMode() cleared the window's minimum, so forget what we last
  // forwarded, or the owner's (unchanged) minimum would not be re-applied
  myWindow.minSize = Common::Size();

  // Print initial usage message, but only print it later if the status has changed
  if(myInitializedCount == 1)
  {
    Logger::info(myBackend->about());
  }
  else
  {
    const string post_about = myBackend->about();
    if(post_about != pre_about)
      Logger::info(post_about);
  }

  return status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setWindowMinSize(const Common::Size& size)
{
  const uInt32 scale = hidpiScaleFactor();
  const Common::Size scaled(size.w * scale, size.h * scale);

  // The content minimum is font-invariant, so a resizeable dialog re-asserts
  // the same value on every layout() — i.e. every frame while the window is
  // being dragged.  Re-applying an unchanged minimum mid-drag is wasteful and
  // can interfere with the interactive resize, so forward it to the backend
  // only when it actually changes.
  if(scaled == myWindow.minSize)
    return;
  myWindow.minSize = scaled;
  myBackend->setWindowMinSize(scaled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::handleResize(int width, int height)
{
  // Only the launcher and debugger windows are user-resizable
  if(myWindow.bufferType != BufferType::Launcher &&
     myWindow.bufferType != BufferType::Debugger)
    return;

  // The new window size becomes the new UI image/screen size
  myVidModeHandler.setImageSize(Common::Size(width, height));
  myWindow.vidMode = myVidModeHandler.buildMode(
      myOSystem.settings(), false, myBezel->info());

  // The window already has its new size; refresh the backend's cached
  // dimensions so the blitters scale correctly, then reload all surfaces
  myBackend->refreshDimensions();
  resetSurfaces();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::liveResize(int width, int height)
{
  // The launcher, the debugger and its companion TIA window are the only
  // user-resizable windows, and all re-flow live; everything else resizes
  // immediately via handleResize()
  if(!isResizable(myWindow.bufferType))
    return false;

  myWindow.pendingResize = Common::Size(width, height);
  myWindow.liveResizePending = true;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::applyLiveResize()
{
  if(!myWindow.liveResizePending)
    return false;

  myWindow.liveResizePending = false;

  // Rebuild the UI video mode for the new window size and refresh the backend's
  // cached dimensions.  Deliberately does NOT reload surfaces or present: each
  // dialog re-flows its own surfaces afterwards (updating dst rects only — no
  // texture re-upload), and the main loop then renders one correct frame.
  // (Reloading here would re-upload every texture every frame.)
  myVidModeHandler.setImageSize(myWindow.pendingResize);
  myWindow.vidMode = myVidModeHandler.buildMode(
      myOSystem.settings(), false, myBezel->info());
  myBackend->refreshDimensions();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update(UpdateMode mode)
{
  // Ignore re-entrant renders triggered by the SDL event-watch hook while a
  // backend is still building its renderer (see myInVideoMode).  The pending
  // frame is reissued once applyVideoMode() completes (myPendingRender).
  if(myInVideoMode)
    return;

  // Onscreen messages are a special case and require different handling than
  // other objects; they aren't UI dialogs in the normal sense nor are they
  // TIA images, and they need to be rendered on top of everything
  // The logic is split in two pieces:
  //  - at the top of ::update(), to determine whether underlying dialogs
  //    need to be force-redrawn
  //  - at the bottom of ::update(), to actually draw them (this must come
  //    last, since they are always drawn on top of everything else).

  const bool forceRedraw = (mode == UpdateMode::REDRAW);
  bool redraw = forceRedraw;

  // Forced render without draw required if messages or dialogs were closed
  // Note: For dialogs only relevant when two or more dialogs were stacked
  const bool rerender = (mode == UpdateMode::REDRAW || mode == UpdateMode::RERENDER
                         || myPendingRender);
  myPendingRender = false;

  // Show any messages enqueued from other threads (e.g. PlusROM/cart callbacks)
  myMsgHandler.drainPending();

  switch(myOSystem.eventHandler().state())
  {
    case EventHandlerState::NONE:
    case EventHandlerState::EMULATION:
      // Do nothing; emulation mode is handled separately (see below)
      return;

    case EventHandlerState::PAUSE:
    {
      // Show a pause message immediately and then every 7 seconds
      const bool shade = myOSystem.settings().getBool("pausedim");

      if(myMsgHandler.tickPause())
      {
        showTextMessage("Paused", MessagePosition::MiddleCenter);
        renderTIA(false, shade);
      }
      if(rerender)
        renderTIA(false, shade);
      break;  // EventHandlerState::PAUSE
    }

  #ifdef GUI_SUPPORT
    case EventHandlerState::OPTIONSMENU:
    case EventHandlerState::CMDMENU:
    case EventHandlerState::HIGHSCORESMENU:
    case EventHandlerState::PLUSROMSMENU:
    case EventHandlerState::OVERLAYMENU:
    {
      // All GUI menus that overlay the TIA image share one render path;
      // the active DialogContainer is tracked by EventHandler::overlay()
      DialogContainer& overlay = myOSystem.eventHandler().overlay();
      overlay.tick();
      redraw |= overlay.needsRedraw();
      if(redraw)
      {
        renderTIA(true, true);
        overlay.draw(forceRedraw);
      }
      else if(rerender)
      {
        renderTIA(true, true);
        overlay.render();
      }
      break;  // GUI menu overlays
    }

    case EventHandlerState::TIMEMACHINE:
    {
      myOSystem.timeMachine().tick();
      redraw |= myOSystem.timeMachine().needsRedraw();
      if(redraw)
      {
        renderTIA();
        myOSystem.timeMachine().draw(forceRedraw);
      }
      else if(rerender)
      {
        renderTIA();
        myOSystem.timeMachine().render();
      }
      break;  // EventHandlerState::TIMEMACHINE
    }

    case EventHandlerState::PLAYBACK:
    {
      static Int32 frames = 0;
      bool success = true;

      if(--frames <= 0)
      {
        RewindManager& r = myOSystem.state().rewindManager();
        const uInt64 prevCycles = r.getCurrentCycles();

        success = r.unwindStates(1);

        // Determine playback speed, the faster the more the states are apart
        const Int64 frameCycles = static_cast<Int64>(76) * std::max<Int32>(myOSystem.console().tia().scanlinesLastFrame(), 240);
        const Int64 intervalFrames = r.getInterval() / frameCycles;
        const Int64 stateFrames = (r.getCurrentCycles() - prevCycles) / frameCycles;

        //frames = intervalFrames + std::sqrt(std::max(stateFrames - intervalFrames, 0));
        frames = std::round(std::sqrt(stateFrames));

        // Pause sound if saved states were removed or states are too far apart
        myOSystem.sound().pause(stateFrames > intervalFrames ||
            std::cmp_greater(frames, myOSystem.audioSettings().bufferSize() / 2 + 1));
      }
      redraw |= success;
      if(redraw)
        renderTIA(false);

      // Stop playback mode at the end of the state buffer
      // and switch to Time Machine or Pause mode
      if(!success)
      {
        frames = 0;
        myOSystem.sound().pause(true);
        myOSystem.eventHandler().enterMenuMode(EventHandlerState::TIMEMACHINE);
      }
      break;  // EventHandlerState::PLAYBACK
    }

    case EventHandlerState::LAUNCHER:
    {
      myOSystem.launcher().tick();
      redraw |= myOSystem.launcher().needsRedraw();
      if(redraw)
        myOSystem.launcher().draw(forceRedraw);
      else if(rerender)
        myOSystem.launcher().render();
      break;  // EventHandlerState::LAUNCHER
    }
  #endif  // GUI_SUPPORT

  #ifdef DEBUGGER_SUPPORT
    case EventHandlerState::DEBUGGER:
    {
      myOSystem.debugger().tick();
      redraw |= myOSystem.debugger().needsRedraw();
      if(redraw)
        myOSystem.debugger().draw(forceRedraw);
      else if(rerender)
        myOSystem.debugger().render();
      break;  // EventHandlerState::DEBUGGER
    }
  #endif  // DEBUGGER_SUPPORT
    default:
      break;
  }

  // Draw any pending messages
  // The logic here determines whether to draw the message
  // If the message is to be disabled, logic inside the draw method
  // indicates that, and then the code at the top of this method sees
  // the change and redraws everything
  if(myMsgHandler.isShown())
    redraw |= myMsgHandler.draw();

  // Push buffers to screen only when necessary
  if(redraw || rerender)
    myBackend->renderToScreen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setRenderTarget(int target)
{
  if(target == myRenderTarget)
    return;

  // Swap only the per-window state; everything else (palette, fonts,
  // TIASurface, message handler, desktop maps) is shared and stays put.
  std::swap(myBackend, myOtherBackend);
  std::swap(myWindow, myOtherWindow);
  myRenderTarget = target;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::openSecondaryWindow(DialogContainer& container,
                                              string_view title, BufferType type,
                                              Common::Size size,
                                              Common::Size minSize)
{
  setRenderTarget(1);

  // Lazily create the secondary backend (its own window + renderer).  The
  // shared state (palette/fonts/TIASurface) already exists from the primary.
  if(!mySecondaryCreated)
  {
    myBackend = MediaFactory::createVideoBackend(myOSystem);
    myBackend->queryHardware(myFullscreenDisplays, myWindowedDisplays, myRenderers);
    mySecondaryCreated = true;
  }

  const FBInitStatus status = createDisplay(title, type, size);
  if(status == FBInitStatus::Success)
  {
    setWindowMinSize(minSize);

    // Open the container's base dialog; its surfaces bind to this (secondary)
    // backend because it is the current render target.
    container.reStack();
    myBackend->setWindowVisible(true);
    mySecondaryActive = true;
  }

  setRenderTarget(0);
  return status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::resizeSecondaryWindow(DialogContainer& container,
                                        int width, int height)
{
  if(!mySecondaryActive)
    return false;

  setRenderTarget(1);

  // The container's dialogs own surfaces bound to this backend, so both the
  // video mode rebuild and the re-flow must happen while it is the target
  const bool applied = liveResize(width, height) && container.applyResize();

  setRenderTarget(0);
  return applied;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::renderSecondaryWindow(DialogContainer& container, UpdateMode mode)
{
  if(!mySecondaryActive)
    return;

  setRenderTarget(1);
  updateContainer(container, mode);
  setRenderTarget(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::closeSecondaryWindow()
{
  if(!mySecondaryActive)
    return;

  // Keep the backend and surfaces alive for a fast re-open; just hide it
  setRenderTarget(1);
  myBackend->setWindowVisible(false);
  setRenderTarget(0);
  mySecondaryActive = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::primaryWindowId() const
{
  return primaryBackend().windowId();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::secondaryWindowId() const
{
  return mySecondaryCreated ? secondaryBackend().windowId() : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::updateContainer(DialogContainer& container, UpdateMode mode)
{
  const bool forceRedraw = (mode == UpdateMode::REDRAW);
  bool redraw = forceRedraw;
  const bool rerender = (mode == UpdateMode::REDRAW || mode == UpdateMode::RERENDER
                         || myPendingRender);
  myPendingRender = false;

  container.tick();
  redraw |= container.needsRedraw();
  if(redraw)
    container.draw(forceRedraw);
  else if(rerender)
    container.render();

  if(redraw || rerender)
    myBackend->renderToScreen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::updateInEmulationMode(float framesPerSecond)
{
  // Update method that is specifically tailored to emulation mode
  //
  // We don't worry about selective rendering here; the rendering
  // always happens at the full framerate

  renderTIA();

  // Show any messages enqueued from the emulation worker thread (e.g. AR
  // Supercharger load notifications) before drawing them this frame
  myMsgHandler.drainPending();

  // Show frame statistics
  if(myMsgHandler.statsShown())
    myMsgHandler.drawStats(framesPerSecond);

  myMsgHandler.onEmulationFrame();

  // Draw any pending messages
  if(myMsgHandler.isShown())
    myMsgHandler.draw();

  // Push buffers to screen
  myBackend->renderToScreen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showTextMessage(string_view message,
                                  MessagePosition position, bool force)
{
#ifdef GUI_SUPPORT
  myMsgHandler.showText(message, position, force);
#else
  if(myBackend && (force || myOSystem.settings().getBool("uimessages")))
    myBackend->showMessage(message);
#endif  // GUI_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showGaugeMessage(string_view message, string_view valueText,
                                   float value, float minValue, float maxValue)
{
#ifdef GUI_SUPPORT
  myMsgHandler.showGauge(message, valueText, value, minValue, maxValue);
#else
  if(myBackend && (myOSystem.settings().getBool("uimessages")))
    myBackend->showGaugeMessage(message, valueText, value, minValue, maxValue);
#endif  // GUI_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::messageShown() const
{
  return myMsgHandler.isShown();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFrameStats(bool toggle)
{
  if(toggle)
    myMsgHandler.showStats(!myMsgHandler.statsEnabled());
  myOSystem.settings().setValue(
    myOSystem.settings().getBool("dev.settings") ? "dev.stats" : "plr.stats",
    myMsgHandler.statsEnabled());

  showTextMessage(std::format("Console info {}",
    myMsgHandler.statsEnabled() ? "enabled" : "disabled"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showFrameStats(bool enable)
{
  myMsgHandler.showStats(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  myMsgHandler.enable(enable);
  if(!enable)
  {
    // Update immediately
    if(myOSystem.eventHandler().state() == EventHandlerState::EMULATION)
      renderTIA();
    else
      update();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setPauseDelay()
{
  myMsgHandler.setPauseDelay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
shared_ptr<FBSurface> FrameBuffer::allocateSurface(
    int w, int h, ScalingInterpolation inter, const uInt32* data)
{
  mySurfaceList.push_back(myBackend->createSurface(w, h, inter, data));
  return mySurfaceList.back();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::deallocateSurface(const shared_ptr<FBSurface>& surface)
{
  if(surface)
    mySurfaceList.remove(surface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resetSurfaces()
{
  for(auto& surface: mySurfaceList)
    surface->reload();

  update(UpdateMode::REDRAW); // force full update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::renderTIA(bool doClear, bool shade)
{
  if(doClear)
    clear();  // TODO - test this: it may cause slowdowns on older systems

  myTIASurface->render(shade);
  if(myBezel)
    myBezel->render();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setTIAPalette(const PaletteArray& rgb_palette)
{
  // Hoist shift values — surface format is constant
  const uInt32 rShift = std::countr_zero(rMask());
  const uInt32 gShift = std::countr_zero(gMask());
  const uInt32 bShift = std::countr_zero(bMask());
  const uInt32 aMask_ = aMask();  // fully transparent; no alpha in RGB palette

  // Create a TIA palette from the raw RGB data
  PaletteArray tia_palette = {0};
  for(int i = 0; i < 256; ++i)
  {
    const uInt32 rgb = rgb_palette[i];
    tia_palette[i] = aMask_
                   | (((rgb >> 16) & 0xFF) << rShift)
                   | (((rgb >>  8) & 0xFF) << gShift)
                   | (( rgb        & 0xFF) << bShift);
  }
  // Remember the TIA palette; place it at the beginning of the full palette
  std::copy_n(tia_palette.begin(), tia_palette.size(), myFullPalette.begin());

  // Let the TIA surface know about the new palette
  myTIASurface->setPalette(tia_palette, rgb_palette);

  // Since the UI palette shares the TIA palette, we need to update it too
  setUIPalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setUIPalette()
{
  const Settings& settings = myOSystem.settings();
  const string& key = settings.getBool("altuipalette") ? "uipalette2" : "uipalette";
  // Set palette for UI (upper area of full palette)
  const UIPaletteArray& ui_palette =
     (settings.getString(key) == "classic") ? ourClassicUIPalette :
     (settings.getString(key) == "light")   ? ourLightUIPalette :
     (settings.getString(key) == "dark")    ? ourDarkUIPalette :
      ourStandardUIPalette;

  // Hoist shift values — surface format is constant
  const uInt32 rShift = std::countr_zero(rMask());
  const uInt32 gShift = std::countr_zero(gMask());
  const uInt32 bShift = std::countr_zero(bMask());
  const uInt32 aMask_ = aMask();

  for(auto i = 0UZ; i < ui_palette.size(); ++i)
  {
    const uInt32 rgb = ui_palette[i];
    myFullPalette[kColor + i] = aMask_
                              | (((rgb >> 16) & 0xFF) << rShift)
                              | (((rgb >>  8) & 0xFF) << gShift)
                              | (( rgb        & 0xFF) << bShift);
  }
  setDisasmPalette();  // fills disasm slots and calls FBSurface::setPalette
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setDisasmPalette()
{
  const Settings& settings = myOSystem.settings();
  const string& key = settings.getBool("altuipalette") ? "uipalette2" : "uipalette";
  const string& name = settings.getString(key);
  const bool isDark = (name == "dark" || name == "classic");
  const DisasmPaletteArray& dp = isDark ? ourDarkDisasmPalette : ourStandardDisasmPalette;

  const uInt32 rShift = std::countr_zero(rMask());
  const uInt32 gShift = std::countr_zero(gMask());
  const uInt32 bShift = std::countr_zero(bMask());
  const uInt32 aMask_ = aMask();

  for(auto i = 0UZ; i < dp.size(); ++i)
  {
    const uInt32 rgb = dp[i];
    myFullPalette[kUINColors + i] = aMask_
                                  | (((rgb >> 16) & 0xFF) << rShift)
                                  | (((rgb >>  8) & 0xFF) << gShift)
                                  | (( rgb        & 0xFF) << bShift);
  }
  FBSurface::setPalette(myFullPalette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::stateChanged(EventHandlerState state)
{
  // Prevent removing state change messages (brand-new ones survive transitions)
  if(!myMsgHandler.msgJustShown())
    myMsgHandler.hide();
  update(); // update immediately
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBuffer::getDisplayKey(BufferType bufferType) const
{
  if(bufferType == BufferType::None)
    bufferType = myWindow.bufferType;

  // save current window's display and position
  switch(bufferType)
  {
    case BufferType::Launcher:
      return "launcherdisplay";

    case BufferType::Emulator:
      return "display";

    #ifdef DEBUGGER_SUPPORT
    case BufferType::Debugger:
      return "dbg.display";

    case BufferType::TiaWindow:
      return "tiawindow.display";
    #endif

    default:
      return "";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBuffer::getPositionKey(BufferType bufferType) const
{
  if(bufferType == BufferType::None)
    bufferType = myWindow.bufferType;

  // save current window's display and position
  switch(bufferType)
  {
    case BufferType::Launcher:
      return "launcherpos";

    case BufferType::Emulator:
      return  "windowedpos";

    #ifdef DEBUGGER_SUPPORT
    case BufferType::Debugger:
      return "dbg.pos";

    case BufferType::TiaWindow:
      return "tiawindow.pos";
    #endif

    default:
      return "";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::savePosition(const FBBackend& backend, BufferType type) const
{
  myOSystem.settings().setValue(
    getDisplayKey(type), backend.getCurrentDisplayID());
  if(backend.isCurrentWindowPositioned())
    myOSystem.settings().setValue(
      getPositionKey(type), backend.getCurrentWindowPos());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::saveCurrentWindowPosition() const
{
  if(myBackend)
    savePosition(*myBackend, myWindow.bufferType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::saveWindowPosition(uInt32 windowId) const
{
  // Each window keeps its own position/display keys, and the one that moved is
  // not necessarily the current render target
  if(windowId == primaryWindowId())
    savePosition(primaryBackend(), primaryWindow().bufferType);
  else if(mySecondaryCreated && windowId == secondaryWindowId())
    savePosition(secondaryBackend(), secondaryWindow().bufferType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::saveConfig(Settings& settings) const
{
  // Save the last windowed position and display of each window on shutdown.
  // The secondary window is never the render target here, so it needs its own
  // pass (it keeps its window alive even while hidden)
  saveCurrentWindowPosition();
  if(mySecondaryCreated)
    savePosition(secondaryBackend(), secondaryWindow().bufferType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
#ifdef WINDOWED_SUPPORT
  // Switching between fullscreen and windowed modes will invariably mean
  // that the 'window' resolution changes.  Currently, dialogs are not
  // able to resize themselves when they are actively being shown
  // (they would have to be closed and then re-opened, etc).
  // For now, we simply disallow screen switches in such modes
  switch(myOSystem.eventHandler().state())
  {
    case EventHandlerState::EMULATION:
    case EventHandlerState::PAUSE:
      break; // continue with processing (aka, allow a mode switch)
    case EventHandlerState::DEBUGGER:
    case EventHandlerState::LAUNCHER:
      if(myOSystem.eventHandler().overlay().baseDialogIsActive())
        break; // allow a mode switch when there is only one dialog
      [[fallthrough]];
    default:
      return;
  }

  myOSystem.settings().setValue("fullscreen", enable);
  saveCurrentWindowPosition();
  applyVideoMode();
#endif  // WINDOWED_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen(bool toggle)
{
  const EventHandlerState state = myOSystem.eventHandler().state();

  switch(state)
  {
    case EventHandlerState::LAUNCHER:
    case EventHandlerState::EMULATION:
    case EventHandlerState::PAUSE:
    case EventHandlerState::DEBUGGER:
    {
      const bool isFullscreen = toggle ? !fullScreen() : fullScreen();
      setFullscreen(isFullscreen);

      if(state != EventHandlerState::LAUNCHER)
      {
        const string_view state_str = isFullscreen ? "enabled" : "disabled";

        if(state != EventHandlerState::DEBUGGER)
        {
          const string msg = isFullscreen
            ? std::format("Fullscreen {} ({} Hz, Zoom {}%)",
                state_str, myBackend->refreshRate(),
                static_cast<int>(round(myWindow.vidMode.zoom * 100)))
            : std::format("Fullscreen {} (Zoom {}%)",
                state_str,
                static_cast<int>(round(myWindow.vidMode.zoom * 100)));
          showTextMessage(msg);
        }
        else
          showTextMessage(std::format("Fullscreen {}", state_str));
      }
      break;
    }
    default:
      break;
  }
}

#ifdef ADAPTABLE_REFRESH_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleAdaptRefresh(bool toggle)
{
  bool isAdaptRefresh = myOSystem.settings().getInt("tia.fs_refresh");

  if(toggle)
    isAdaptRefresh = !isAdaptRefresh;

  if(myWindow.bufferType == BufferType::Emulator)
  {
    if(toggle)
    {
      myOSystem.settings().setValue("tia.fs_refresh", isAdaptRefresh);
      // issue a complete framebuffer re-initialization
      myOSystem.createFrameBuffer();
    }

    showTextMessage(std::format("Adapt refresh rate {} ({} Hz)",
      isAdaptRefresh ? "enabled" : "disabled",
      myBackend->refreshRate()));
  }
}
#endif  // ADAPTABLE_REFRESH_SUPPORT

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::changeOverscan(int direction)
{
  if(fullScreen())
  {
    const int oldOverscan = myOSystem.settings().getInt("tia.fs_overscan");
    const int overscan = BSPF::clamp(oldOverscan + direction, 0, 10);

    if(overscan != oldOverscan)
    {
      myOSystem.settings().setValue("tia.fs_overscan", overscan);

      // issue a complete framebuffer re-initialization
      myOSystem.createFrameBuffer();
    }

    const string val = overscan
      ? std::format("{}{}{}", overscan > 0 ? "+" : "", overscan, "%")
      : "Off";
    myOSystem.frameBuffer().showGaugeMessage("Overscan", val, overscan, 0, 10);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::switchVideoMode(int direction)
{
  // Only applicable when in TIA/emulation mode
  if(!myOSystem.eventHandler().inTIAMode())
    return;

  if(!fullScreen())
  {
    // Windowed TIA modes support variable zoom levels
    auto zoom = static_cast<double>(myOSystem.settings().getFloat("tia.zoom"));
    if(direction == +1)       zoom += ZOOM_STEPS;
    else if(direction == -1)  zoom -= ZOOM_STEPS;

    // Make sure the level is within the allowable desktop size
    zoom = BSPF::clampw(zoom, supportedTIAMinZoom(), supportedTIAMaxZoom());
    myOSystem.settings().setValue("tia.zoom", zoom);
  }
  else
  {
    // In fullscreen mode, there are only two modes, so direction
    // is irrelevant
    if(direction == +1 || direction == -1)
    {
      const bool stretch = myOSystem.settings().getBool("tia.fs_stretch");
      myOSystem.settings().setValue("tia.fs_stretch", !stretch);
    }
  }

  saveCurrentWindowPosition();
  if(!direction || applyVideoMode() == FBInitStatus::Success)
  {
    if(fullScreen())
      showTextMessage(myWindow.vidMode.description);
    else
      showGaugeMessage("Zoom", myWindow.vidMode.description,
                       static_cast<float>(myWindow.vidMode.zoom),
                       static_cast<float>(supportedTIAMinZoom()),
                       static_cast<float>(supportedTIAMaxZoom()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleBezel(bool toggle)
{
  bool enabled = myOSystem.settings().getBool("bezel.show");

  if(toggle && myWindow.bufferType == BufferType::Emulator)
  {
    if(!fullScreen() && !myOSystem.settings().getBool("bezel.windowed"))
    {
      myOSystem.frameBuffer().showTextMessage("Bezels in windowed mode are not enabled");
      return;
    }
    else
    {
      enabled = !enabled;
      myOSystem.settings().setValue("bezel.show", enabled);
      if(!myBezel->load() && enabled)
      {
        myOSystem.settings().setValue("bezel.show", !enabled);
        return;
      }
      else
      {
        // Determine possible TIA windowed zoom levels
        const auto currentTIAZoom =
          static_cast<double>(myOSystem.settings().getFloat("tia.zoom"));
        myOSystem.settings().setValue("tia.zoom",
          BSPF::clamp(currentTIAZoom, supportedTIAMinZoom(), supportedTIAMaxZoom()));

        saveCurrentWindowPosition();
        applyVideoMode();
      }
    }
  }
  myOSystem.frameBuffer().showTextMessage(enabled ? "Bezel enabled" : "Bezel disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::applyVideoMode()
{
  // Update display size, in case windowed/fullscreen mode has changed
  const Settings& s = myOSystem.settings();
  const int ID = displayId(); // TODO SDL 3:

  if(s.getBool("fullscreen"))
    myVidModeHandler.setDisplaySize(myFullscreenDisplays[ID], true);
  else
    myVidModeHandler.setDisplaySize(myAbsDesktopSize[ID], false);

  const bool inTIAMode = myOSystem.eventHandler().inTIAMode();

#ifdef IMAGE_SUPPORT
  if(inTIAMode)
    myBezel->load();
#endif

  // Build the new mode based on current settings
  const VideoModeHandler::Mode& mode
    = myVidModeHandler.buildMode(s, inTIAMode, myBezel->info());
  if(mode.imageR.size() > mode.screenS)
    return FBInitStatus::FailTooLarge;

  // Changing the video mode can take some time, during which the last
  // sound played may get 'stuck'
  // So we pause the sound until the operation completes
  const bool oldPauseState = myOSystem.sound().pause(true);
  FBInitStatus status = FBInitStatus::FailNotSupported;

  // Guard against the SDL event-watch hook re-entering update() while the
  // backend pumps events from inside window/renderer creation (see
  // myInVideoMode); critical for the companion TIA window whose renderer does
  // not exist yet at this point
  myInVideoMode = true;
  const bool modeApplied = myBackend->setVideoMode(mode,
      myOSystem.settings().getInt(getDisplayKey()),
      myOSystem.settings().getPoint(getPositionKey()));
  myInVideoMode = false;

  if(modeApplied)
  {
    myWindow.vidMode = mode;
    status = FBInitStatus::Success;

    // A completed mode change supersedes any in-progress interactive resize, so
    // a size still pending from the previous mode (launcher/debugger) can't
    // leak into this one
    myWindow.liveResizePending = false;

    // Did we get the requested fullscreen state?
    myOSystem.settings().setValue("fullscreen", fullScreen());

    // Inform TIA surface about new mode, and update TIA settings
    if(inTIAMode)
    {
      myTIASurface->initialize(myOSystem.console(), myWindow.vidMode);
      if(fullScreen())
        myOSystem.settings().setValue("tia.fs_stretch",
          myWindow.vidMode.stretch == VideoModeHandler::Mode::Stretch::Fill);
      else
        myOSystem.settings().setValue("tia.zoom", myWindow.vidMode.zoom);

      myBezel->apply();
    }

    resetSurfaces();
    setCursorState();

    myPendingRender = true;
  }
  else
    Logger::error("ERROR: Couldn't initialize video subsystem");

  // Restore sound settings
  myOSystem.sound().pause(oldPauseState);

  return status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double FrameBuffer::maxWindowZoom() const
{
  const uInt32 display = displayId(BufferType::Emulator);
  double multiplier = 1;

  for(;;)
  {
    // Figure out the zoomed size of the window (incl. the bezel)
    const uInt32 width  = static_cast<double>(TIAConstants::viewableWidth)  * myBezel->ratioW() * multiplier;
    const uInt32 height = static_cast<double>(TIAConstants::viewableHeight) * myBezel->ratioH() * multiplier;

    if((width > myAbsDesktopSize.at(display).w) ||
       (height > myAbsDesktopSize.at(display).h))
      break;

    multiplier += ZOOM_STEPS;
  }
  return multiplier > 1 ? multiplier - ZOOM_STEPS : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  myGrabMouse = myOSystem.settings().getBool("grabmouse");
  // Always grab mouse in emulation (if enabled) and emulating a controller
  // that always uses the mouse
  const bool emulation =
      myOSystem.eventHandler().state() == EventHandlerState::EMULATION;
  const bool usesLightgun = emulation && myOSystem.hasConsole() ?
    myOSystem.console().leftController().type() == Controller::Type::Lightgun ||
    myOSystem.console().rightController().type() == Controller::Type::Lightgun : false;
  // Show/hide cursor in UI/emulation mode based on 'cursor' setting
  int cursor = myOSystem.settings().getInt("cursor");

  // Always enable cursor in lightgun games
  if (usesLightgun && !myGrabMouse)
    cursor |= 1;  // +Emulation

  switch(cursor)
  {
    case 0:                   // -UI, -Emulation
      showCursor(false);
      break;
    case 1:
      showCursor(emulation);  // -UI, +Emulation
      break;
    case 2:                   // +UI, -Emulation
      showCursor(!emulation);
      break;
    case 3:
      showCursor(true);       // +UI, +Emulation
      break;
    default:
      break;
  }

  myGrabMouse &= grabMouseAllowed();
  myBackend->grabMouse(myGrabMouse);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableTextEvents(bool enable)
{
  myBackend->enableTextEvents(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::grabMouseAllowed()
{
  // Allow grabbing mouse in emulation (if enabled) and emulating a controller
  // that always uses the mouse
  const bool emulation =
    myOSystem.eventHandler().state() == EventHandlerState::EMULATION;
  const bool analog = myOSystem.hasConsole() ?
    (myOSystem.console().leftController().usesMouse() ||
     myOSystem.console().rightController().usesMouse()) : false;
  const bool usesLightgun = emulation && myOSystem.hasConsole() ?
    myOSystem.console().leftController().type() == Controller::Type::Lightgun ||
    myOSystem.console().rightController().type() == Controller::Type::Lightgun : false;
  const bool alwaysUseMouse = BSPF::equalsIgnoreCase("always", myOSystem.settings().getString("usemouse"));

  // Disable grab while cursor is shown in emulation
  const bool cursorHidden = !(myOSystem.settings().getInt("cursor") & 1);

  return emulation && (analog || usesLightgun || alwaysUseMouse) && cursorHidden;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableGrabMouse(bool enable)
{
  myGrabMouse = enable;
  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleGrabMouse(bool toggle)
{
  bool oldState = myGrabMouse = myOSystem.settings().getBool("grabmouse");

  if(toggle)
  {
    if(grabMouseAllowed())
    {
      myGrabMouse = !myGrabMouse;
      myOSystem.settings().setValue("grabmouse", myGrabMouse);
      setCursorState();
    }
  }
  else
    oldState = !myGrabMouse; // display current state

  myOSystem.frameBuffer().showTextMessage(oldState != myGrabMouse ? myGrabMouse
                                          ? "Grab mouse enabled" : "Grab mouse disabled"
                                          : "Grab mouse not allowed");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::updateTheme()
{
  if(myOSystem.settings().getBool("autouipalette"))
  {
    const bool darkTheme = myOSystem.settings().getBool("altuipalette");

    if((myBackend->isLightTheme() && darkTheme) ||
       (myBackend->isDarkTheme() && !darkTheme))
    {
      myOSystem.settings().setValue("altuipalette", !darkTheme);
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
  Palette is defined as follows:
    *** Base colors ***
    kColor            Normal foreground color (non-text)
    kBGColor          Normal background color (non-text)
    kBGColorLo        Disabled background color dark (non-text)
    kBGColorHi        Disabled background color light (non-text)
    kShadowColor      Item is disabled (unused)
    *** Text colors ***
    kTextColor        Normal text color
    kTextColorHi      Highlighted text color
    kTextColorEm      Emphasized text color
    kTextColorInv     Color for selected text
    kTextColorLink    Color for links
    *** UI elements (dialog and widgets) ***
    kDlgColor         Dialog background
    kWidColor         Widget background
    kWidColorHi       Widget highlight color
    kWidFrameColor    Border for currently selected widget
    *** Button colors ***
    kBtnColor         Normal button background
    kBtnColorHi       Highlighted button background
    kBtnBorderColor,
    kBtnBorderColorHi,
    kBtnTextColor     Normal button font color
    kBtnTextColorHi   Highlighted button font color
    *** Checkbox colors ***
    kCheckColor       Color of 'X' in checkbox
    *** Scrollbar colors ***
    kScrollColor      Normal scrollbar color
    kScrollColorHi    Highlighted scrollbar color
    *** Debugger colors ***
    kDbgChangedColor      Background color for changed cells
    kDbgChangedTextColor  Text color for changed cells
    kDbgColorHi           Highlighted color in debugger data cells
    kDbgColorRed          Red color in debugger
    *** Slider colors ***
    kSliderColor          Enabled slider
    kSliderColorHi        Focussed slider
    kSliderBGColor        Enabled slider background
    kSliderBGColorHi      Focussed slider background
    kSliderBGColorLo      Disabled slider background
    *** Other colors ***
    kColorInfo            TIA output position color
    kColorTitleBar        Title bar color
    kColorTitleText       Title text color
*/
UIPaletteArray FrameBuffer::ourStandardUIPalette = {
  { 0x686868, 0x000000, 0xa38c61, 0xdccfa5, 0x404040,           // base
    0x000000, 0xac3410, 0x9f0000, 0xf0f0cf, 0xac3410,           // text
    0xc9af7c, 0xf0f0cf, 0xd55941, 0xc80000,                     // UI elements
    0xac3410, 0xd55941, 0x686868, 0xdccfa5, 0xf0f0cf, 0xf0f0cf, // buttons
    0xac3410,                                                   // checkbox
    0xac3410, 0xd55941,                                         // scrollbar
    0xc80000, 0xffff80, 0xc8c8ff, 0xc80000,                     // debugger
    0xac3410, 0xd55941, 0xdccfa5, 0xf0f0cf, 0xa38c61,           // slider
    0xffffff, 0xac3410, 0xf0f0cf                                // other
  }
};

UIPaletteArray FrameBuffer::ourClassicUIPalette = {
  { 0x686868, 0x000000, 0x404040, 0x404040, 0x404040,           // base
    0x20a020, 0x00ff00, 0xc80000, 0x000000, 0x00ff00,           // text
    0x000000, 0x000000, 0x00ff00, 0xc80000,                     // UI elements
    0x000000, 0x000000, 0x686868, 0x00ff00, 0x20a020, 0x00ff00, // buttons
    0x20a020,                                                   // checkbox
    0x20a020, 0x00ff00,                                         // scrollbar
    0xc80000, 0x00ff00, 0xc8c8ff, 0xc80000,                     // debugger
    0x20a020, 0x00ff00, 0x404040, 0x686868, 0x404040,           // slider
    0x00ff00, 0x20a020, 0x000000                                // other
  }
};

UIPaletteArray FrameBuffer::ourLightUIPalette = {
  { 0x808080, 0x000000, 0xc0c0c0, 0xe1e1e1, 0x333333,           // base
    0x000000, 0xBDDEF9, 0x0078d7, 0x000000, 0x005aa1,           // text
    0xf0f0f0, 0xffffff, 0x0078d7, 0x0f0f0f,                     // UI elements
    0xe1e1e1, 0xe5f1fb, 0x808080, 0x0078d7, 0x000000, 0x000000, // buttons
    0x333333,                                                   // checkbox
    0xc0c0c0, 0x808080,                                         // scrollbar
    0xffc0c0, 0x000000, 0xe00000, 0xc00000,                     // debugger
    0x333333, 0x0078d7, 0xc0c0c0, 0xffffff, 0xc0c0c0,           // slider 0xBDDEF9| 0xe1e1e1 | 0xffffff
    0xffffff, 0x333333, 0xf0f0f0                                // other
  }
};

UIPaletteArray FrameBuffer::ourDarkUIPalette = {
  { 0x646464, 0xc0c0c0, 0x3c3c3c, 0x282828, 0x989898,           // base
    0xc0c0c0, 0x1567a5, 0x0064b7, 0xc0c0c0, 0x1d92e0,           // text
    0x202020, 0x000000, 0x0059a3, 0xb0b0b0,                     // UI elements
    0x282828, 0x00467f, 0x646464, 0x0059a3, 0xc0c0c0, 0xc0c0c0, // buttons
    0x989898,                                                   // checkbox
    0x3c3c3c, 0x646464,                                         // scrollbar
    0x7f2020, 0xc0c0c0, 0xe00000, 0xc00000,                     // debugger
    0x989898, 0x0059a3, 0x3c3c3c, 0x000000, 0x3c3c3c,           // slider
    0x000000, 0x404040, 0xc0c0c0                                // other
  }
};

// Disassembly palettes — entry order matches kDisasmBlack..kDisasmWhite
// "standard": muted shades readable on light UI backgrounds (Standard, Light)
DisasmPaletteArray FrameBuffer::ourStandardDisasmPalette = {{
  0x202020,  // Black
  0xbb1100,  // Red
  0xcc5500,  // Orange
  0xaa7700,  // Yellow  (amber)
  0x558800,  // Lime
  0x226600,  // Green
  0x006666,  // Teal
  0x007799,  // Cyan
  0x2255cc,  // Blue
  0x333399,  // Indigo
  0x6633aa,  // Violet
  0x882288,  // Magenta
  0xaa2266,  // Pink
  0x774422,  // Brown
  0x666666,  // Gray
  0xf0f0f0,  // White
}};
// "dark": vivid shades readable on dark UI backgrounds (Classic, Dark)
DisasmPaletteArray FrameBuffer::ourDarkDisasmPalette = {{
  0x101010,  // Black
  0xff6060,  // Red
  0xff9944,  // Orange
  0xffdd00,  // Yellow
  0xaaff44,  // Lime
  0x44dd44,  // Green
  0x22ddbb,  // Teal
  0x44ddff,  // Cyan
  0x6699ff,  // Blue
  0x8888ff,  // Indigo
  0xbb77ff,  // Violet
  0xff66ff,  // Magenta
  0xff66aa,  // Pink
  0xcc8855,  // Brown
  0xaaaaaa,  // Gray
  0xffffff,  // White
}};
