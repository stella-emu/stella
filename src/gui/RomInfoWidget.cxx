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

#include "FrameBuffer.hxx"
#include "OSystem.hxx"
#include "PNGLibrary.hxx"
#include "Settings.hxx"
#include "Widget.hxx"

#include "RomInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomInfoWidget::RomInfoWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    mySurface(NULL),
    mySurfaceID(-1),
    myZoomLevel(w > 400 ? 2 : 1),
    mySurfaceIsValid(false),
    myHaveProperties(false)
{
  _flags = WIDGET_ENABLED;
  _bgcolor = _bgcolorhi = kWidColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomInfoWidget::~RomInfoWidget()
{
  myRomInfo.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::loadConfig()
{
  // The ROM may have changed since we were last in the browser, either
  // by saving a different image or through a change in video renderer,
  // so we reload the properties
  if(myHaveProperties)
  {
    parseProperties();
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::setProperties(const Properties& props)
{
  myHaveProperties = true;
  myProperties = props;

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandler::S_LAUNCHER)
  {
    parseProperties();
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::clearProperties()
{
  myHaveProperties = mySurfaceIsValid = false;

  // Decide whether the information should be shown immediately
  if(instance().eventHandler().state() == EventHandler::S_LAUNCHER)
  {
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::parseProperties()
{
  // Check if a surface has ever been created; if so, we use it
  // The surface will always be the maximum size, but sometimes we'll
  // only draw certain parts of it
  mySurface = instance().frameBuffer().surface(mySurfaceID);
  if(mySurface == NULL)
  {
    mySurfaceID = instance().frameBuffer().allocateSurface(
                    320*myZoomLevel, 256*myZoomLevel, false);
    mySurface   = instance().frameBuffer().surface(mySurfaceID);
  }
  else
  {
    mySurface->setWidth(320*myZoomLevel);
    mySurface->setHeight(256*myZoomLevel);
  }

  // Initialize to empty properties entry
  mySurfaceErrorMsg = "";
  mySurfaceIsValid = false;
  myRomInfo.clear();

  // Get a valid filename representing a snapshot file for this rom
  const string& filename = instance().snapshotDir() +
      myProperties.get(Cartridge_Name) + ".png";

  // Read the PNG file
  try
  {
    mySurfaceIsValid =
      myPNGLib.readImage(filename, instance().frameBuffer(), *mySurface);
  }
  catch(const char* msg)
  {
    mySurfaceIsValid = false;
    mySurfaceErrorMsg = msg;
  }

  // Now add some info for the message box below the image
  myRomInfo.push_back("Name:  " + myProperties.get(Cartridge_Name));
  myRomInfo.push_back("Manufacturer:  " + myProperties.get(Cartridge_Manufacturer));
  myRomInfo.push_back("Model:  " + myProperties.get(Cartridge_ModelNo));
  myRomInfo.push_back("Rarity:  " + myProperties.get(Cartridge_Rarity));
  myRomInfo.push_back("Note:  " + myProperties.get(Cartridge_Note));
  myRomInfo.push_back("Controllers:  " + myProperties.get(Controller_Left) +
                      " (left), " + myProperties.get(Controller_Right) + " (right)");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();

  const int yoff = myZoomLevel > 1 ? 260*2 + 10 : 275;

  s.fillRect(_x+2, _y+2, _w-4, _h-4, kWidColor);
  s.box(_x, _y, _w, _h, kColor, kShadowColor);
  s.box(_x, _y+yoff, _w, _h-yoff, kColor, kShadowColor);

  if(!myHaveProperties) return;

  if(mySurfaceIsValid)
  {
    uInt32 x = _x + ((_w - mySurface->getWidth()) >> 1);
    uInt32 y = _y + ((yoff - mySurface->getHeight()) >> 1);
    s.drawSurface(mySurface, x, y);
  }
  else if(mySurfaceErrorMsg != "")
  {
    const GUI::Font& font = instance().font();
    uInt32 x = _x + ((_w - font.getStringWidth(mySurfaceErrorMsg)) >> 1);
    uInt32 y = _y + ((yoff - font.getLineHeight()) >> 1);
    s.drawString(font, mySurfaceErrorMsg, x, y, _w - 10, _textcolor);
  }
  int xpos = _x + 5, ypos = _y + yoff + 10;
  for(unsigned int i = 0; i < myRomInfo.size(); ++i)
  {
    s.drawString(_font, myRomInfo[i], xpos, ypos, _w - 10, _textcolor);
    ypos += _font.getLineHeight();
  }
}

/*
cerr << "surface:" << endl
	<< "  w = " << sw << endl
	<< "  h = " << sh << endl
	<< "  szoom = " << myZoomLevel << endl
	<< "  spitch = " << spitch << endl
	<< endl;

cerr << "image:" << endl
	<< "  width  = " << width << endl
	<< "  height = " << height << endl
	<< "  izoom = " << izoom << endl
	<< "  ipitch = " << ipitch << endl
	<< "  bufsize = " << bufsize << endl
	<< "  buf_offset = " << buf_offset << endl
	<< "  i_offset = " << i_offset << endl
	<< endl;
*/
