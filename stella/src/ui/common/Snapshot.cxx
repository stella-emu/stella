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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Snapshot.cxx,v 1.6 2003-10-17 18:02:16 stephena Exp $
//============================================================================

#include <png.h>
#include <iostream>
#include <fstream>

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "Snapshot.hxx"


Snapshot::Snapshot()
{
  palette = (png_colorp) NULL;
}

Snapshot::~Snapshot()
{
  if(palette)
    delete[] palette;
}

void Snapshot::png_write_data(png_structp ctx, png_bytep area, png_size_t size)
{
  ofstream* out = (ofstream *) png_get_io_ptr(ctx);
  out->write((const char *)area, size);
}

void Snapshot::png_io_flush(png_structp ctx)
{
  ofstream* out = (ofstream *) png_get_io_ptr(ctx);
  out->flush();
}

void Snapshot::png_user_warn(png_structp ctx, png_const_charp str)
{
  cerr << "Snapshot:  libpng warning: " << str << endl;
}

void Snapshot::png_user_error(png_structp ctx, png_const_charp str)
{
  cerr << "Snapshot:  libpng error: " << str << endl;
}

/**
  This routine saves the current frame buffer to a 256 color PNG file,
  appropriately scaled by the amount specified in 'multiplier'.
*/
int Snapshot::savePNG(string filename, FrameBuffer& frameBuffer, uInt32 multiplier)
{
#if 0
// FIXME
  png_structp png_ptr = 0;
  png_infop info_ptr = 0;

  uInt8* pixels = frameBuffer.current();

  // PNG and window dimensions will be different because of scaling
  int picWidth  = frameBuffer.width() * 2 * multiplier;
  int picHeight = frameBuffer.height() * multiplier;
  int width  = frameBuffer.width();
  int height = frameBuffer.height();

  ofstream* out = new ofstream(filename.c_str());
  if(!out)
    return 0;

  // Create the palette if it hasn't previously been set
  if(!palette)
  {
    palette = (png_colorp) new png_color[256];
    if(!palette)
    {
      cerr << "Snapshot:  Couldn't allocate memory for PNG palette\n";
      out->close();

      return 0;
    }

    const uInt32* gamePalette = frameBuffer.palette();
    for(uInt32 i = 0; i < 256; ++i)
    {
      palette[i].red   = (uInt8) ((gamePalette[i] & 0x00ff0000) >> 16);
      palette[i].green = (uInt8) ((gamePalette[i] & 0x0000ff00) >> 8);
      palette[i].blue  = (uInt8) (gamePalette[i] & 0x000000ff);
    }
  }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_user_error, png_user_warn);
  if(png_ptr == NULL)
  {
    cerr << "Snapshot:  Couldn't allocate memory for PNG file\n";
    return 0;
  }

  // Allocate/initialize the image information data.  REQUIRED.
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
  {
    cerr << "Snapshot:  Couldn't create image information for PNG file\n";

    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    out->close();

    return 0;
  }

  png_set_write_fn(png_ptr, out, png_write_data, png_io_flush);

  png_set_IHDR(png_ptr, info_ptr, picWidth, picHeight, 8,
    PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE, PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  // Set the 8-bit palette
  png_set_PLTE(png_ptr, info_ptr, palette, 256);

  // Write the file header information.  REQUIRED
  png_write_info(png_ptr, info_ptr);

  // Pack pixels into bytes
  png_set_packing(png_ptr);

  // The width has to be scaled by 2 * multiplier.  Each pixel must be
  // present scaleX times.  Each scanline must be present scaleY times.
  int scaleX = 2 * multiplier;
  int scaleY = multiplier;  

  // Create a buffer to hold the new scanline.
  uInt8* newScanline = new uInt8[width * scaleX];
  uInt8* oldScanline;

  // Look at each original scanline
  for(int y = 0; y < height; y++)
  {
    // First construct a new scanline that is scaled
    oldScanline = (uInt8*) pixels + y*width;

    int px = 0;
    for(int x = 0; x < width; x++)
      for(int offx = 0; offx < scaleX; offx++)
        newScanline[px++] = oldScanline[x];

    // Now output the new scanline 'scaleY' times
    for(int offy = 0; offy < scaleY; offy++)
    {
      png_bytep row_pointer = (uInt8*) newScanline;
      png_write_row(png_ptr, row_pointer);
    }
  }

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

  if(newScanline)
    delete[] newScanline;

  out->close();
  delete out;
#endif
  return 1;
}
