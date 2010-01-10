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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cstring>
#include <cmath>
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
    // For some reason, we need to allocate a buffer slightly higher than
    // the maximum that Stella can generate
    mySurfaceID = instance().frameBuffer().allocateSurface(
                    320*myZoomLevel, 257*myZoomLevel, false);
    mySurface   = instance().frameBuffer().surface(mySurfaceID);
  }

  // Initialize to empty properties entry
  mySurfaceErrorMsg = "";
  mySurfaceIsValid = false;
  myRomInfo.clear();

  // The input stream for the PNG file
  ifstream in;

  // Contents of each PNG chunk
  string type = "";
  uInt8* data = NULL;
  int size = 0;

  // 'tEXt' chucks from the PNG image
  StringList textChucks;

  // Get a valid filename representing a snapshot file for this rom
  const string& filename = instance().snapshotDir() + BSPF_PATH_SEPARATOR +
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
        }
        else if(type == "IDAT")
        {
          if(!parseIDATChunk(mySurface, width, height, data, size))
            throw "Invalid PNG image (IDAT)";
        }
        else if(type == "tEXt")
          textChucks.push_back(parseTextChunk(data, size));

        delete[] data;  data = NULL;
      }

      in.close();
      mySurfaceIsValid = true;
    }
    catch(const char* msg)
    {
      mySurfaceIsValid = false;
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
    const GUI::Font* font = &instance().font();
    uInt32 x = _x + ((_w - font->getStringWidth(mySurfaceErrorMsg)) >> 1);
    uInt32 y = _y + ((yoff - font->getLineHeight()) >> 1);
    s.drawString(font, mySurfaceErrorMsg, x, y, _w - 10, _textcolor);
  }
  int xpos = _x + 5, ypos = _y + yoff + 10;
  for(unsigned int i = 0; i < myRomInfo.size(); ++i)
  {
    s.drawString(_font, myRomInfo[i], xpos, ypos, _w - 10, _textcolor);
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
bool RomInfoWidget::parseIDATChunk(FBSurface* surface, int width, int height,
                                   uInt8* data, int size)
{
  // Figure out the original zoom level of the snapshot
  // All snapshots generated by Stella are at most some multiple of 320
  // pixels wide
  // The only complication is when the aspect ratio is changed, the width
  // can range from 256 (80%) to 320 (100%)
  // The following calculation will work up to approx. 16x zoom level,
  // but since Stella only generates snapshots at up to 10x, we should
  // be fine for a while ...
  uInt32 izoom = uInt32(ceil(width/320.0));

  // Set the surface size 
  uInt32 sw = width / izoom * myZoomLevel,
         sh = height / izoom * myZoomLevel;
  sw = BSPF_min(sw, myZoomLevel * 320u);
  sh = BSPF_min(sh, myZoomLevel * 256u);
  mySurface->setWidth(sw);
  mySurface->setHeight(sh);

  // Decompress the image, and scale it correctly
  uInt32 ipitch = width * 3 + 1;   // bytes per line of the actual PNG image
  uLongf bufsize = ipitch * height;
  uInt8* buffer = new uInt8[bufsize];
  uInt32* line  = new uInt32[ipitch];

  if(uncompress(buffer, &bufsize, data, size) == Z_OK)
  {
    uInt8* buf_ptr = buffer + 1;  // skip past first column (PNG filter type)
    uInt32 buf_offset = ipitch * izoom;
    uInt32 i_offset = 3 * izoom;
    uInt32 srow = 0;

    // We can only scan at most izoom*256 lines
    height = BSPF_min(uInt32(height), izoom*256u);

    // Grab each non-duplicate row of data from the image
    for(int irow = 0; irow < height; irow += izoom, buf_ptr += buf_offset)
    {
      // Scale the image data into the temporary line buffer
      uInt8*  i_ptr = buf_ptr;
      uInt32* l_ptr = line;
      for(int icol = 0; icol < width; icol += izoom, i_ptr += i_offset)
      {
        uInt32 pixel =
          instance().frameBuffer().mapRGB(*i_ptr, *(i_ptr+1), *(i_ptr+2));
        uInt32 xstride = myZoomLevel;
        while(xstride--)
          *l_ptr++ = pixel;
      }

      // Then fill the surface with those bytes
      uInt32 ystride = myZoomLevel;
      while(ystride--)
        surface->drawPixels(line, 0, srow++, sw);
    }
    delete[] buffer;
    delete[] line;
    return true;
  }
  delete[] buffer;
  delete[] line;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RomInfoWidget::parseTextChunk(uInt8* data, int size)
{
  return "";
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
