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
// $Id: SnapSDL.cxx,v 1.1 2002-02-17 04:41:41 stephena Exp $
//============================================================================

#include <SDL.h>
#include <png.h>
#include <stdlib.h>
#include <iostream>

#include "SnapSDL.hxx"


SnapshotSDL::SnapshotSDL()
{
}

SnapshotSDL::~SnapshotSDL()
{
}

int SnapshotSDL::savePNG(SDL_Surface *surface, const char *file)
{
  SDL_RWops *out = SDL_RWFromFile(file, "wb");
  if(!out)
    return -1;

  int ret = IMG_SavePNG_RW(surface, out);
  SDL_RWclose(out);

  return ret;
}

/* Save a PNG type image to an SDL datasource */
void SnapshotSDL::png_write_data(png_structp ctx, png_bytep area, png_size_t size)
{
  SDL_RWops *src;
  src = (SDL_RWops *) png_get_io_ptr(ctx);
  SDL_RWwrite(src, area, size, 1);
}

void SnapshotSDL::png_io_flush(png_structp ctx)
{
  SDL_RWops *src;
  src = (SDL_RWops *)png_get_io_ptr(ctx);
	/* how do I flush src? */
}

int SnapshotSDL::png_colortype_from_surface(SDL_Surface *surface)
{
  int colortype = PNG_COLOR_MASK_COLOR; /* grayscale not supported */

  if (surface->format->palette)
    colortype |= PNG_COLOR_MASK_PALETTE;
  else if (surface->format->Amask)
    colortype |= PNG_COLOR_MASK_ALPHA;

  return colortype;
}

void SnapshotSDL::png_user_warn(png_structp ctx, png_const_charp str)
{
  cerr << "SnapshotSDL:  libpng warning: " << str << endl;
}

void SnapshotSDL::png_user_error(png_structp ctx, png_const_charp str)
{
  cerr << "SnapshotSDL:  libpng error: " << str << endl;
}

int SnapshotSDL::IMG_SavePNG_RW(SDL_Surface *surface, SDL_RWops *src)
{
  png_structp png_ptr = 0;
  png_infop info_ptr = 0;
  png_colorp palette = 0;
  png_bytep *row_pointers = 0;

  int i;
  int colortype;
  int result = 0;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_user_error, png_user_warn);
  if(png_ptr == NULL)
  {
    cerr << "SnapshotSDL:  Couldn't allocate memory for PNG file\n";
    return -1;
  }

  /* Allocate/initialize the image information data.  REQUIRED */
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
  {
    cerr << "SnapshotSDL:  Couldn't create image information for PNG file\n";
    goto done;
  }

  png_set_write_fn(png_ptr, src, png_write_data, png_io_flush);

  /* Set the image information here.  Width and height are up to 2^31,
   * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
   * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
   * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
   * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
   * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
   * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
   */

  colortype = png_colortype_from_surface(surface);
  png_set_IHDR(png_ptr, info_ptr, surface->w, surface->h, 8,
    colortype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
    PNG_FILTER_TYPE_BASE);

  /* set the palette if there is one.  REQUIRED for indexed-color images */
  if(colortype & PNG_COLOR_MASK_PALETTE)
  {
    palette = (png_colorp)malloc(surface->format->palette->ncolors * sizeof (png_color));
    if(!palette)
    {
      cerr << "SnapshotSDL:  Couldn't allocate memory for PNG palette\n";
      goto done;
    }

    for(i = 0; i < surface->format->palette->ncolors; i++)
    {
      palette[i].red   = surface->format->palette->colors[i].r;
      palette[i].green = surface->format->palette->colors[i].g;
      palette[i].blue  = surface->format->palette->colors[i].b;
    }

    png_set_PLTE(png_ptr, info_ptr, palette, surface->format->palette->ncolors);
  }

  // Write the file header information.  REQUIRED
  png_write_info(png_ptr, info_ptr);

  // pack pixels into bytes
  png_set_packing(png_ptr);

  // Create the array of pointers to image data
  row_pointers = (png_bytep*) malloc(sizeof(png_bytep)*surface->h);

  if((row_pointers == NULL))
  {
    cerr << "SnapshotSDL:  Couldn't allocate PNG row pointers\n";
    goto done;
  }

  for(i = 0; i < surface->h; i++)
    row_pointers[i] = (png_bytep)(Uint8 *)surface->pixels + i*surface->pitch;

  // write out the entire image data in one call 
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);

  result = 1;

  done:
  if(row_pointers)
    free(row_pointers);
  if(info_ptr->palette)
    free(info_ptr->palette);

  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
  return result;
}
