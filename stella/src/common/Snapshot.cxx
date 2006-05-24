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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Snapshot.cxx,v 1.10 2006-05-24 17:37:32 stephena Exp $
//============================================================================

#ifdef SNAPSHOT_SUPPORT

#include <png.h>
#include <iostream>
#include <fstream>

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "Snapshot.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Snapshot::Snapshot(FrameBuffer& framebuffer)
  : myFrameBuffer(framebuffer)
{
  // Make sure we have a 'clean' image, with no onscreen messages
  myFrameBuffer.hideMessage();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Snapshot::~Snapshot()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Snapshot::png_write_data(png_structp ctx, png_bytep area, png_size_t size)
{
  ofstream* out = (ofstream *) png_get_io_ptr(ctx);
  out->write((const char *)area, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Snapshot::png_io_flush(png_structp ctx)
{
  ofstream* out = (ofstream *) png_get_io_ptr(ctx);
  out->flush();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Snapshot::png_user_warn(png_structp ctx, png_const_charp str)
{
  cerr << "Snapshot:  libpng warning: " << str << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Snapshot::png_user_error(png_structp ctx, png_const_charp str)
{
  cerr << "Snapshot:  libpng error: " << str << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Snapshot::savePNG(string filename)
{
  png_structp png_ptr = 0;
  png_infop info_ptr  = 0;

  // Get actual image dimensions. which are not always the same
  // as the framebuffer dimensions
  uInt32 width  = myFrameBuffer.imageWidth();
  uInt32 height = myFrameBuffer.imageHeight();

  ofstream* out = new ofstream(filename.c_str(), ios_base::binary);
  if(!out)
    return "Couldn't create snapshot file";

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_user_error, png_user_warn);
  if(png_ptr == NULL)
    return "Snapshot: Out of memory";

  // Allocate/initialize the image information data.  REQUIRED.
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
  {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    out->close();

    return "Snapshot: Error on create image info";
  }

  png_set_write_fn(png_ptr, out, png_write_data, png_io_flush);

  png_set_IHDR(png_ptr, info_ptr, width, height, 8,
    PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  // Write the file header information.  REQUIRED
  png_write_info(png_ptr, info_ptr);

  // Pack pixels into bytes
  png_set_packing(png_ptr);

  // Create space for one full scanline (3 bytes per pixel in RGB format)
  uInt8* data = new uInt8[width * 3];

  // Write a new scanline to the PNG file
  for(uInt32 row = 0; row < height; row++)
  {
    myFrameBuffer.scanline(row, data);
    png_write_row(png_ptr, (png_bytep) data);
  }

  // Cleanup
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
  delete[] data;

  out->close();
  delete out;

  return "Snapshot saved";
}

#endif  // SNAPSHOT_SUPPORT
