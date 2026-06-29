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

#include <bit>
#include <fstream>

#include "OSystem.hxx"
#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "FSNode.hxx"
#include "Props.hxx"
#include "TIASurface.hxx"
#include "Version.hxx"
#include "PNGLibrary.hxx"

namespace {
  template<typename Fn>
  struct ScopeExit {
    explicit ScopeExit(Fn f) : myFn{std::move(f)} {}
    ~ScopeExit() { myFn(); }
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit(ScopeExit&&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ScopeExit& operator=(ScopeExit&&) = delete;
  private:
    Fn myFn;
  };

  void png_read_data(png_structp ctx, png_bytep area, png_size_t size) {
    (static_cast<std::ifstream*>(png_get_io_ptr(ctx)))->read(
                                 reinterpret_cast<char*>(area), size);
  }
  void png_write_data(png_structp ctx, png_bytep area, png_size_t size) {
    (static_cast<std::ofstream*>(png_get_io_ptr(ctx)))->write(
                                 reinterpret_cast<const char*>(area), size);
  }
  void png_io_flush(png_structp ctx) {
    (static_cast<std::ofstream*>(png_get_io_ptr(ctx)))->flush();
  }
  void png_user_warn(png_structp, png_const_charp str) {
    // Optional: log, but DO NOT throw
    cerr << "libpng warning: " << str << '\n';
  }
  [[noreturn]] void png_user_error(png_structp, png_const_charp msg) {
    throw std::runtime_error(msg);
  }

  // Filename-safe local timestamp (YYYY-MM-DD_HH-MM-SS), used to keep
  // successive snapshots unique without overwriting earlier ones
  string snapTimestamp()
  {
    const std::tm t = BSPF::localTime();
    return std::format("{:04}-{:02}-{:02}_{:02}-{:02}-{:02}",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec);
  }
}  // namespace

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

  auto in = FSNode(filename).openIFStream(std::ios_base::binary);
  if(!in.is_open())
    throw std::runtime_error("No image found");

