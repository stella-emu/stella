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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef LOGGER_DIALOG_HXX
#define LOGGER_DIALOG_HXX

class GuiObject;
class CheckboxWidget;
class PopUpWidget;
class StringListWidget;

#include "Dialog.hxx"
#include "bspf.hxx"


class LoggerDialog : public Dialog
{
  public:
    LoggerDialog(OSystem* osystem, DialogContainer* parent, const GUI::Font& font, int max_w, int max_h);
    virtual ~LoggerDialog();

  protected:
    void loadConfig();
    void saveConfig();
    void saveLogFile();

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    StringListWidget* myLogInfo;
    PopUpWidget*      myLogLevel;
    CheckboxWidget*   myLogToConsole;
};

#endif
