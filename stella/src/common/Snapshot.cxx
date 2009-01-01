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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Snapshot.cxx,v 1.24 2009-01-01 18:13:35 stephena Exp $
//============================================================================

#include <zlib.h>
#include <fstream>
#include <cstring>
#include <sstream>

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "Props.hxx"
#include "Version.hxx"
#include "Snapshot.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Snapshot::savePNG(FrameBuffer& framebuffer, const Properties& props,
                       const string& filename)
{
  uInt8* buffer  = (uInt8*) NULL;
  uInt8* compmem = (uInt8*) NULL;
  ofstream out;

  try
  {
    // Make sure we have a 'clean' image, with no onscreen messages
    framebuffer.enableMessages(false);

    // Get actual image dimensions. which are not always the same
    // as the framebuffer dimensions
    const GUI::Rect& image = framebuffer.imageRect();
    int width = image.width(), height = image.height();

    out.open(filename.c_str(), ios_base::binary);
    if(!out.is_open())
      throw "ERROR: Couldn't create snapshot file";

    // PNG file header
    uInt8 header[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    out.write((const char*)header, 8);

    // PNG IHDR
    uInt8 ihdr[13];
    ihdr[0]  = width >> 24;   // width
    ihdr[1]  = width >> 16;
    ihdr[2]  = width >> 8;
    ihdr[3]  = width & 0xFF;
    ihdr[4]  = height >> 24;  // height
    ihdr[5]  = height >> 16;
    ihdr[6]  = height >> 8;
    ihdr[7]  = height & 0xFF;
    ihdr[8]  = 8;  // 8 bits per sample (24 bits per pixel)
    ihdr[9]  = 2;  // PNG_COLOR_TYPE_RGB
    ihdr[10] = 0;  // PNG_COMPRESSION_TYPE_DEFAULT
    ihdr[11] = 0;  // PNG_FILTER_TYPE_DEFAULT
    ihdr[12] = 0;  // PNG_INTERLACE_NONE
    writePNGChunk(out, "IHDR", ihdr, 13);

    // Fill the buffer with scanline data
    int rowbytes = width * 3;
    buffer = new uInt8[(rowbytes + 1) * height];
    uInt8* buf_ptr = buffer;
    for(int row = 0; row < height; row++)
    {
      *buf_ptr++ = 0;                      // first byte of row is filter type
      framebuffer.scanline(row, buf_ptr);  // get another scanline
      buf_ptr += rowbytes;                 // add pitch
    }

    // Compress the data with zlib
    uLongf compmemsize = (uLongf)((height * (width + 1) * 3 * 1.001 + 1) + 12);
    compmem = new uInt8[compmemsize];
    if(compmem == NULL ||
       (compress(compmem, &compmemsize, buffer, height * (width * 3 + 1)) != Z_OK))
      throw "ERROR: Couldn't compress PNG";

    // Write the compressed framebuffer data
    writePNGChunk(out, "IDAT", compmem, compmemsize);

    // Add some info about this snapshot
    writePNGText(out, "Software", string("Stella ") + STELLA_VERSION);
    writePNGText(out, "ROM Name", props.get(Cartridge_Name));
    writePNGText(out, "ROM MD5", props.get(Cartridge_MD5));
    writePNGText(out, "Display Format", props.get(Display_Format));

    // Finish up
    writePNGChunk(out, "IEND", 0, 0);

    // Clean up
    if(buffer)  delete[] buffer;
    if(compmem) delete[] compmem;
    out.close();

    // Re-enable old messages
    framebuffer.enableMessages(true);
    framebuffer.showMessage("Snapshot saved");
  }
  catch(const char *msg)
  {
    if(buffer)  delete[] buffer;
    if(compmem) delete[] compmem;
    out.close();
    framebuffer.showMessage(msg);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Snapshot::writePNGChunk(ofstream& out, const char* type,
                             uInt8* data, int size)
{
  // Stuff the length/type into the buffer
  uInt8 temp[8];
  temp[0] = size >> 24;
  temp[1] = size >> 16;
  temp[2] = size >> 8;
  temp[3] = size;
  temp[4] = type[0];
  temp[5] = type[1];
  temp[6] = type[2];
  temp[7] = type[3];

  // Write the header
  out.write((const char*)temp, 8);

  // Append the actual data
  uInt32 crc = crc32(0, temp + 4, 4);
  if(size > 0)
  {
    out.write((const char*)data, size);
    crc = crc32(crc, data, size);
  }

  // Write the CRC
  temp[0] = crc >> 24;
  temp[1] = crc >> 16;
  temp[2] = crc >> 8;
  temp[3] = crc;
  out.write((const char*)temp, 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Snapshot::writePNGText(ofstream& out, const string& key, const string& text)
{
  int length = key.length() + 1 + text.length() + 1;
  uInt8* data = new uInt8[length];

  strcpy((char*)data, key.c_str());
  strcpy((char*)data + key.length() + 1, text.c_str());

  writePNGChunk(out, "tEXt", data, length-1);

  delete[] data;
}
