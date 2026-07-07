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

#ifndef LAUNCHER_HXX
#define LAUNCHER_HXX

class Properties;
class OSystem;
class FSNode;
class LauncherDialog;

#include "Rect.hxx"
#include "FrameBufferConstants.hxx"
#include "DialogContainer.hxx"

/**
  The base dialog for the ROM launcher in Stella.

  @author  Stephen Anthony
*/
class Launcher : public DialogContainer
{
  public:
    /**
      Create a new menu stack
    */
    explicit Launcher(OSystem& osystem);
    ~Launcher() override;

    /**
      Initialize the video subsystem wrt this class.
    */
    FBInitStatus initializeVideo();

    /**
      Wrapper for LauncherDialog::selectedRom() method.
    */
    const string& selectedRom();

    /**
      Wrapper for LauncherDialog::selectedRomMD5() method.
    */
    const string& selectedRomMD5();

    /**
      Wrapper for LauncherDialog::currentDir() method.
    */
    const FSNode& currentDir() const;

    /**
      Wrapper for LauncherDialog::reload() method.
    */
    void reload();

    /**
      Wrapper for LauncherDialog::quit() method.
    */
    void quit();

    /**
      Return (and possibly create) the bottom-most dialog of this container.
    */
    Dialog* baseDialog() override;

    /**
      Live window-resize handling.  EventHandler records the latest dragged size
      (FrameBuffer::liveResize) and drives applyResize() to apply + re-flow it,
      so the window shows live content as it is dragged rather than a stretched
      snapshot updated at the end.  updateTime() flushes any size the throttle
      skipped and, once the drag settles, runs one final re-flow with
      resizeInProgress() false so the deferred finalization (minimum-size hint,
      size persistence) happens exactly once per drag.
    */
    void updateTime(uInt64 time) override;
    bool applyResize() override;
    bool resizeInProgress() const override { return mySettleCountdown > 0; }

  private:
    /**
      (Re)load the launcher window size from the 'launcherres' setting,
      clamping it to the allowed bounds.  Called both at construction and each
      time the launcher video is (re)initialized, so a size the user changed
      by resizing the window is picked up again.
    */
    void loadSize();

  private:
    unique_ptr<LauncherDialog> myBaseDialog;

    // The dimensions of this dialog
    Common::Size mySize;

    // Frames of idle left before a live resize counts as settled
    int mySettleCountdown{0};

    // Time (microseconds) of the last applied live resize, for throttling
    uInt64 myLastResizeTime{0};

  private:
    // Following constructors and assignment operators not supported
    Launcher() = delete;
    Launcher(const Launcher&) = delete;
    Launcher(Launcher&&) = delete;
    Launcher& operator=(const Launcher&) = delete;
    Launcher& operator=(Launcher&&) = delete;
};

#endif  // LAUNCHER_HXX
