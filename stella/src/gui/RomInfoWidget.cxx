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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RomInfoWidget.cxx,v 1.2 2007-09-03 18:37:23 stephena Exp $
//============================================================================

#include <zlib.h>
#include <cstring>

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
    mySurface(NULL)
{
  _flags = WIDGET_ENABLED | WIDGET_RETAIN_FOCUS;
  _bgcolor = _bgcolorhi = kWidColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomInfoWidget::~RomInfoWidget()
{
  clearInfo(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::loadConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::showInfo(const Properties& props)
{
  // The input stream for the PNG file
  ifstream in;

  // Contents of each PNG chunk
  string type = "";
  uInt8* data = NULL;
  int size = 0;

  // 'tEXt' chucks from the PNG image
  StringList textChucks;

  // Get a valid filename representing a snapshot file for this rom
  const string& path = instance()->settings().getString("ssdir");
  const string& filename = instance()->getFilename(path, props, "png");

  // Open the PNG and check for a valid signature
  clearInfo(false);
  in.open(filename.c_str(), ios_base::binary);
  if(in)
  {
    try
    {
      uInt8 header[8];
      in.read((char*)header, 8);
      if(!isValidPNGHeader(header))
        throw "RomInfoWidget: Not a PNG image";

      // Read all chunks until we reach the end
      int width = 0, height = 0;
      while(type != "IEND" && !in.eof())
      {
        readPNGChunk(in, type, &data, size);

        if(type == "IHDR" && !parseIHDR(width, height, data, size))
          throw "RomInfoWidget: IHDR chunk not supported";
        else if(type == "IDAT")
        {
          // Restrict surface size to available space
          int s_width = BSPF_min(320, width);
          int s_height = BSPF_min(250, height);

          FrameBuffer& fb = instance()->frameBuffer();
          mySurface = fb.createSurface(s_width, s_height);
          if(!parseIDATChunk(fb, mySurface, width, height, data, size))
            throw "RomInfoWidget: IDAT processing failed";
        }
        else if(type == "tEXt")
          textChucks.push_back(parseTextChunk(data, size));

        delete[] data;  data = NULL;
      }

      in.close();
    }
    catch(const char *msg)
    {
      clearInfo(false);
      if(data) delete[] data;
      data = NULL;

      cerr << msg << endl;
    }
  }
  // Now add some info for the message box below the image
  myRomInfo.push_back("Name:  " + props.get(Cartridge_Name));
  myRomInfo.push_back("Manufacturer:  " + props.get(Cartridge_Manufacturer));
  myRomInfo.push_back("Model:  " + props.get(Cartridge_ModelNo));
  myRomInfo.push_back("Rarity:  " + props.get(Cartridge_Rarity));
  myRomInfo.push_back("Note:  " + props.get(Cartridge_Note));
  myRomInfo.push_back("Controllers:  " + props.get(Controller_Left) +
                      " (left), " + props.get(Controller_Right) + " (right)");
  // TODO - add the PNG tEXt chunks

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::clearInfo(bool redraw)
{
  if(mySurface)
    delete mySurface;
  mySurface = NULL;
  myRomInfo.clear();

  if(redraw)
  {
    setDirty(); draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomInfoWidget::drawWidget(bool hilite)
{
  FrameBuffer& fb = instance()->frameBuffer();

  fb.fillRect(_x+2, _y+2, _w-4, _h-4, kWidColor);
  fb.box(_x, _y, _w, _h, kColor, kShadowColor);
  fb.box(_x, _y+254, _w, _h-254, kColor, kShadowColor);

  if(mySurface)
  {
    int x = (_w - mySurface->getWidth()) >> 1;
    int y = (256 - mySurface->getHeight()) >> 1;
    fb.drawSurface(mySurface, x + getAbsX(), y + getAbsY());
  }
  int xpos = _x + 5, ypos = _y + 256 + 5;
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

  // Only get as many scanlines as necessary to fill the surface
  height = BSPF_min(250, height);

  if(uncompress(buffer, &bufsize, data, size) == Z_OK)
  {
    uInt8* buf_ptr = buffer;
    int pitch = width * 3;  // bytes per line of the image
    for(int row = 0; row < height; row++, buf_ptr += pitch)
    {
      buf_ptr++;           // skip past first byte (PNG filter type)
      fb.bytesToSurface(surface, row, buf_ptr);
    }
  }
  else
  {
    cerr << "RomInfoWidget: error decompressing data\n";
    return false;
  }

  delete[] buffer;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RomInfoWidget::parseTextChunk(uInt8* data, int size)
{
  return "";
}
