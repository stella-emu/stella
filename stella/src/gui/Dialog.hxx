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
// $Id: Dialog.hxx,v 1.1 2005-02-27 23:41:19 stephena Exp $
//============================================================================

#ifndef DIALOG_HXX
#define DIALOG_HXX

#include "bspf.hxx"

#include "common/scummsys.h"
#include "common/str.h"

#include "gui/object.h"

/**
  This is the base class for all dialog boxes.
  
  @author  Stephen Anthony
  @version $Id: Dialog.hxx,v 1.1 2005-02-27 23:41:19 stephena Exp $
*/
class Dialog
{
  public:
    Dialog(uInt16 x, uInt16 y, uInt16 w, uInt16 h)
		: GuiObject(x, y, w, h),
		  _mouseWidget(0), _focusedWidget(0), _visible(false) {
	}
	virtual ~Dialog();

	virtual int runModal();

	bool 	isVisible() const	{ return _visible; }

	void	releaseFocus();

protected:
	virtual void open();
	virtual void close();
	
	virtual void draw();
	virtual void drawDialog();

	virtual void handleTickle(); // Called periodically (in every guiloop() )
	virtual void handleMouseDown(int x, int y, int button, int clickCount);
	virtual void handleMouseUp(int x, int y, int button, int clickCount);
	virtual void handleMouseWheel(int x, int y, int direction);
	virtual void handleKeyDown(uint16 ascii, int keycode, int modifiers);
	virtual void handleKeyUp(uint16 ascii, int keycode, int modifiers);
	virtual void handleMouseMoved(int x, int y, int button);
	virtual void handleCommand(CommandSender *sender, uint32 cmd, uint32 data);
	virtual void handleScreenChanged() {}
	
	Widget *findWidget(int x, int y); // Find the widget at pos x,y if any

	ButtonWidget *addButton(int x, int y, const Common::String &label, uint32 cmd, char hotkey);

	void setResult(int result) { _result = result; }
	int getResult() const { return _result; }


protected:
	Widget	*_mouseWidget;
	Widget  *_focusedWidget;
	bool	_visible;

private:
	int		_result;

};

} // End of namespace GUI

#endif
