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

#ifndef OVERLAY_MENU_HXX
#define OVERLAY_MENU_HXX

class OSystem;
class Dialog;

#include <array>

#include "bspf.hxx"
#include "DialogContainer.hxx"

/**
  The single DialogContainer for all dialogs that overlay TIA mode.

  It serves two roles:
    - The built-in menus (options, command, high scores, message, PlusROM)
      are selected by the current EventHandler state in baseDialog() and
      lazily created/cached on first use.
    - Any other dialog can be shown over TIA via setDialog() (see
      EventHandler::openDialog), which takes ownership of a transient dialog.

  Adding a new overlay:

    A) Transient/one-off dialog (the common case) - nothing to change here.
       Just call eventHandler().openDialog(new FooDialog(...)) for an owned
       dialog, or eventHandler().openBrowserDialog(...) for a file browser.
       It is shown via the generic OVERLAYMENU state and freed on replacement.

    B) A new persistent built-in menu (its own hotkey/toggle, like options):
       1. Add an EventHandlerState value in EventHandlerConstants.hxx.
       2. Add a Cached enum entry below and a matching createDialog() case.
       3. Add a case to baseDialog() mapping the new state to that cached
          dialog (in OverlayMenu.cxx).
       4. In EventHandler::setState(), add the new state to the fall-through
          group that sets myOverlay = &overlayMenu().
       5. Add the open/toggle handler for its Event::XxxMenuMode in
          EventHandler::handleEvent().
       FrameBuffer needs no change: the generic GUI-overlay render case and
       OSystem::createFrameBuffer() (which keys off hasConsole()) cover it.

  @author  Stephen Anthony
*/
class OverlayMenu : public DialogContainer
{
  public:
    explicit OverlayMenu(OSystem& osystem);
    ~OverlayMenu() override;

    // Take ownership of a transient dialog (deletes any previously held one)
    // and make it the active base dialog for the OVERLAYMENU state.
    void setDialog(Dialog* dialog);

    // Return the base dialog for the current EventHandler state, lazily
    // creating and caching the matching built-in menu dialog as needed.
    Dialog* baseDialog() override;

  private:
    // The concrete built-in menu dialogs, each cached on first use
    enum class Cached: uInt8 {
      Options, StellaSettings, Command,
      HighScores, Message, PlusRoms, NumCached
    };

    // Return the cached dialog for 'id', creating it on first access
    Dialog& cached(Cached id);
    Dialog* createDialog(Cached id);

  private:
    std::array<unique_ptr<Dialog>,
               static_cast<size_t>(Cached::NumCached)> myCached;
    unique_ptr<Dialog> myTransientDialog;

  private:
    // Following constructors and assignment operators not supported
    OverlayMenu() = delete;
    OverlayMenu(const OverlayMenu&) = delete;
    OverlayMenu(OverlayMenu&&) = delete;
    OverlayMenu& operator=(const OverlayMenu&) = delete;
    OverlayMenu& operator=(OverlayMenu&&) = delete;
};

#endif  // OVERLAY_MENU_HXX
