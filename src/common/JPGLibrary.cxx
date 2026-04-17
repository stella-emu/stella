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

#include <limits>
#include <span>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "FSNode.hxx"
#include "SpanStream.hxx"
#include "nanojpeg_lib.hxx"
#include "tinyexif_lib.hxx"

#include "JPGLibrary.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JPGLibrary::JPGLibrary(OSystem& osystem)
  : myOSystem{osystem}
{
  njInit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JPGLibrary::loadImage(string_view filename, FBSurface& surface,
                           VariantList& metaData)
{
  auto in = FSNode(filename).openIFStream(std::ios_base::binary |
                                          std::ios_base::ate);
  if(!in.is_open())
    throw std::runtime_error{"No image found"};

  const auto rawPos = in.tellg();
  if(rawPos < 0)
    throw std::runtime_error{"Failed to determine JPG file size"};
  const auto size = static_cast<std::size_t>(rawPos);
  in.seekg(0);

  if(size > static_cast<size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error{"JPG file too large"};

  myFileBuffer.resize(size);

  if(!in.read(reinterpret_cast<char*>(myFileBuffer.data()),
              static_cast<std::streamsize>(size)))
    throw std::runtime_error{"JPG image data reading failed"};

  // RAII guard: ensures njDone() is always called on scope exit
  struct NJGuard {
    NJGuard() = default;
    ~NJGuard() { njDone(); }
    NJGuard(const NJGuard&) = delete;
    NJGuard(NJGuard&&) = delete;
    NJGuard& operator=(const NJGuard&) = delete;
    NJGuard& operator=(NJGuard&&) = delete;
  };

  const NJGuard guard;

  if(njDecode(reinterpret_cast<const char*>(myFileBuffer.data()),
              static_cast<int>(size)))
    throw std::runtime_error{"Error decoding the JPG image"};

  // Read the entire image in one go
  const auto width  = static_cast<uInt32>(njGetWidth());
  const auto height = static_cast<uInt32>(njGetHeight());
  const auto pixels = std::span<const uInt8>{ njGetImage(),
      static_cast<size_t>(width) * static_cast<size_t>(height) * 3 };

  // Read the meta data we got
  readMetaData({myFileBuffer.data(), size}, metaData);

  // Load image into the surface, setting the correct dimensions
  loadImagetoSurface(surface, pixels, width, height);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JPGLibrary::loadImagetoSurface(FBSurface& surface,
                                    std::span<const uInt8> pixels,
                                    uInt32 width, uInt32 height)
{
  // First determine if we need to resize the surface
  if(width > surface.width() || height > surface.height())
    surface.resize(width, height);

  // The source dimensions are set here; the destination dimensions are
  // set by whoever owns the surface
  surface.setSrcPos(0, 0);
  surface.setSrcSize(width, height);

  // Convert RGB triples into pixels and store in the surface
  uInt32 *s_buf{nullptr}, s_pitch{0};
  surface.basePtr(s_buf, s_pitch);

  const FrameBuffer& fb = myOSystem.frameBuffer();
  const size_t i_pitch  = static_cast<size_t>(width) * 3;
  const uInt8* i_buf    = pixels.data();

  for(uInt32 irow = 0; irow < height; ++irow, i_buf += i_pitch, s_buf += s_pitch)
  {
    const uInt8* i_ptr = i_buf;
    uInt32*      s_ptr = s_buf;  // NOLINT(misc-const-correctness)
    for(uInt32 icol = 0; icol < width; ++icol, i_ptr += 3)
      *s_ptr++ = fb.mapRGB(*i_ptr, *(i_ptr+1), *(i_ptr+2));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JPGLibrary::readMetaData(std::span<const std::byte> file,
                              VariantList& metaData)
{
  metaData.clear();
  SpanStream stream{std::span<const char>{
    reinterpret_cast<const char*>(file.data()), file.size()}};
  const TinyEXIF::EXIFInfo imageEXIF{stream};
  if(imageEXIF.Fields && !imageEXIF.ImageDescription.empty())
    VarList::push_back(metaData, "ImageDescription", imageEXIF.ImageDescription);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
vector<std::byte> JPGLibrary::myFileBuffer;

#endif  // IMAGE_SUPPORT
