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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef IMAGE_SUPPORT

#include "OSystem.hxx"
#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "FSNode.hxx"
#include "Props.hxx"
#include "TIASurface.hxx"
#include "Version.hxx"
#include "PNGLibrary.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PNGLibrary::PNGLibrary(OSystem& osystem)
  : myOSystem{osystem}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::loadImage(string_view filename, FBSurface& surface,
                           VariantList& metaData)
{
  png_structp png_ptr{nullptr};
  png_infop info_ptr{nullptr};

  // RAII guard: ensures png read struct is always destroyed on scope exit
  struct PNGReadGuard {
    png_structp& png_ptr;
    png_infop&   info_ptr;
    PNGReadGuard(png_structp& sp, png_infop& info) : png_ptr{sp}, info_ptr{info} { }
    ~PNGReadGuard() {
      if(png_ptr)
        png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : nullptr, nullptr);
    }
    PNGReadGuard(const PNGReadGuard&) = delete;
    PNGReadGuard(PNGReadGuard&&) = delete;
    PNGReadGuard& operator=(const PNGReadGuard&) = delete;
    PNGReadGuard& operator=(PNGReadGuard&&) = delete;
  };

  // TODO: can we pass in FSNode directly in parameter?
  auto in = FSNode(filename).openIFStream(std::ios_base::binary);
  if(!in.is_open())
    throw std::runtime_error("No image found");

  const PNGReadGuard guard{png_ptr, info_ptr};

  // Create the PNG loading context structure
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                   png_user_error, png_user_warn);
  if(png_ptr == nullptr)
    throw std::runtime_error("Couldn't allocate memory for PNG image");

  // Allocate/initialize the memory for image information.  REQUIRED.
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == nullptr)
    throw std::runtime_error("Couldn't create image information for PNG image");

  // Set up the input control
  png_set_read_fn(png_ptr, &in, png_read_data);

  // Read PNG header info
  png_uint_32 width{}, height{};
  int color_type{}, bit_depth{};
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               nullptr, nullptr, nullptr);

  // Normalize format
  if(bit_depth == 16)
    png_set_strip_16(png_ptr);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  if(!(color_type & PNG_COLOR_MASK_ALPHA))
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_set_bgr(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  // First determine if we need to resize the surface
  if(width > surface.width() || height > surface.height())
    surface.resize(width, height);

  // The source dimensions are set here; the destination dimensions are
  // set by whoever owns the surface
  surface.setSrcPos(0, 0);
  surface.setSrcSize(width, height);
  myRowPointers.resize(height);

  // Format is SDL_PIXELFORMAT_ARGB8888
  uInt32* base{};
  uInt32 pitch{};
  surface.basePtr(base, pitch);

  // Set row pointers into surface buffer
  const uInt32 rowStride = pitch * sizeof(uInt32);
  png_bytep* dst = myRowPointers.data();  // NOLINT(misc-const-correctness)
  auto*      row = reinterpret_cast<png_bytep>(base);
  for(uInt32 y = 0; y < height; ++y, row += rowStride)
    *dst++ = row;

  // And read directly into the surface buffer
  png_read_image(png_ptr, myRowPointers.data());

  // We're finished reading
  png_read_end(png_ptr, info_ptr);

  // Read the meta data we got
  readMetaData(png_ptr, info_ptr, metaData);

  // Handle big-endian architectures; maybe SDL can do this directly?
  // Big-endian: byte-swap in-place. Callers must treat the pixel buffer
  // as consumed after this call — the data is not preserved.
  if constexpr(std::endian::native == std::endian::big)
  {
    // Swap ARGB32 -> ABGR32 on big-endian CPUs
    for(uInt32 y = 0; y < height; ++y)
    {
      auto* row = reinterpret_cast<uInt32*>(myRowPointers[y]);
      for(uInt32 x = 0; x < width; ++x)
        row[x] = byteswap<uInt32>(row[x]);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImage(string_view filename, const VariantList& metaData)
{
#if 0  // FIXME: disabled for now, other parts of the codebase need work
  // FIXME: leave this for now saveImage(filename, , Common::Rect{}, metaData);

  const FrameBuffer& fb = myOSystem.frameBuffer();

  const Common::Rect& rectUnscaled = fb.imageRect();
  const Common::Rect rect(
    Common::Point(fb.scaleX(rectUnscaled.x()), fb.scaleY(rectUnscaled.y())),
    fb.scaleX(rectUnscaled.w()), fb.scaleY(rectUnscaled.h())
  );

  const size_t width = rect.w(), height = rect.h();

  // Get framebuffer pixel data (we get ABGR format)
  const size_t bufferSize = width * height * 4;
  if(bufferSize > myPNGReadBuffer.size())
    myPNGReadBuffer.resize(bufferSize);

  fb.readPixels(myPNGReadBuffer.data(), width * 4, rect);

  // Create a span of row pointers
  std::vector<png_bytep> rowPointers(height);
  for(size_t y = 0; y < height; ++y)
    rowPointers[y] = myPNGReadBuffer.data() + y * width * 4;

  std::ofstream out(string{filename}, std::ios_base::binary);
  if(!out.is_open())
    throw std::runtime_error("ERROR: Couldn't create snapshot file");

  saveImageToDisk(out, std::span<png_bytep>(rowPointers), width, height, metaData);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImage(const FSNode& filename, const FBSurface& surface,
                           const Common::Rect& rect, const VariantList& metaData)
{
#if 0
  size_t width = rect.w(), height = rect.h();
  if(rect.empty())
  {
    width  = surface.width();
    height = surface.height();
  }

  const size_t bufferSize = width * height * 4;
  if(bufferSize > myPNGReadBuffer.size())
    myPNGReadBuffer.resize(bufferSize);

  surface.readPixels(myPNGReadBuffer.data(), static_cast<uInt32>(width), rect);

  // Create a span of row pointers
  std::vector<png_bytep> rowPointers(height);
  for(size_t y = 0; y < height; ++y)
    rowPointers[y] = myPNGReadBuffer.data() + y * width * 4;

  std::ofstream out(string{filename}, std::ios_base::binary);
  if(!out.is_open())
    throw std::runtime_error("ERROR: Couldn't create snapshot file");

  saveImageToDisk(out, std::span<png_bytep>(rowPointers), width, height, metaData);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImageToDisk(std::ofstream& out,
                                 std::span<png_bytep> rows,
                                 size_t width, size_t height,
                                 const VariantList& metaData)
{
  png_structp png_ptr{nullptr};
  png_infop info_ptr{nullptr};

  // RAII guard: ensures png write struct is always destroyed on scope exit
  struct PNGWriteGuard {
    png_structp& png_ptr;
    png_infop&   info_ptr;
    PNGWriteGuard(png_structp& sp, png_infop& info) : png_ptr{sp}, info_ptr{info} { }
    ~PNGWriteGuard() {
      if(png_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
    }
    PNGWriteGuard(const PNGWriteGuard&) = delete;
    PNGWriteGuard(PNGWriteGuard&&) = delete;
    PNGWriteGuard& operator=(const PNGWriteGuard&) = delete;
    PNGWriteGuard& operator=(PNGWriteGuard&&) = delete;
  };

  const PNGWriteGuard guard{png_ptr, info_ptr};

  // Create the PNG saving context structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                    png_user_error, png_user_warn);
  if(png_ptr == nullptr)
    throw std::runtime_error("Couldn't allocate memory for PNG write struct");

  // Allocate/initialize the memory for image information.  REQUIRED.
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == nullptr)
    throw std::runtime_error("Couldn't create PNG info struct");

  // Set up the output control
  png_set_write_fn(png_ptr, &out, png_write_data, png_io_flush);

  // Write PNG header info
  png_set_IHDR(
    png_ptr, info_ptr,
    static_cast<png_uint_32>(width),
    static_cast<png_uint_32>(height),
    8,                  // bit depth
    PNG_COLOR_TYPE_RGB, // no alpha in output
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );

  // Metadata
  writeMetaData(png_ptr, info_ptr, metaData);

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

  // Handle big-endian architectures; maybe SDL can do this directly?
  // Big-endian: byte-swap in-place. Callers must treat the pixel buffer
  // as consumed after this call — the data is not preserved.
  if constexpr(std::endian::native == std::endian::big)
  {
    // Swap ARGB32 -> ABGR32 on big-endian CPUs
    for(size_t y = 0; y < height; ++y)
    {
      auto* row = reinterpret_cast<uInt32*>(rows[y]);
      for(size_t x = 0; x < width; ++x)
        row[x] = byteswap<uInt32>(row[x]);
    }
  }

  // Write the entire image in one go
  png_write_image(png_ptr, const_cast<png_bytep*>(rows.data()));

  // We're finished writing
  png_write_end(png_ptr, info_ptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::updateTime(uInt64 time)
{
  if(mySnapInterval > 0 && ++mySnapCounter % mySnapInterval == 0)
    takeSnapshot(static_cast<uInt32>(time) >> 10);  // not quite milliseconds, but close enough
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::toggleContinuousSnapshots(bool perFrame)
{
  if(mySnapInterval == 0)
  {
    string msg;
    uInt32 interval = myOSystem.settings().getInt("ssinterval");
    if(perFrame)
    {
      msg = "Enabling snapshots every frame";
      interval = 1;
    }
    else
    {
      msg = std::format("Enabling snapshots in {} second intervals", interval);
      interval *= static_cast<uInt32>(myOSystem.frameRate());
    }
    myOSystem.frameBuffer().showTextMessage(msg);
    setContinuousSnapInterval(interval);
  }
  else
  {
    auto msg = std::format("Disabling snapshots, generated {} files",
                           mySnapCounter / mySnapInterval);
    myOSystem.frameBuffer().showTextMessage(msg);
    setContinuousSnapInterval(0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::setContinuousSnapInterval(uInt32 interval)
{
  mySnapInterval = interval;
  mySnapCounter  = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::takeSnapshot(uInt32 number)
{
  if(!myOSystem.hasConsole())
    return;

  // Figure out the correct snapshot name
  string filename;
  const string sspath = myOSystem.snapshotSaveDir().getPath() +
      (myOSystem.settings().getString("snapname") != "int"
        ? myOSystem.romFile().getNameWithExt("")
        : myOSystem.console().properties().get(PropType::Cart_Name));

  // Check whether we want multiple snapshots created
  if(number > 0)
  {
    filename = std::format("{}_{:0>8X}.png", sspath, number);
  }
  else if(!myOSystem.settings().getBool("sssingle"))
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = sspath + ".png";
    const FSNode node(filename);
    if(node.exists())
    {
      for(uInt32 i = 1; ; ++i)
      {
        filename = std::format("{}_{}.png", sspath, i);
        const FSNode next(filename);
        if(!next.exists())
          break;
      }
    }
  }
  else
    filename = sspath + ".png";

  // Some text fields to add to the PNG snapshot
  VariantList metaData;
  VarList::push_back(metaData, "Title", "Snapshot");
  VarList::push_back(metaData, "Software",
    std::format("{} (Build {}) [{}]", STELLA_FULL_TITLE, STELLA_BUILD, BSPF::ARCH));
  const string& name = (myOSystem.settings().getString("snapname") == "int")
      ? myOSystem.console().properties().get(PropType::Cart_Name)
      : myOSystem.romFile().getName();
  VarList::push_back(metaData, "ROM Name", name);
  VarList::push_back(metaData, "ROM MD5", myOSystem.console().properties().get(PropType::Cart_MD5));
  VarList::push_back(metaData, "TV Effects", myOSystem.frameBuffer().tiaSurface().effectsInfo());

  // Now create a PNG snapshot
  string message = "Snapshot saved";
  if(myOSystem.settings().getBool("ss1x"))
  {
    try
    {
      Common::Rect rect;
      const FBSurface& surface =
        myOSystem.frameBuffer().tiaSurface().baseSurface(rect);
      const FSNode pngfile(filename);
      saveImage(pngfile, surface, rect, metaData);
    }
    catch(const std::runtime_error& e)
    {
      message = e.what();
    }
  }
  else
  {
    // Make sure we have a 'clean' image, with no onscreen messages
    myOSystem.frameBuffer().enableMessages(false);
    myOSystem.frameBuffer().tiaSurface().renderForSnapshot();

    try
    {
      saveImage(filename, metaData);
    }
    catch(const std::runtime_error& e)
    {
      message = e.what();
    }

    // Re-enable old messages
    myOSystem.frameBuffer().enableMessages(true);
  }
  myOSystem.frameBuffer().showTextMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::writeMetaData(png_structp png_ptr, png_infop info_ptr,
                               const VariantList& metaData)
{
  const auto numMetaData = metaData.size();
  if(numMetaData == 0)
    return;

  std::array<png_text, 16> text_ptr{};  // Assume 16 or fewer metadata entries
  assert(numMetaData <= text_ptr.size());
  for(size_t i = 0; i < numMetaData; ++i)
  {
    text_ptr[i].key         = const_cast<char*>(metaData[i].first.c_str());
    text_ptr[i].text        = const_cast<char*>(metaData[i].second.toCString());
    text_ptr[i].compression = PNG_TEXT_COMPRESSION_NONE;
    text_ptr[i].text_length = 0;
  }
  png_set_text(png_ptr, info_ptr, text_ptr.data(), static_cast<int>(numMetaData));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::readMetaData(png_structp png_ptr, png_infop info_ptr,
                              VariantList& metaData)
{
  png_textp text_ptr{nullptr};
  int numMetaData{0};

  png_get_text(png_ptr, info_ptr, &text_ptr, &numMetaData);

  metaData.clear();
  for(int i = 0; i < numMetaData; ++i)
    VarList::push_back(metaData, text_ptr[i].key, text_ptr[i].text);
}

#endif  // IMAGE_SUPPORT
