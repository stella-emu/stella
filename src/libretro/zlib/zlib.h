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

#ifndef ZLIB_H
#define ZLIB_H

#include "bspf.hxx"


#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4
#define Z_DEFAULT_STRATEGY    0

#define Z_BINARY   0
#define Z_TEXT     1
#define Z_ASCII    Z_TEXT
#define Z_UNKNOWN  2

#define Z_DEFLATED   8

#define Z_NULL  0

#define MAX_WBITS 16


#define Bytef uInt8


typedef struct z_stream_s {
    uInt8*   next_in;
    uInt32   avail_in;
    uInt32   total_in;

    uInt8*   next_out;
    uInt32   avail_out;
    uInt32   total_out;

    char*    msg;

    void*    zalloc;
    void*    zfree;
	void*    opaque;

    int      data_type;
    uInt32   adler;
    uInt32   reserved;
} z_stream;

typedef z_stream *z_streamp;

struct gzFile_s {
    unsigned have;
    unsigned char *next;
    uInt64 pos;
};

typedef struct gzFile_s *gzFile;    /* semi-opaque gzip file descriptor */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline int inflateInit2(z_streamp, int) { return Z_ERRNO; }
static inline bool readStream(z_streamp, int, int, int) { return false; }

static inline int inflate(z_streamp, int) { return Z_ERRNO; }
static inline int inflateEnd(z_streamp) { return Z_ERRNO; }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static inline gzFile gzopen(const char*, const char*) { return nullptr; }
static inline int gzread(gzFile, void*, unsigned) { return Z_ERRNO; }
static inline int gzclose(gzFile) { return Z_ERRNO; }

#endif /* ZLIB_H */
