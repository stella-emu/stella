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
      Debounced window-resize handling: re-flow the launcher once the user
      stops dragging, rather than on every resize event (mirrors the
      debugger).  requestResize() just restarts the idle countdown;
      updateTime() performs the actual re-flow once it expires.
    */
    void requestResize() override;
    void updateTime(uInt64 time) override;

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

    // Debounced window-resize state: re-flow only after dragging settles
    bool myResizePending{false};
    int  myResizeCountdown{0};

  private:
    // Following constructors and assignment operators not supported
    Launcher() = delete;
    Launcher(const Launcher&) = delete;
    Launcher(Launcher&&) = delete;
    Launcher& operator=(const Launcher&) = delete;
    Launcher& operator=(Launcher&&) = delete;
};

#endif  // LAUNCHER_HXX
