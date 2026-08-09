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

#ifndef LOGGER_DIALOG_HXX
#define LOGGER_DIALOG_HXX

class GuiObject;
class CheckboxWidget;
class PopUpWidget;
class LabelWidget;
class StringListWidget;

#include "Dialog.hxx"
#include "bspf.hxx"

/**
  Shows accumulated log messages, with controls for log level and
  console echo, and a save-to-disk option.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class LoggerDialog : public Dialog
{
  public:
    LoggerDialog(OSystem& osystem, DialogContainer& parent,
                 const GUI::Font& font, int max_w, int max_h,
                 bool useLargeFont = true);
    ~LoggerDialog() override = default;

    // Populates the log listing/level/console-echo controls from Logger and settings
    void loadConfig() override;
    // Saves the log level/console-echo settings and applies them to Logger
    void saveConfig() override;

  protected:
    // OK saves and closes; Defaults ("Save log to disk") opens a save-file browser
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void layout() override;

  private:
    // Writes the current log to 'node', reporting success/failure via a status message
    void saveLogFile(const FSNode& node);

  private:
    // The scrollable log output itself
    StringListWidget* myLogInfo{nullptr};
    // How much detail is logged (see Logger::Level)
    LabelWidget* myLogLevelLbl{nullptr};
    PopUpWidget*      myLogLevel{nullptr};
    // Whether log output is also echoed to the console
    CheckboxWidget*   myLogToConsole{nullptr};

  private:
    // Following constructors and assignment operators not supported
    LoggerDialog() = delete;
    LoggerDialog(const LoggerDialog&) = delete;
    LoggerDialog(LoggerDialog&&) = delete;
    LoggerDialog& operator=(const LoggerDialog&) = delete;
    LoggerDialog& operator=(LoggerDialog&&) = delete;
};

#endif  // LOGGER_DIALOG_HXX
