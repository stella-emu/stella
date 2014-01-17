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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <zlib.h>
#include <fstream>
#include <cstring>
#include <sstream>
#include <cmath>

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "Props.hxx"
#include "TIA.hxx"
#include "Version.hxx"
#include "PNGLibrary.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PNGLibrary::PNGLibrary()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PNGLibrary::~PNGLibrary()
{
  delete[] ReadInfo.buffer;
  delete[] ReadInfo.line;
  delete[] ReadInfo.row_pointers;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PNGLibrary::loadImage(const string& filename,
                           const FrameBuffer& fb, FBSurface& surface)
{
  #define readImageERROR(s) { err_message = s; goto done; }

  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  png_uint_32 iwidth, iheight;
  int bit_depth, color_type, interlace_type;
  const char* err_message = NULL;

  ifstream in(filename.c_str(), ios_base::binary);
  if(!in.is_open())
    readImageERROR("No image found");

  // Create the PNG loading context structure
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
                 png_user_error, png_user_warn);
  if(png_ptr == NULL)
    readImageERROR("Couldn't allocate memory for PNG file");

  // Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
    readImageERROR("Couldn't create image information for PNG file");

  // Set up the input control
  png_set_read_fn(png_ptr, &in, png_read_data);

  // Read PNG header info
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &iwidth, &iheight, &bit_depth,
    &color_type, &interlace_type, NULL, NULL);

  // Tell libpng to strip 16 bit/color files down to 8 bits/color
  png_set_strip_16(png_ptr);

  // Extract multiple pixels with bit depths of 1, 2, and 4 from a single
  // byte into separate bytes (useful for paletted and grayscale images).
  png_set_packing(png_ptr);

  // Only normal RBG(A) images are supported (without the alpha channel)
  if(color_type == PNG_COLOR_TYPE_RGBA)
  {
    png_set_strip_alpha(png_ptr);
  }
  else if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
  {
    readImageERROR("Greyscale PNG images not supported");
  }
  else if(color_type == PNG_COLOR_TYPE_PALETTE)
  {
    readImageERROR("Paletted PNG images not supported");
  }
  else if(color_type != PNG_COLOR_TYPE_RGB)
  {
    readImageERROR("Unknown format in PNG image");
  }

  // Create/initialize storage area for the current image
  if(!allocateStorage(iwidth, iheight))
    readImageERROR("Not enough memory to read PNG file");

  // The PNG read function expects an array of rows, not a single 1-D array
  for(uInt32 irow = 0, offset = 0; irow < ReadInfo.height; ++irow, offset += ReadInfo.pitch)
    ReadInfo.row_pointers[irow] = (png_bytep) (uInt8*)ReadInfo.buffer + offset;

  // Read the entire image in one go
  png_read_image(png_ptr, ReadInfo.row_pointers);

  // We're finished reading
  png_read_end(png_ptr, info_ptr);

  // Scale image to surface dimensions
  scaleImagetoSurface(fb, surface);

  // Cleanup
