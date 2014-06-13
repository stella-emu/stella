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
PNGLibrary::PNGLibrary(const FrameBuffer& fb)
  : myFB(fb)
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
void PNGLibrary::loadImage(const string& filename, FBSurface& surface)
{
  #define loadImageERROR(s) { err_message = s; goto done; }

  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  png_uint_32 iwidth, iheight;
  int bit_depth, color_type, interlace_type;
  const char* err_message = NULL;

  ifstream in(filename.c_str(), ios_base::binary);
  if(!in.is_open())
    loadImageERROR("No image found");

  // Create the PNG loading context structure
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
                 png_user_error, png_user_warn);
  if(png_ptr == NULL)
    loadImageERROR("Couldn't allocate memory for PNG file");

  // Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
    loadImageERROR("Couldn't create image information for PNG file");

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
    loadImageERROR("Greyscale PNG images not supported");
  }
  else if(color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_palette_to_rgb(png_ptr);
  }
  else if(color_type != PNG_COLOR_TYPE_RGB)
  {
    loadImageERROR("Unknown format in PNG image");
  }

  // Create/initialize storage area for the current image
  if(!allocateStorage(iwidth, iheight))
    loadImageERROR("Not enough memory to read PNG file");

  // The PNG read function expects an array of rows, not a single 1-D array
  for(uInt32 irow = 0, offset = 0; irow < ReadInfo.height; ++irow, offset += ReadInfo.pitch)
    ReadInfo.row_pointers[irow] = (png_bytep) (uInt8*)ReadInfo.buffer + offset;

  // Read the entire image in one go
  png_read_image(png_ptr, ReadInfo.row_pointers);

  // We're finished reading
  png_read_end(png_ptr, info_ptr);

  // Scale image to surface dimensions
  scaleImagetoSurface(surface);

  // Cleanup
