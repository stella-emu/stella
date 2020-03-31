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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#if defined(PNG_SUPPORT)

#include <cmath>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Props.hxx"
#include "Settings.hxx"
#include "TIASurface.hxx"
#include "Version.hxx"
#include "PNGLibrary.hxx"
#include "Rect.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PNGLibrary::PNGLibrary(OSystem& osystem)
  : myOSystem(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::loadImage(const string& filename, FBSurface& surface)
{
  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;
  png_uint_32 iwidth, iheight;
  int bit_depth, color_type, interlace_type;

  auto loadImageERROR = [&](const char* s) {
    if(png_ptr)
      png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : nullptr, nullptr);
    if(s)
      throw runtime_error(s);
  };

  ifstream in(filename, std::ios_base::binary);
  if(!in.is_open())
    loadImageERROR("No snapshot found");

  // Create the PNG loading context structure
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                 png_user_error, png_user_warn);
  if(png_ptr == nullptr)
    loadImageERROR("Couldn't allocate memory for PNG file");

  // Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == nullptr)
    loadImageERROR("Couldn't create image information for PNG file");

  // Set up the input control
  png_set_read_fn(png_ptr, &in, png_read_data);

  // Read PNG header info
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &iwidth, &iheight, &bit_depth,
    &color_type, &interlace_type, nullptr, nullptr);

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
    ReadInfo.row_pointers[irow] = static_cast<png_bytep>(ReadInfo.buffer.data() + offset);

  // Read the entire image in one go
  png_read_image(png_ptr, ReadInfo.row_pointers.data());

  // We're finished reading
  png_read_end(png_ptr, info_ptr);

  // Load image into the surface, setting the correct dimensions
  loadImagetoSurface(surface);

  // Cleanup
  if(png_ptr)
    png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : nullptr, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImage(const string& filename, const VariantList& comments)
{
  ofstream out(filename, std::ios_base::binary);
  if(!out.is_open())
    throw runtime_error("ERROR: Couldn't create snapshot file");

  const FrameBuffer& fb = myOSystem.frameBuffer();

  const Common::Rect& rectUnscaled = fb.imageRect();
  const Common::Rect rect(
    Common::Point(fb.scaleX(rectUnscaled.x()), fb.scaleY(rectUnscaled.y())),
    fb.scaleX(rectUnscaled.w()), fb.scaleY(rectUnscaled.h())
  );

  png_uint_32 width = rect.w(), height = rect.h();

  // Get framebuffer pixel data (we get ABGR format)
  vector<png_byte> buffer(width * height * 4);
  fb.readPixels(buffer.data(), width*4, rect);

  // Set up pointers into "buffer" byte array
  vector<png_bytep> rows(height);
  for(png_uint_32 k = 0; k < height; ++k)
    rows[k] = static_cast<png_bytep>(buffer.data() + k*width*4);

  // And save the image
  saveImageToDisk(out, rows, width, height, comments);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImage(const string& filename, const FBSurface& surface,
                           const Common::Rect& rect, const VariantList& comments)
{
  ofstream out(filename, std::ios_base::binary);
  if(!out.is_open())
    throw runtime_error("ERROR: Couldn't create snapshot file");

  // Do we want the entire surface or just a section?
  png_uint_32 width = rect.w(), height = rect.h();
  if(rect.empty())
  {
    width = surface.width();
    height = surface.height();
  }

  // Get the surface pixel data (we get ABGR format)
  vector<png_byte> buffer(width * height * 4);
  surface.readPixels(buffer.data(), width, rect);

  // Set up pointers into "buffer" byte array
  vector<png_bytep> rows(height);
  for(png_uint_32 k = 0; k < height; ++k)
    rows[k] = static_cast<png_bytep>(buffer.data() + k*width*4);

  // And save the image
  saveImageToDisk(out, rows, width, height, comments);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImageToDisk(ofstream& out, const vector<png_bytep>& rows,
    png_uint_32 width, png_uint_32 height, const VariantList& comments)
{
  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;

  auto saveImageERROR = [&](const char* s) {
    if(png_ptr)
      png_destroy_write_struct(&png_ptr, &info_ptr);
    if(s)
      throw runtime_error(s);
  };

  // Create the PNG saving context structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr,
                 png_user_error, png_user_warn);
  if(png_ptr == nullptr)
    saveImageERROR("Couldn't allocate memory for PNG file");

  // Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == nullptr)
    saveImageERROR("Couldn't create image information for PNG file");

  // Set up the output control
  png_set_write_fn(png_ptr, &out, png_write_data, png_io_flush);

  // Write PNG header info
  png_set_IHDR(png_ptr, info_ptr, width, height, 8,
      PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);

  // Write comments
  writeComments(png_ptr, info_ptr, comments);

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

  // Write the entire image in one go
  png_write_image(png_ptr, const_cast<png_bytep*>(rows.data()));

  // We're finished writing
  png_write_end(png_ptr, info_ptr);

  // Cleanup
  if(png_ptr)
    png_destroy_write_struct(&png_ptr, &info_ptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::updateTime(uInt64 time)
{
  if(++mySnapCounter % mySnapInterval == 0)
    takeSnapshot(uInt32(time) >> 10);  // not quite milliseconds, but close enough
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::toggleContinuousSnapshots(bool perFrame)
{
  if(mySnapInterval == 0)
  {
    ostringstream buf;
    uInt32 interval = myOSystem.settings().getInt("ssinterval");
    if(perFrame)
    {
      buf << "Enabling snapshots every frame";
      interval = 1;
    }
    else
    {
      buf << "Enabling snapshots in " << interval << " second intervals";
      interval *= uInt32(myOSystem.frameRate());
    }
    myOSystem.frameBuffer().showMessage(buf.str());
    setContinuousSnapInterval(interval);
  }
  else
  {
    ostringstream buf;
    buf << "Disabling snapshots, generated "
      << (mySnapCounter / mySnapInterval)
      << " files";
    myOSystem.frameBuffer().showMessage(buf.str());
    setContinuousSnapInterval(0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::setContinuousSnapInterval(uInt32 interval)
{
  mySnapInterval = interval;
  mySnapCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::takeSnapshot(uInt32 number)
{
  if(!myOSystem.hasConsole())
    return;

  // Figure out the correct snapshot name
  string filename;
  bool showmessage = number == 0;
  string sspath = myOSystem.snapshotSaveDir() +
      (myOSystem.settings().getString("snapname") != "int" ?
          myOSystem.romFile().getNameWithExt("")
        : myOSystem.console().properties().get(PropType::Cart_Name));

  // Check whether we want multiple snapshots created
  if(number > 0)
  {
    ostringstream buf;
    buf << sspath << "_" << std::hex << std::setw(8) << std::setfill('0')
        << number << ".png";
    filename = buf.str();
  }
  else if(!myOSystem.settings().getBool("sssingle"))
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = sspath + ".png";
    FilesystemNode node(filename);
    if(node.exists())
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << sspath << "_" << i << ".png";
        FilesystemNode next(buf.str());
        if(!next.exists())
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = sspath + ".png";

  // Some text fields to add to the PNG snapshot
  VariantList comments;
  ostringstream version;
  version << "Stella " << STELLA_VERSION << " (Build " << STELLA_BUILD << ") ["
          << BSPF::ARCH << "]";
  VarList::push_back(comments, "Software", version.str());
  const string& name = (myOSystem.settings().getString("snapname") == "int")
      ? myOSystem.console().properties().get(PropType::Cart_Name)
      : myOSystem.romFile().getName();
  VarList::push_back(comments, "ROM Name", name);
  VarList::push_back(comments, "ROM MD5", myOSystem.console().properties().get(PropType::Cart_MD5));
  VarList::push_back(comments, "TV Effects", myOSystem.frameBuffer().tiaSurface().effectsInfo());

  // Now create a PNG snapshot
  if(myOSystem.settings().getBool("ss1x"))
  {
    string message = "Snapshot saved";
    try
    {
      Common::Rect rect;
      const FBSurface& surface = myOSystem.frameBuffer().tiaSurface().baseSurface(rect);
      myOSystem.png().saveImage(filename, surface, rect, comments);
    }
    catch(const runtime_error& e)
    {
      message = e.what();
    }
    if(showmessage)
      myOSystem.frameBuffer().showMessage(message);
  }
  else
  {
    // Make sure we have a 'clean' image, with no onscreen messages
    myOSystem.frameBuffer().enableMessages(false);
    myOSystem.frameBuffer().tiaSurface().renderForSnapshot();

    string message = "Snapshot saved";
    try
    {
      myOSystem.png().saveImage(filename, comments);
    }
    catch(const runtime_error& e)
    {
      message = e.what();
    }

    // Re-enable old messages
    myOSystem.frameBuffer().enableMessages(true);
    if(showmessage)
      myOSystem.frameBuffer().showMessage(message);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PNGLibrary::allocateStorage(png_uint_32 w, png_uint_32 h)
{
  // Create space for the entire image (3 bytes per pixel in RGB format)
  size_t req_buffer_size = w * h * 3;
  if(req_buffer_size > ReadInfo.buffer.size())
    ReadInfo.buffer.resize(req_buffer_size);

  size_t req_row_size = h;
  if(req_row_size > ReadInfo.row_pointers.size())
    ReadInfo.row_pointers.resize(req_row_size);

  ReadInfo.width  = w;
  ReadInfo.height = h;
  ReadInfo.pitch  = w * 3;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::loadImagetoSurface(FBSurface& surface)
{
  // First determine if we need to resize the surface
  uInt32 iw = ReadInfo.width, ih = ReadInfo.height;
  if(iw > surface.width() || ih > surface.height())
    surface.resize(iw, ih);

  // The source dimensions are set here; the destination dimensions are
  // set by whoever owns the surface
  surface.setSrcPos(0, 0);
  surface.setSrcSize(iw, ih);

  // Convert RGB triples into pixels and store in the surface
  uInt32 *s_buf, s_pitch;
  surface.basePtr(s_buf, s_pitch);
  const uInt8* i_buf = ReadInfo.buffer.data();
  const uInt32 i_pitch = ReadInfo.pitch;

  const FrameBuffer& fb = myOSystem.frameBuffer();
  for(uInt32 irow = 0; irow < ih; ++irow, i_buf += i_pitch, s_buf += s_pitch)
  {
    const uInt8* i_ptr = i_buf;
    uInt32* s_ptr = s_buf;
    for(uInt32 icol = 0; icol < ReadInfo.width; ++icol, i_ptr += 3)
      *s_ptr++ = fb.mapRGB(*i_ptr, *(i_ptr+1), *(i_ptr+2));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::writeComments(png_structp png_ptr, png_infop info_ptr,
                               const VariantList& comments)
{
  uInt32 numComments = uInt32(comments.size());
  if(numComments == 0)
    return;

  vector<png_text> text_ptr(numComments);
  for(uInt32 i = 0; i < numComments; ++i)
  {
    text_ptr[i].key = const_cast<char*>(comments[i].first.c_str());
    text_ptr[i].text = const_cast<char*>(comments[i].second.toCString());
    text_ptr[i].compression = PNG_TEXT_COMPRESSION_NONE;
    text_ptr[i].text_length = 0;
  }
  png_set_text(png_ptr, info_ptr, text_ptr.data(), numComments);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
  (static_cast<ifstream*>(png_get_io_ptr(ctx)))->read(
    reinterpret_cast<char *>(area), size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_write_data(png_structp ctx, png_bytep area, png_size_t size)
{
  (static_cast<ofstream*>(png_get_io_ptr(ctx)))->write(
    reinterpret_cast<const char *>(area), size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_io_flush(png_structp ctx)
{
  (static_cast<ofstream*>(png_get_io_ptr(ctx)))->flush();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_user_warn(png_structp ctx, png_const_charp str)
{
  throw runtime_error(string("PNGLibrary warning: ") + str);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::png_user_error(png_structp ctx, png_const_charp str)
{
  throw runtime_error(string("PNGLibrary error: ") + str);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PNGLibrary::ReadInfoType PNGLibrary::ReadInfo;

#endif  // PNG_SUPPORT