done:
  if(png_ptr)
    png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : (png_infopp)0, (png_infopp)0);

  if(err_message)
    throw err_message;
  else
    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PNGLibrary::saveImage(const string& filename,
                             const FrameBuffer& framebuffer,
                             const Properties& props)
{
  ofstream out(filename.c_str(), ios_base::binary);
  if(!out.is_open())
    return "ERROR: Couldn't create snapshot file";

  // Get actual image dimensions. which are not always the same
  // as the framebuffer dimensions
  const GUI::Rect& image = framebuffer.imageRect();
  uInt32 width = image.width(), height = image.height(),
         pitch = width * 3;
  uInt8* buffer = new uInt8[(pitch + 1) * height];

  // Fill the buffer with scanline data
  uInt8* buf_ptr = buffer;
  for(uInt32 row = 0; row < height; row++)
  {
    *buf_ptr++ = 0;                      // first byte of row is filter type
    framebuffer.scanline(row, buf_ptr);  // get another scanline
    buf_ptr += pitch;                    // add pitch
  }

  return saveBufferToPNG(out, buffer, width, height,
                         props, framebuffer.effectsInfo());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PNGLibrary::saveImage(const string& filename,
                             const FrameBuffer& framebuffer, const TIA& tia,
                             const Properties& props)
{
  ofstream out(filename.c_str(), ios_base::binary);
  if(!out.is_open())
    return "ERROR: Couldn't create snapshot file";

  uInt32 width = tia.width(), height = tia.height();
  uInt8* buffer = new uInt8[(width*3*2 + 1) * height];

  // Fill the buffer with pixels from the mediasrc
  uInt8 r, g, b;
  uInt8* buf_ptr = buffer;
  for(uInt32 y = 0; y < height; ++y)
  {
    *buf_ptr++ = 0;   // first byte of row is filter type
    for(uInt32 x = 0; x < width; ++x)
    {
      uInt32 pixel = framebuffer.tiaPixel(y*width+x);
      framebuffer.getRGB(pixel, &r, &g, &b);
      *buf_ptr++ = r;
      *buf_ptr++ = g;
      *buf_ptr++ = b;
      *buf_ptr++ = r;
      *buf_ptr++ = g;
      *buf_ptr++ = b;
    }
  }

  return saveBufferToPNG(out, buffer, width << 1, height,
                         props, framebuffer.effectsInfo());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PNGLibrary::saveBufferToPNG(ofstream& out, uInt8* buffer,
                                   uInt32 width, uInt32 height,
                                   const Properties& props,
                                   const string& effectsInfo)
{
  uInt8* compmem = (uInt8*) NULL;

  try
  {
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

    // Compress the data with zlib
    uLongf compmemsize = (uLongf)((height * (width + 1) * 3 * 1.001 + 1) + 12);
    compmem = new uInt8[compmemsize];
    if(compmem == NULL ||
       (compress(compmem, &compmemsize, buffer, height * (width * 3 + 1)) != Z_OK))
      throw "ERROR: Couldn't compress PNG";

    // Write the compressed framebuffer data
    writePNGChunk(out, "IDAT", compmem, compmemsize);

    // Add some info about this snapshot
    ostringstream text;
    text << "Stella " << STELLA_VERSION << " (Build " << STELLA_BUILD << ") ["
         << BSPF_ARCH << "]";

    writePNGText(out, "Software", text.str());
    writePNGText(out, "ROM Name", props.get(Cartridge_Name));
    writePNGText(out, "ROM MD5", props.get(Cartridge_MD5));
    writePNGText(out, "TV Effects", effectsInfo);

    // Finish up
    writePNGChunk(out, "IEND", 0, 0);

    // Clean up
    if(buffer)  delete[] buffer;
    if(compmem) delete[] compmem;
    out.close();

    return "Snapshot saved";
  }
  catch(const char* msg)
  {
    if(buffer)  delete[] buffer;
    if(compmem) delete[] compmem;
    out.close();
    return msg;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PNGLibrary::allocateStorage(png_uint_32 w, png_uint_32 h)
{
  // Create space for the entire image (3 bytes per pixel in RGB format)
  uInt32 req_buffer_size = w * h * 3;
  if(req_buffer_size > ReadInfo.buffer_size)
  {
    delete[] ReadInfo.buffer;
    ReadInfo.buffer = new uInt8[req_buffer_size];
    if(ReadInfo.buffer == NULL)
      return false;

    ReadInfo.buffer_size = req_buffer_size;
  }
  uInt32 req_line_size = w * 3;
  if(req_line_size > ReadInfo.line_size)
  {
    delete[] ReadInfo.line;
    ReadInfo.line = new uInt32[req_line_size];
    if(ReadInfo.line == NULL)
      return false;

    ReadInfo.line_size = req_line_size;
  }
  uInt32 req_row_size = h;
  if(req_row_size > ReadInfo.row_size)
  {
    delete[] ReadInfo.row_pointers;
    ReadInfo.row_pointers = new png_bytep[req_row_size];
    if(ReadInfo.row_pointers == NULL)
      return false;

    ReadInfo.row_size = req_row_size;
  }

  ReadInfo.width  = w;
  ReadInfo.height = h;
  ReadInfo.pitch  = w * 3;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::scaleImagetoSurface(const FrameBuffer& fb, FBSurface& surface)
{
  // Figure out the original zoom level of the snapshot
  // All snapshots generated by Stella are at most some multiple of 320
  // pixels wide
  // The only complication is when the aspect ratio is changed, the width
  // can range from 256 (80%) to 320 (100%)
  // The following calculation will work up to approx. 16x zoom level,
  // but since Stella only generates snapshots at up to 10x, we should
  // be fine for a while ...
  uInt32 izoom = uInt32(ceil(ReadInfo.width/320.0)),
         szoom = surface.getWidth()/320;

  uInt32 sw = ReadInfo.width / izoom * szoom,
         sh = ReadInfo.height / izoom * szoom;
  sw = BSPF_min(sw, surface.getWidth());
  sh = BSPF_min(sh, surface.getHeight());
  surface.setWidth(sw);
  surface.setHeight(sh);

  // Decompress the image, and scale it correctly
  uInt32 buf_offset = ReadInfo.pitch * izoom;
  uInt32 i_offset = 3 * izoom;

  // We can only scan at most the height of the image to the constraints of
  // the surface height (some multiple of 256)
  uInt32 iheight = BSPF_min((uInt32)ReadInfo.height, izoom * 256);

  // Grab each non-duplicate row of data from the image
  uInt8* buffer = ReadInfo.buffer;
  for(uInt32 irow = 0, srow = 0; irow < iheight; irow += izoom, buffer += buf_offset)
  {
    // Scale the image data into the temporary line buffer
    uInt8*  i_ptr = buffer;
    uInt32* l_ptr = ReadInfo.line;
    for(uInt32 icol = 0; icol < ReadInfo.width; icol += izoom, i_ptr += i_offset)
    {
      uInt32 pixel = fb.mapRGB(*i_ptr, *(i_ptr+1), *(i_ptr+2));
      uInt32 xstride = szoom;
      while(xstride--)
        *l_ptr++ = pixel;
    }

    // Then fill the surface with those bytes
    uInt32 ystride = szoom;
    while(ystride--)
      surface.drawPixels(ReadInfo.line, 0, srow++, sw);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::writePNGChunk(ofstream& out, const char* type,
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
void PNGLibrary::writePNGText(ofstream& out, const string& key, const string& text)
{
  int length = key.length() + 1 + text.length() + 1;
  uInt8* data = new uInt8[length];

  strcpy((char*)data, key.c_str());
  strcpy((char*)data + key.length() + 1, text.c_str());

  writePNGChunk(out, "tEXt", data, length-1);
  delete[] data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
  ifstream* stream = (ifstream *) png_get_io_ptr(ctx);
  stream->read((char *)area, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_write_data(png_structp ctx, png_bytep area, png_size_t size)
{
  ofstream* stream = (ofstream *) png_get_io_ptr(ctx);
  stream->write((const char *)area, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_io_flush(png_structp ctx)
{
  ofstream* stream = (ofstream *) png_get_io_ptr(ctx);
  stream->flush();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_user_warn(png_structp ctx, png_const_charp str)
{
  const string& msg = string("PNGLibrary warning: ") + str;
  throw msg.c_str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_user_error(png_structp ctx, png_const_charp str)
{
  const string& msg = string("PNGLibrary error: ") + str;
  throw msg.c_str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PNGLibrary::ReadInfoType PNGLibrary::ReadInfo = {
  NULL, NULL, 0, 0, 0, NULL, 0, 0, 0
};
