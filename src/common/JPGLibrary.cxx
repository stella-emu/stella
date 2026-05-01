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
#include <limits>

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

  const auto size = static_cast<size_t>(rawPos);
  if(size > static_cast<size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error{"JPG file too large"};

  in.seekg(0);

  vector<std::byte> fileBuffer(size);
  if(!in.read(reinterpret_cast<char*>(fileBuffer.data()),
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

  if(njDecode(reinterpret_cast<const char*>(fileBuffer.data()),
              static_cast<int>(size)))
    throw std::runtime_error{"Error decoding the JPG image"};

  // Read the entire image in one go
  const auto width  = static_cast<uInt32>(njGetWidth());
  const auto height = static_cast<uInt32>(njGetHeight());

  // njGetImage() points into nanojpeg's internal buffer — no extra copy needed
  const ByteSpan pixels{ njGetImage(),
      static_cast<size_t>(width) * static_cast<size_t>(height) * 3 };

  if(width > surface.width() || height > surface.height())
    surface.resize(width, height);

  surface.setSrcPos(0, 0);
  surface.setSrcSize(width, height);

  uInt32* s_buf{nullptr};
  uInt32  s_pitch{0};
  surface.basePtr(s_buf, s_pitch);

  const FrameBuffer& fb = myOSystem.frameBuffer();
  const size_t  i_pitch = static_cast<size_t>(width) * 3;
  const uInt8*  i_buf   = pixels.data();

  // Get the shift values for each colour component once, instead
  // of calling mapRGB per-pixel
  const uInt32 rShift = std::countr_zero(fb.mapRGB(0xFF, 0x00, 0x00));
  const uInt32 gShift = std::countr_zero(fb.mapRGB(0x00, 0xFF, 0x00));
  const uInt32 bShift = std::countr_zero(fb.mapRGB(0x00, 0x00, 0xFF));
  const uInt32 aMask  = fb.mapRGB(0x00, 0x00, 0x00);

  // Convert RGB triples into pixels and store in the surface
  for(uInt32 irow = 0; irow < height; ++irow, i_buf += i_pitch, s_buf += s_pitch)
  {
    const uInt8* i_ptr = i_buf;
    uInt32*      s_ptr = s_buf;  // NOLINT(misc-const-correctness)
    for(uInt32 icol = 0; icol < width; ++icol, i_ptr += 3)
      *s_ptr++ = aMask
               | (static_cast<uInt32>(i_ptr[0]) << rShift)
               | (static_cast<uInt32>(i_ptr[1]) << gShift)
               | (static_cast<uInt32>(i_ptr[2]) << bShift);
  }

  // Read the meta data we got
  metaData.clear();
  SpanStream stream{SpanOf<char>{
      reinterpret_cast<const char*>(fileBuffer.data()), size}};
  const TinyEXIF::EXIFInfo imageEXIF{stream};
  if(imageEXIF.Fields && !imageEXIF.ImageDescription.empty())
    VarList::push_back(metaData, "ImageDescription", imageEXIF.ImageDescription);
}

#endif  // IMAGE_SUPPORT