done:
  if(png_ptr)
    png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : (png_infopp)0, (png_infopp)0);

  if(err_message)
    throw err_message;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImage(const string& filename, const Properties& props)
{
  #define saveImageERROR(s) { err_message = s; goto done; }

  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  const char* err_message = NULL;
  const GUI::Rect& imageR = myFB.imageRect();
  png_uint_32 width = imageR.width(), height = imageR.height();
  png_bytep image = new png_byte[width * height * 4];
  png_bytep row_pointers[height];

  ofstream out(filename.c_str(), ios_base::binary);
  if(!out.is_open())
    saveImageERROR("ERROR: Couldn't create snapshot file");
    
  // Create the PNG saving context structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                 png_user_error, png_user_warn);
  if(png_ptr == NULL)
    saveImageERROR("Couldn't allocate memory for PNG file");

  // Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
    saveImageERROR("Couldn't create image information for PNG file");

  // Set up the output control
  png_set_write_fn(png_ptr, &out, png_write_data, png_io_flush);

  // Write PNG header info
  png_set_IHDR(png_ptr, info_ptr, width, height, 8,
      PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);

  // Write comments
  writeComments(png_ptr, info_ptr, props);

  // Write the file header information.  REQUIRED
  png_write_info(png_ptr, info_ptr);

  // Pack pixels into bytes
  png_set_packing(png_ptr);

  // Swap location of alpha bytes from ARGB to RGBA
  png_set_swap_alpha(png_ptr);

  // Pack ARGB into RGB
  png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

  // Flip BGR pixels to RGB
  png_set_bgr(png_ptr);

  // Get framebuffer surface pixel data (we get ABGR format)
  myFB.readPixels(imageR, image, width*4);

  // Set up pointers into "image" byte array
  for(png_uint_32 k = 0; k < height; ++k)
    row_pointers[k] = image + k*width*4;

  // Write the entire image in one go
  png_write_image(png_ptr, row_pointers);

  // We're finished writing
  png_write_end(png_ptr, info_ptr);

  // Cleanup
done:
  if(png_ptr)
    png_destroy_write_struct(&png_ptr, &info_ptr);
  if(image)
    delete[] image;
  if(err_message)
    throw err_message;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImage(const string& filename, const TIA& tia,
                           const Properties& props)
{
  #define saveImageERROR(s) { err_message = s; goto done; }

  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  const char* err_message = NULL;
  png_uint_32 tiaw = tia.width(), width = tiaw*2, height = tia.height();
  png_bytep image = new png_byte[width * height * 3];
  png_bytep row_pointers[height];
  uInt8 r, g, b;
  uInt8* buf_ptr = image;

  ofstream out(filename.c_str(), ios_base::binary);
  if(!out.is_open())
    saveImageERROR("ERROR: Couldn't create snapshot file");
    
  // Create the PNG saving context structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                 png_user_error, png_user_warn);
  if(png_ptr == NULL)
    saveImageERROR("Couldn't allocate memory for PNG file");

  // Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
    saveImageERROR("Couldn't create image information for PNG file");

  // Set up the output control
  png_set_write_fn(png_ptr, &out, png_write_data, png_io_flush);

  // Write PNG header info
  png_set_IHDR(png_ptr, info_ptr, width, height, 8,
      PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);

  // Write comments
  writeComments(png_ptr, info_ptr, props);

  // Write the file header information.  REQUIRED
  png_write_info(png_ptr, info_ptr);

  // Fill the buffer with pixels from the tia, scaled 2x horizontally
  for(uInt32 y = 0; y < height; ++y)
  {
    for(uInt32 x = 0; x < tiaw; ++x)
    {
      uInt32 pixel = myFB.tiaSurface().pixel(y*tiaw+x);
      myFB.getRGB(pixel, &r, &g, &b);
      *buf_ptr++ = r;
      *buf_ptr++ = g;
      *buf_ptr++ = b;
      *buf_ptr++ = r;
      *buf_ptr++ = g;
      *buf_ptr++ = b;
    }
    // Set up pointers into "image" byte array
    buf_ptr = row_pointers[y] = image + y*width*3;
  }

  // Write the entire image in one go
  png_write_image(png_ptr, row_pointers);

  // We're finished writing
  png_write_end(png_ptr, info_ptr);

  // Cleanup
done:
  if(png_ptr)
    png_destroy_write_struct(&png_ptr, &info_ptr);
  if(image)
    delete[] image;
  if(err_message)
    throw err_message;
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
void PNGLibrary::scaleImagetoSurface(FBSurface& surface)
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
         szoom = surface.width()/320;

  uInt32 sw = ReadInfo.width / izoom * szoom,
         sh = ReadInfo.height / izoom * szoom;
  sw = BSPF_min(sw, surface.width());
  sh = BSPF_min(sh, surface.height());
  surface.setSrcSize(sw, sh);
  surface.setDstSize(sw, sh);

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
      uInt32 pixel = myFB.mapRGB(*i_ptr, *(i_ptr+1), *(i_ptr+2));
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
void PNGLibrary::writeComments(png_structp png_ptr, png_infop info_ptr,
                               const Properties& props)
{
  // Pre-processor voodoo to make the code shorter
  #define CONVERT_TO_PNGTEXT(_idx, _key, _text) \
      char key##_idx[] = _key;                  \
      char text##_idx[256];                     \
      strncpy(text##_idx, _text.c_str(), 255);  \
      text_ptr[_idx].key = key##_idx;           \
      text_ptr[_idx].text = text##_idx;         \
      text_ptr[_idx].compression = PNG_TEXT_COMPRESSION_NONE; \
      text_ptr[_idx].text_length = 0;

  ostringstream version;
  version << "Stella " << STELLA_VERSION << " (Build " << STELLA_BUILD << ") ["
          << BSPF_ARCH << "]";

  png_text text_ptr[4];
  CONVERT_TO_PNGTEXT(0, "Software", version.str());
  CONVERT_TO_PNGTEXT(1, "ROM Name", props.get(Cartridge_Name));
  CONVERT_TO_PNGTEXT(2, "ROM MD5", props.get(Cartridge_MD5));
  CONVERT_TO_PNGTEXT(3, "TV Effects", myFB.tiaSurface().effectsInfo());

  png_set_text(png_ptr, info_ptr, text_ptr, 4);
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
