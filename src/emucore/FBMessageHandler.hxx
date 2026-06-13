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

#ifndef FB_MESSAGE_HANDLER_HXX
#define FB_MESSAGE_HANDLER_HXX

class FrameBuffer;
class FBSurface;
class OSystem;

#include <mutex>
#include <thread>

#include "FrameBufferConstants.hxx"
#include "bspf.hxx"

/**
  Encapsulates all onscreen message and frame-statistics overlay logic for
  the FrameBuffer.  FrameBuffer owns one instance and delegates all message
  state to it.

  @author  Stephen Anthony
*/
class FBMessageHandler
{
  public:
    FBMessageHandler(FrameBuffer& fb, OSystem& osystem);
    ~FBMessageHandler() = default;

    /**
      Allocate rendering surfaces.  Call each time FrameBuffer::createDisplay()
      runs (i.e. whenever the window is (re)created).
    */
    void init();

    /**
      Show a plain text message onscreen.

      @param message  The message to display
      @param position Onscreen position
      @param force    Show even if ui messages are disabled in settings
    */
    void showText(string_view message,
                  MessagePosition position = MessagePosition::BottomCenter,
                  bool force = false);

    /**
      Show a message with a gauge bar onscreen.
    */
    void showGauge(string_view message, string_view valueText,
                   float value, float minValue = 0.F, float maxValue = 100.F);

    /**
      Show any messages enqueued from other threads (e.g. cartridge code on the
      emulation worker thread).  Must be called on the main (render) thread;
      FrameBuffer invokes it once per frame before drawing.
    */
    void drainPending();

    bool isShown() const { return myMsg.enabled; }

    /**
      Toggle the frame-statistics overlay on/off.
    */
    void showStats(bool enable);
    bool statsEnabled() const { return myStatsEnabled; }
    bool statsShown()   const { return myStatsMsg.enabled; }

    /**
      Enable or disable all pending messages.  When disabled the message
      surfaces are hidden but not forgotten; re-enabling restores them.
    */
    void enable(bool enable);

    /**
      Set the initial pause-display delay (called when leaving emulation).
    */
    void setPauseDelay();

    /**
      Advance pause-display timing.  Called each frame while in PAUSE state.
      Returns true when it is time to re-show the "Paused" banner (the caller
      is responsible for calling showText and re-rendering the TIA).
    */
    bool tickPause();

    /**
      Record end-of-emulation-frame state: capture the current scanline count
      (used by drawStats) and reset the pause counter.
    */
    void onEmulationFrame();

    /**
      Returns true if the current message was posted in this frame (counter
      is at its maximum).  Used by stateChanged() to avoid hiding a brand-new
      message when transitioning states.
    */
    bool msgJustShown() const { return myMsg.counter >= MESSAGE_TIME - 1; }

    /**
      Draw the pending text/gauge message.
      Returns true when the surface changed (signals FrameBuffer to redraw).
    */
    bool draw();

    /**
      Draw the frame-statistics overlay.
    */
    void drawStats(float framesPerSecond);

    /**
      Hide the pending message immediately.
    */
    void hide();

  private:
    struct Message {
      string text;
      int counter{-1};
      int x{0}, y{0}, w{0}, h{0};
      MessagePosition position{MessagePosition::BottomCenter};
      ColorId color{kNone};
      shared_ptr<FBSurface> surface;
      bool enabled{false};
      bool dirty{false};
      bool showGauge{false};
      float value{0.F};
      string valueText;
    };

    /**
      Internal helper: apply text/gauge state to myMsg and mark it active.
    */
    void create(string_view message, MessagePosition position, bool force = false);

    FrameBuffer& myFB;
    OSystem&     myOSystem;

    Message myMsg;
    Message myStatsMsg;
    bool    myStatsEnabled{false};
    uInt32  myLastScanlines{0};
    Int32   myPausedCount{0};

    static constexpr int MESSAGE_WIDTH  = 56;
    static constexpr int GAUGEBAR_WIDTH = 30;
    static constexpr int MESSAGE_TIME   = 120; // frames (~2 seconds at 60 fps)

#ifdef GUI_SUPPORT
    // showText() mutates myMsg and allocates GUI surfaces, so it must run on
    // the main (render) thread.  Calls arriving from other threads (e.g. a
    // cartridge's message callback on the emulation worker thread) are queued
    // here and replayed on the main thread by drainPending(), avoiding a data
    // race with draw().
    struct PendingMessage {
      string text;
      MessagePosition position{MessagePosition::BottomCenter};
      bool force{false};
    };
    const std::thread::id myMainThreadId{std::this_thread::get_id()};
    std::mutex myPendingMutex;
    vector<PendingMessage> myPending;
#endif  // GUI_SUPPORT

  private:
    // Following constructors and assignment operators not supported
    FBMessageHandler() = delete;
    FBMessageHandler(const FBMessageHandler&) = delete;
    FBMessageHandler(FBMessageHandler&&) = delete;
    FBMessageHandler& operator=(const FBMessageHandler&) = delete;
    FBMessageHandler& operator=(FBMessageHandler&&) = delete;
};

#endif  // FB_MESSAGE_HANDLER_HXX
