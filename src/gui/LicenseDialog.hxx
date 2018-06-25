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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef LICENSE_DIALOG_HXX
#define LICENSE_DIALOG_HXX

class GuiObject;
class CheckboxWidget;
class PopUpWidget;
class StringListWidget;
class StringList;

#include "Dialog.hxx"
#include "bspf.hxx"


class LicenseDialog : public Dialog
{
  public:
    LicenseDialog(OSystem* osystem, DialogContainer* parent,
                 const GUI::Font& font, int max_w, int max_h);
    virtual ~LicenseDialog();
    void setFilename(string name);

  protected:
    void loadConfig();
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    virtual void handleJoyDown(int stick, int button);
  private:
    StringListWidget* myLogInfo;
    StringList myLicenseTxt;
    int _lineHeight;
    int _buttonWidth;
    int _buttonHeight;
};

#endif
