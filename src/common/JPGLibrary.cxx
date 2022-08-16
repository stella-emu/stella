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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef IMAGE_SUPPORT 
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "nanojpeg_lib.hxx"

#include "JPGLibrary.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JPGLibrary::JPGLibrary(OSystem& osystem)
  : myOSystem{osystem}
{
  njInit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JPGLibrary::loadImage(const string& filename, FBSurface& surface, VariantList& comments)
{
  const auto loadImageERROR = [&](const char* s) {
    if(s)
      throw runtime_error(s);
  };  

  std::ifstream in(filename, std::ios_base::binary | std::ios::ate);
  if(!in.is_open())
    loadImageERROR("No image found");
  size_t size = in.tellg();
  in.clear();
  in.seekg(0);

  // Create space for the entire file
  if(size > myFileBuffer.size())
    myFileBuffer.resize(size);
  if(!in.read(myFileBuffer.data(), size))
    loadImageERROR("Image data reading failed");
  in.close();

  if(njDecode(myFileBuffer.data(), static_cast<int>(size)))
    loadImageERROR("Error decoding the input file");

  // Read the entire image in one go
  myReadInfo.buffer = njGetImage();
  myReadInfo.width = njGetWidth();
  myReadInfo.height = njGetHeight();
  myReadInfo.pitch = myReadInfo.width * 3;

  // Read the comments we got   TODO
  //readComments(png_ptr, info_ptr, comments);

  // Load image into the surface, setting the correct dimensions
  loadImagetoSurface(surface);

  // Cleanup
  njDone();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JPGLibrary::loadImagetoSurface(FBSurface& surface)
{
  // First determine if we need to resize the surface
  const uInt32 iw = myReadInfo.width, ih = myReadInfo.height;
  if(iw > surface.width() || ih > surface.height())
    surface.resize(iw, ih);

  // The source dimensions are set here; the destination dimensions are
  // set by whoever owns the surface
  surface.setSrcPos(0, 0);
  surface.setSrcSize(iw, ih);

  // Convert RGB triples into pixels and store in the surface
  uInt32* s_buf, s_pitch;
  surface.basePtr(s_buf, s_pitch);
  const uInt8* i_buf = myReadInfo.buffer;
  const uInt32 i_pitch = myReadInfo.pitch;

  const FrameBuffer& fb = myOSystem.frameBuffer();
  for(uInt32 irow = 0; irow < ih; ++irow, i_buf += i_pitch, s_buf += s_pitch)
  {
    const uInt8* i_ptr = i_buf;
    uInt32* s_ptr = s_buf;
    for(uInt32 icol = 0; icol < myReadInfo.width; ++icol, i_ptr += 3)
      *s_ptr++ = fb.mapRGB(*i_ptr, *(i_ptr + 1), *(i_ptr + 2));
  }
}

//// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void JPGLibrary::readComments(const png_structp png_ptr, png_infop info_ptr,
//  VariantList& comments)
//{
//  png_textp text_ptr;
//  int numComments = 0;
//
//  // TODO: currently works only if comments are *before* the image data
//  png_get_text(png_ptr, info_ptr, &text_ptr, &numComments);
//
//  comments.clear();
//  for(int i = 0; i < numComments; ++i)
//  {
//    VarList::push_back(comments, text_ptr[i].key, text_ptr[i].text);
//  }
//}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<char> JPGLibrary::myFileBuffer;

#endif  // IMAGE_SUPPORT
