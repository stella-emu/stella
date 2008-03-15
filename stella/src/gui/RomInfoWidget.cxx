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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RomInfoWidget.cxx,v 1.7 2008-03-15 19:11:00 stephena Exp $
//============================================================================

#include <cstring>
#include <zlib.h>

#include "FrameBuffer.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "Surface.hxx"
#include "Widget.hxx"

#include "RomInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomInfoWidget::RomInfoWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    mySurface(NULL),
    myDrawSurface(false),
    myHaveProperties(false)
{
  _flags = WIDGET_ENABLED | WIDGET_RETAIN_FOCUS;
  _bgcolor = _bgcolorhi = kWidColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomInfoWidget::~RomInfoWidget()
{
  if(mySurface)
  {
    delete mySurface;
    mySurface = NULL;
  }
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
  if(instance()->eventHandler().state() == EventHandler::S_LAUNCHER)
  {
    parseProperties();
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::clearProperties()
{
  myHaveProperties = myDrawSurface = false;

  // Decide whether the information should be shown immediately
  if(instance()->eventHandler().state() == EventHandler::S_LAUNCHER)
  {
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::initialize()
{
  // Delete surface; a new one will be created by parseProperties
  delete mySurface;
  mySurface = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::parseProperties()
{
  // Initialize to empty properties entry
  mySurfaceErrorMsg = "";
  myDrawSurface = false;
  myRomInfo.clear();

  // Check if a surface has ever been created; if so, we use it
  // The surface will always be the maximum size, but sometimes we'll
  // only draw certain parts of it
  if(mySurface == NULL)
    mySurface = instance()->frameBuffer().createSurface(320, 260);

  // The input stream for the PNG file
  ifstream in;

  // Contents of each PNG chunk
  string type = "";
  uInt8* data = NULL;
  int size = 0;

  // 'tEXt' chucks from the PNG image
  StringList textChucks;

  // Get a valid filename representing a snapshot file for this rom
  const string& filename =
    instance()->settings().getString("ssdir") + BSPF_PATH_SEPARATOR +
    myProperties.get(Cartridge_Name) + ".png";

  // Open the PNG and check for a valid signature
  in.open(filename.c_str(), ios_base::binary);
  if(in)
  {
    try
    {
      uInt8 header[8];
      in.read((char*)header, 8);
      if(!isValidPNGHeader(header))
        throw "Invalid PNG image";

      // Read all chunks until we reach the end
      int width = 0, height = 0;
      while(type != "IEND" && !in.eof())
      {
        readPNGChunk(in, type, &data, size);

        if(type == "IHDR")
        {
          if(!parseIHDR(width, height, data, size))
            throw "Invalid PNG image (IHDR)";

          mySurface->setClipWidth(width);
          mySurface->setClipHeight(height);
        }
        else if(type == "IDAT")
        {
          if(!parseIDATChunk(instance()->frameBuffer(), mySurface,
                             width, height, data, size))
            throw "PNG image too large";
        }
        else if(type == "tEXt")
          textChucks.push_back(parseTextChunk(data, size));

        delete[] data;  data = NULL;
      }

      in.close();
      myDrawSurface = true;
    }
    catch(const char* msg)
    {
      myDrawSurface = false;
      myRomInfo.clear();
      if(data) delete[] data;
      data = NULL;
      in.close();

      mySurfaceErrorMsg = msg;
    }
  }
  else
    mySurfaceErrorMsg = "No image found";

  // Now add some info for the message box below the image
  myRomInfo.push_back("Name:  " + myProperties.get(Cartridge_Name));
  myRomInfo.push_back("Manufacturer:  " + myProperties.get(Cartridge_Manufacturer));
  myRomInfo.push_back("Model:  " + myProperties.get(Cartridge_ModelNo));
  myRomInfo.push_back("Rarity:  " + myProperties.get(Cartridge_Rarity));
  myRomInfo.push_back("Note:  " + myProperties.get(Cartridge_Note));
  myRomInfo.push_back("Controllers:  " + myProperties.get(Controller_Left) +
                      " (left), " + myProperties.get(Controller_Right) + " (right)");
  // TODO - add the PNG tEXt chunks
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = instance()->frameBuffer();

  fb.fillRect(_x+2, _y+2, _w-4, _h-4, kWidColor);
  fb.box(_x, _y, _w, _h, kColor, kShadowColor);
  fb.box(_x, _y+264, _w, _h-264, kColor, kShadowColor);

  if(!myHaveProperties) return;

  if(myDrawSurface && mySurface)
  {
    int x = (_w - mySurface->getClipWidth()) >> 1;
    int y = (266 - mySurface->getClipHeight()) >> 1;
    fb.drawSurface(mySurface, x + getAbsX(), y + getAbsY());
  }
  else if(mySurfaceErrorMsg != "")
  {
    int x = _x + ((_w - _font->getStringWidth(mySurfaceErrorMsg)) >> 1);
    fb.drawString(_font, mySurfaceErrorMsg, x, 120, _w - 10, _textcolor);
  }
  int xpos = _x + 5, ypos = _y + 266 + 5;
  for(unsigned int i = 0; i < myRomInfo.size(); ++i)
  {
    fb.drawString(_font, myRomInfo[i], xpos, ypos, _w - 10, _textcolor);
    ypos += _font->getLineHeight();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomInfoWidget::isValidPNGHeader(uInt8* header)
{
  // Unique signature indicating a PNG image file
  uInt8 signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

  return memcmp(header, signature, 8) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::readPNGChunk(ifstream& in, string& type,
                                 uInt8** data, int& size)
{
  uInt8 temp[9];
  temp[8] = '\0';

  // Get the size and type from the 8-byte header
  in.read((char*)temp, 8);
  size = temp[0] << 24 | temp[1] << 16 | temp[2] << 8 | temp[3];
  type = string((const char*)temp+4);

  // Now read the payload
  if(size > 0)
  {
    *data = new uInt8[size];
    in.read((char*) *data, size);
  }

  // Read (and discard) the 4-byte CRC
  in.read((char*)temp, 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomInfoWidget::parseIHDR(int& width, int& height, uInt8* data, int size)
{
  // We only support the PNG functionality defined in Snapshot.cxx
  // Specifically, 24 bpp RGB data; any other formats are ignored

  if(size != 13)
    return false;

  width  = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
  height = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];

  // Make sure image can fit in widget bounds
  width  = BSPF_min(320, width);
  height = BSPF_min(260, height);

  uInt8 trailer[5] = { 8, 2, 0, 0, 0 };  // 24-bit RGB
  return memcmp(trailer, data + 8, 5) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomInfoWidget::parseIDATChunk(const FrameBuffer& fb, GUI::Surface* surface,
                                   int width, int height, uInt8* data, int size)
{
  // The entire decompressed image data
  uLongf bufsize = (width * 3 + 1) * height;
  uInt8* buffer = new uInt8[bufsize];

  int pitch = width * 3;  // bytes per line of the image
  if(uncompress(buffer, &bufsize, data, size) == Z_OK)
  {
    uInt8* buf_ptr = buffer;
    for(int row = 0; row < height; row++, buf_ptr += pitch)
    {
      buf_ptr++;           // skip past first byte (PNG filter type)
      fb.bytesToSurface(surface, row, buf_ptr, pitch);
    }
    delete[] buffer;
    return true;
  }
  delete[] buffer;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RomInfoWidget::parseTextChunk(uInt8* data, int size)
{
  return "";
}