  const ScopeExit pngGuard{[&]() {
    if(png_ptr)
      png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : nullptr, nullptr);
  }};

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

  // The dimensions come straight from the (untrusted) PNG header; libpng only
  // caps them at 1,000,000 each, which would drive a multi-gigabyte surface
  // allocation below.  Reject implausibly large images up front.
  constexpr png_uint_32 MAX_DIMENSION = 16384;
  if(width == 0 || height == 0 || width > MAX_DIMENSION || height > MAX_DIMENSION)
    throw std::runtime_error("PNG image dimensions out of range");

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

  uInt32* base{nullptr};
  uInt32  pitch{0};
  surface.basePtr(base, pitch);

  const size_t rowStride = pitch * sizeof(uInt32);

  // And read directly into the surface buffer
  auto* row = reinterpret_cast<png_bytep>(base);
  for(size_t y = 0; y < height; ++y, row += rowStride)
    png_read_row(png_ptr, reinterpret_cast<png_bytep>(row), nullptr);

  // We're finished reading
  png_read_end(png_ptr, info_ptr);

  // Read the meta data we got
  readMetaData(png_ptr, info_ptr, metaData);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::saveImage(string_view filename, const FBSurface& surface,
                           const Common::Rect& rect, const VariantList& metaData)
{
  const Common::Rect srcRect = rect.empty()
    ? Common::Rect{0, 0, surface.width(), surface.height()}
    : rect;

  const size_t width  = srcRect.w();
  const size_t height = srcRect.h();

  png_structp png_ptr{nullptr};
  png_infop info_ptr{nullptr};

  const ScopeExit pngGuard{[&]() {
    if(png_ptr)
      png_destroy_write_struct(&png_ptr, &info_ptr);
  }};

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
  auto out = FSNode(filename).openOFStream(std::ios_base::binary);
  if(!out.is_open())
    throw std::runtime_error("ERROR: Couldn't create snapshot file");
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

  // Write the file header information
  png_write_info(png_ptr, info_ptr);

  // Pack pixels into bytes
  png_set_packing(png_ptr);

  // Pack ARGB into RGB
  png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

  // Little-endian architectures needs bytes swapped; big-endian seems to
  // have the data in the correct order already
  // TODO: test this on a real big-endian machine
  if constexpr(std::endian::native == std::endian::little)
  {
    // Flip BGR pixels to RGB
    png_set_bgr(png_ptr);
  }

  // TODO: Experiment with compression levels to speed up saving
  //       For snapshots (interactive, possibly frequent): 1
  //       For continuous snapshots (ssinterval): 0
  //       For user-triggered (save snapshot): 3
//   png_set_compression_level(png_ptr, 0);
//   png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

  // Direct access to surface memory
  uInt32* base{nullptr};
  uInt32  pitch{0};
  surface.basePtr(base, pitch);

  const size_t rowStride = pitch * sizeof(uInt32);
  auto* row = reinterpret_cast<png_bytep>(base)
            + srcRect.y() * rowStride
            + srcRect.x() * sizeof(uInt32);

  for(size_t y = 0; y < height; ++y, row += rowStride)
    png_write_row(png_ptr, reinterpret_cast<png_bytep>(row));

  // We're finished writing
  png_write_end(png_ptr, info_ptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::updateTime(uInt64 time)
{
  if(mySnapInterval > 0 && (++mySnapCounter) % mySnapInterval == 0)
    takeSnapshot(static_cast<uInt32>(time >> 10));  // not quite milliseconds, but close enough
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::toggleContinuousSnapshots(bool perFrame)
{
  if(mySnapInterval == 0)
  {
    string msg;
    uInt32 interval{1};
    if(perFrame)
    {
      msg = "Enabling snapshots every frame";
    }
    else
    {
      interval = myOSystem.settings().getInt("ssinterval");
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
  myCropValid    = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect PNGLibrary::croppedRect(const FBSurface& surface,
                                     const Common::Rect& rect, uInt32 number)
{
  // For continuous snapshots, reuse the crop computed on the first frame so
  // that every frame in the sequence is saved with identical dimensions
  if(number > 0 && myCropValid)
    return myCropRect;

  uInt32* base{nullptr};
  uInt32  pitch{0};
  surface.basePtr(base, pitch);

  // A pixel is 'black' once its color channels are all zero (the high
  // byte is alpha/filler and is ignored)
  const auto isBlack = [](uInt32 pixel) {
    return (pixel & 0x00FFFFFF) == 0;
  };
  const auto rowIsBlack = [&](uInt32 y, uInt32 x0, uInt32 x1) {
    const uInt32* row = base + static_cast<size_t>(y) * pitch;
    for(uInt32 x = x0; x < x1; ++x)
      if(!isBlack(row[x]))
        return false;
    return true;
  };
  const auto colIsBlack = [&](uInt32 x, uInt32 y0, uInt32 y1) {
    for(uInt32 y = y0; y < y1; ++y)
      if(!isBlack(base[static_cast<size_t>(y) * pitch + x]))
        return false;
    return true;
  };

  // Trim fully-black border rows/columns; if the entire region is black the
  // original rect is kept, so a snapshot is never reduced to nothing
  uInt32 top  = rect.y(), bottom = rect.y() + rect.h();
  uInt32 left = rect.x(), right  = rect.x() + rect.w();

  while(top < bottom && rowIsBlack(top, left, right))
    ++top;
  while(bottom > top && rowIsBlack(bottom - 1, left, right))
    --bottom;

  Common::Rect cropped = rect;
  if(top < bottom)
  {
    // Trim columns over the already-trimmed rows only
    while(left < right && colIsBlack(left, top, bottom))
      ++left;
    while(right > left && colIsBlack(right - 1, top, bottom))
      --right;
    cropped = Common::Rect{left, top, right, bottom};
  }

  if(number > 0)
  {
    myCropRect  = cropped;
    myCropValid = true;
  }
  return cropped;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PNGLibrary::takeSnapshot(uInt32 number)
{
  if(!myOSystem.hasConsole())
    return;

  // Figure out the correct snapshot name
  const bool useIntName = myOSystem.settings().getString("snapname") == "int";
  string filename;
  const string sspath = std::format("{}{}",
      myOSystem.snapshotSaveDir().getPath(),
      !useIntName
        ? myOSystem.romFile().getBaseName()
        : myOSystem.console().properties().get(PropType::Cart_Name));

  // Check whether we want multiple snapshots created
  if(number > 0)
  {
    filename = std::format("{}_{:0>8X}.png", sspath, number);
  }
  else if(!myOSystem.settings().getBool("sssingle"))
  {
    // Use the plain ROM name; if that snapshot already exists, append a
    // timestamp so we never overwrite an earlier one
    filename = sspath + ".png";
    if(FSNode(filename).exists())
      filename = std::format("{}_{}.png", sspath, snapTimestamp());
  }
  else
    filename = sspath + ".png";

  // Some text fields to add to the PNG snapshot
  VariantList metaData;
  VarList::push_back(metaData, "Title", "Snapshot");
  VarList::push_back(metaData, "Software",
    std::format("{} (Build {}) [{}]", STELLA_FULL_TITLE, STELLA_BUILD, BSPF::ARCH));
  const string name = useIntName
      ? string{myOSystem.console().properties().get(PropType::Cart_Name)}
      : myOSystem.romFile().getName();
  VarList::push_back(metaData, "ROM Name", name);
  VarList::push_back(metaData, "ROM MD5",
                     myOSystem.console().properties().get(PropType::Cart_MD5));
  VarList::push_back(metaData, "TV Effects",
                     myOSystem.frameBuffer().tiaSurface().effectsInfo());

  // Now create a PNG snapshot
  const bool autoCrop = myOSystem.settings().getBool("sscrop");
  string message = "Snapshot saved";
  if(myOSystem.settings().getBool("ss1x"))
  {
    try
    {
      Common::Rect rect;
      const FBSurface& surface =
        myOSystem.frameBuffer().tiaSurface().baseSurface(rect);
      if(autoCrop)
        rect = croppedRect(surface, rect, number);
      saveImage(filename, surface, rect, metaData);
    }
    catch(const std::runtime_error& e)
    {
      message = e.what();
    }
  }
  else
  {
    myOSystem.frameBuffer().tiaSurface().renderForSnapshot();

    try
    {
      const FBSurface& surface = myOSystem.frameBuffer().compositedSurface();
      Common::Rect rect;
      if(autoCrop)
        rect = croppedRect(surface,
            Common::Rect{0, 0, surface.width(), surface.height()}, number);
      saveImage(filename, surface, rect, metaData);
    }
    catch(const std::runtime_error& e)
    {
      message = e.what();
    }
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

  std::vector<png_text> text_ptr(numMetaData);
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
