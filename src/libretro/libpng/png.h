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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PNG_H
#define PNG_H

#include "bspf.hxx"


#define PNG_LIBPNG_VER_STRING "1.6.36"
#define PNG_HEADER_VERSION_STRING "libpng version 1.6.36 - December 1, 2018\n"

#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

#define PNG_COMPRESSION_TYPE_BASE 0
#define PNG_COMPRESSION_TYPE_DEFAULT PNG_COMPRESSION_TYPE_BASE

#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

#define PNG_INTERLACE_NONE        0
#define PNG_INTERLACE_ADAM7       1
#define PNG_INTERLACE_LAST        2

#define PNG_FILTER_TYPE_BASE      0
#define PNG_INTRAPIXEL_DIFFERENCING 64
#define PNG_FILTER_TYPE_DEFAULT   PNG_FILTER_TYPE_BASE

#define PNG_TEXT_COMPRESSION_NONE_WR -3
#define PNG_TEXT_COMPRESSION_zTXt_WR -2
#define PNG_TEXT_COMPRESSION_NONE    -1

#define PNG_FILLER_BEFORE 0
#define PNG_FILLER_AFTER 1

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef void *png_voidp;

typedef unsigned char png_byte;
typedef png_byte *png_bytep;
typedef png_bytep *png_bytepp;

typedef uInt32 png_uint_32;

typedef char *png_charp;
typedef const png_charp png_const_charp;

typedef size_t png_size_t;

typedef struct png_struct_def
{
} png_struct;

typedef struct png_info_struct_def
{
} png_info;

typedef struct png_text_struct_def {
    int compression;
    png_charp key;
    png_charp text;
    png_size_t text_length;
} png_text;

typedef png_struct *png_structp;
typedef png_structp *png_structpp;

typedef png_info *png_infop;
typedef png_infop *png_infopp;

typedef png_text *png_textp;
typedef png_textp *png_textpp;

typedef void (*png_error_ptr) (png_structp, png_const_charp);
typedef void (*png_rw_ptr) (png_structp, png_bytep, png_size_t);
typedef void (*png_flush_ptr) (png_structp);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline png_structp png_create_read_struct(png_const_charp, png_voidp, png_error_ptr, png_error_ptr) { return nullptr; }
static inline png_structp png_create_write_struct(png_const_charp, png_voidp, png_error_ptr, png_error_ptr) { return nullptr; }
static inline png_infop png_create_info_struct(png_structp) { return nullptr; }

static inline void png_set_palette_to_rgb(png_structp) { }
static inline void png_set_bgr(png_structp) { }
static inline void png_set_strip_alpha(png_structp) { }
static inline void png_set_swap_alpha(png_structp) { }
static inline void png_set_filler(png_structp, png_uint_32, int) { }
static inline void png_set_packing(png_structp) { }
static inline void png_set_strip_16(png_structp) { }
static inline void png_set_text(png_structp, png_infop, png_textp, int) { }

static inline void png_set_read_fn(png_structp, png_voidp, png_rw_ptr) { }
static inline void png_set_write_fn(png_structp, png_voidp, png_rw_ptr, png_flush_ptr) { }

static inline png_voidp png_get_io_ptr(png_structp) { return nullptr; }

static inline png_uint_32 png_get_IHDR(png_structp, png_infop, png_uint_32*, png_uint_32*, int*, int*, int*, int*, int*) { return 0; }
static inline void png_set_IHDR(png_structp, png_infop, png_uint_32, png_uint_32, int, int, int, int, int) { }

static inline void png_read_info(png_structp, png_infop) { }
static inline void png_read_image(png_structp, png_bytepp) { }
static inline void png_read_end(png_structp, png_infop) { }

static inline void png_write_info(png_structp, png_infop) { }
static inline void png_write_image(png_structp, png_bytepp) { }
static inline void png_write_end(png_structp, png_infop) { }

static inline void png_destroy_read_struct(png_structpp, png_infopp, png_infopp) { }
static inline void png_destroy_write_struct(png_structpp, png_infopp) { }

#endif /* PNG_H */
