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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Dialog.hxx,v 1.6 2005-03-15 22:28:05 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef DIALOG_HXX
#define DIALOG_HXX

class OSystem;
class Menu;

#include "Command.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"

#include "bspf.hxx"

/**
  This is the base class for all dialog boxes.
  
  @author  Stephen Anthony
  @version $Id: Dialog.hxx,v 1.6 2005-03-15 22:28:05 stephena Exp $
*/
class Dialog : public GuiObject
{
  friend class Menu;

  public:
    Dialog(OSystem* instance, uInt16 x, uInt16 y, uInt16 w, uInt16 h);

    virtual ~Dialog();

    virtual int runModal();

    bool isVisible() const { return _visible; }

    virtual void open();
    virtual void close();
    virtual void drawDialog();
    virtual void loadConfig() {}
    virtual void saveConfig() {}

  protected:
    virtual void draw();
    void releaseFocus();

    virtual void handleTickle(); // Called periodically (in every guiloop() )
    virtual void handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers);
    virtual void handleKeyUp(uInt16 ascii, Int32 keycode, Int32 modifiers);
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual void handleMouseMoved(int x, int y, int button);
    virtual void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data);
    virtual void handleScreenChanged() {}
	
    Widget* findWidget(int x, int y); // Find the widget at pos x,y if any

    ButtonWidget* addButton(int x, int y, const string& label, uInt32 cmd, char hotkey);

    void setResult(int result) { _result = result; }
    int getResult() const { return _result; }

  protected:
    Widget* _mouseWidget;
    Widget* _focusedWidget;
    bool    _visible;
    uInt32  _openCount;

  private:
    int _result;
};

#endif
