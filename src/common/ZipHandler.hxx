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

#ifndef ZIP_HANDLER_HXX
#define ZIP_HANDLER_HXX

#include <fstream>
#include "bspf.hxx"

/***************************************************************************

    Copyright Aaron Giles
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#define ZIP_DECOMPRESS_BUFSIZE  16384

/**
  This class implements a thin wrapper around the zip file management code
  from the MAME project.

  @author  Wrapper class by Stephen Anthony, with main functionality
           by Aaron Giles
*/
class ZipHandler
{
  public:
    ZipHandler();
    virtual ~ZipHandler();

    // Open ZIP file for processing
    void open(const string& filename);

    // The following form an iterator for processing the filenames in the ZIP file
    void reset();     // Reset iterator to first file
    bool hasNext();   // Answer whether there are more files present
    string next();    // Get next file

    // Decompress the currently selected file and return its length
    // An exception will be thrown on any errors
    uInt32 decompress(uInt8*& image);

    // Answer the number of ROM files found in the archive
    // Currently, this means files with extension a26/bin/rom
    uInt16 romFiles() const { return myZip ? myZip->romfiles : 0; }

  private:
    // Replaces functionaity of various osd_xxxx functions
    static bool stream_open(const char* filename, fstream** stream, uInt64& length);
    static void stream_close(fstream** stream);
    static bool stream_read(fstream* stream, void* buffer, uInt64 offset,
                            uInt32 length, uInt32& actual);

    /* Error types */
    enum zip_error
    {
      ZIPERR_NONE = 0,
      ZIPERR_OUT_OF_MEMORY,
      ZIPERR_FILE_ERROR,
      ZIPERR_BAD_SIGNATURE,
      ZIPERR_DECOMPRESS_ERROR,
      ZIPERR_FILE_TRUNCATED,
      ZIPERR_FILE_CORRUPT,
      ZIPERR_UNSUPPORTED,
      ZIPERR_BUFFER_TOO_SMALL
    };

    /* contains extracted file header information */
    struct zip_file_header
    {
      uInt32      signature;            /* central file header signature */
      uInt16      version_created;      /* version made by */
      uInt16      version_needed;       /* version needed to extract */
      uInt16      bit_flag;             /* general purpose bit flag */
      uInt16      compression;          /* compression method */
      uInt16      file_time;            /* last mod file time */
      uInt16      file_date;            /* last mod file date */
      uInt32      crc;                  /* crc-32 */
      uInt32      compressed_length;    /* compressed size */
      uInt32      uncompressed_length;  /* uncompressed size */
      uInt16      filename_length;      /* filename length */
      uInt16      extra_field_length;   /* extra field length */
      uInt16      file_comment_length;  /* file comment length */
      uInt16      start_disk_number;    /* disk number start */
      uInt16      internal_attributes;  /* internal file attributes */
      uInt32      external_attributes;  /* external file attributes */
      uInt32      local_header_offset;  /* relative offset of local header */
      const char* filename;     /* filename */

      uInt8*      raw;          /* pointer to the raw data */
      uInt32      rawlength;    /* length of the raw data */
      uInt8       saved;        /* saved byte from after filename */
    };

    /* contains extracted end of central directory information */
    struct zip_ecd
    {
      uInt32      signature;            /* end of central dir signature */
      uInt16      disk_number;          /* number of this disk */
      uInt16      cd_start_disk_number; /* number of the disk with the start of the central directory */
      uInt16      cd_disk_entries;      /* total number of entries in the central directory on this disk */
      uInt16      cd_total_entries;     /* total number of entries in the central directory */
      uInt32      cd_size;              /* size of the central directory */
      uInt32      cd_start_disk_offset; /* offset of start of central directory with respect to the starting disk number */
      uInt16      comment_length;       /* .ZIP file comment length */
      const char* comment;              /* .ZIP file comment */

      uInt8*      raw;              /* pointer to the raw data */
      uInt32      rawlength;        /* length of the raw data */
    };

    /* describes an open ZIP file */
    struct zip_file
    {
      const char*     filename;   /* copy of ZIP filename (for caching) */
      fstream*        file;       /* C++ fstream file handle */
      uInt64          length;     /* length of zip file */
      uInt16          romfiles;   /* number of ROM files in central directory */

      zip_ecd         ecd;        /* end of central directory */

      uInt8*          cd;         /* central directory raw data */
      uInt32          cd_pos;     /* position in central directory */
      zip_file_header header;     /* current file header */

      uInt8 buffer[ZIP_DECOMPRESS_BUFSIZE]; /* buffer for decompression */
    };

    enum {
      /* number of open files to cache */
      ZIP_CACHE_SIZE = 8,

      /* offsets in end of central directory structure */
      ZIPESIG  = 0x00,
      ZIPEDSK  = 0x04,
      ZIPECEN  = 0x06,
      ZIPENUM  = 0x08,
      ZIPECENN = 0x0a,
      ZIPECSZ  = 0x0c,
      ZIPEOFST = 0x10,
      ZIPECOML = 0x14,
      ZIPECOM  = 0x16,

      /* offsets in central directory entry structure */
      ZIPCENSIG = 0x00,
      ZIPCVER   = 0x04,
      ZIPCOS    = 0x05,
      ZIPCVXT   = 0x06,
      ZIPCEXOS  = 0x07,
      ZIPCFLG   = 0x08,
      ZIPCMTHD  = 0x0a,
      ZIPCTIM   = 0x0c,
      ZIPCDAT   = 0x0e,
      ZIPCCRC   = 0x10,
      ZIPCSIZ   = 0x14,
      ZIPCUNC   = 0x18,
      ZIPCFNL   = 0x1c,
      ZIPCXTL   = 0x1e,
      ZIPCCML   = 0x20,
      ZIPDSK    = 0x22,
      ZIPINT    = 0x24,
      ZIPEXT    = 0x26,
      ZIPOFST   = 0x2a,
      ZIPCFN    = 0x2e,

      /* offsets in local file header structure */
      ZIPLOCSIG = 0x00,
      ZIPVER    = 0x04,
      ZIPGENFLG = 0x06,
      ZIPMTHD   = 0x08,
      ZIPTIME   = 0x0a,
      ZIPDATE   = 0x0c,
      ZIPCRC    = 0x0e,
      ZIPSIZE   = 0x12,
      ZIPUNCMP  = 0x16,
      ZIPFNLN   = 0x1a,
      ZIPXTRALN = 0x1c,
      ZIPNAME   = 0x1e
    };

  private:
    /* ----- ZIP file access ----- */

    /* open a ZIP file and parse its central directory */
    zip_error zip_file_open(const char *filename, zip_file **zip);

    /* close a ZIP file (may actually be left open due to caching) */
    void zip_file_close(zip_file *zip);

    /* clear out all open ZIP files from the cache */
    void zip_file_cache_clear(void);


    /* ----- contained file access ----- */

    /* find the next file in the ZIP */
    const zip_file_header *zip_file_next_file(zip_file *zip);

    /* decompress the most recently found file in the ZIP */
    zip_error zip_file_decompress(zip_file *zip, void *buffer, uInt32 length);

    inline static uInt16 read_word(uInt8* buf)
    {
      return (buf[1] << 8) | buf[0];
    }

    inline static uInt32 read_dword(uInt8* buf)
    {
      return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    }

    /* cache management */
    static void free_zip_file(zip_file *zip);

    /* ZIP file parsing */
    static zip_error read_ecd(zip_file *zip);
    static zip_error get_compressed_data_offset(zip_file *zip, uInt64 *offset);

    /* decompression interfaces */
    static zip_error decompress_data_type_0(zip_file *zip, uInt64 offset,
                                            void *buffer, uInt32 length);
    static zip_error decompress_data_type_8(zip_file *zip, uInt64 offset,
                                            void *buffer, uInt32 length);

  private:
    zip_file* myZip;
    zip_file* myZipCache[ZIP_CACHE_SIZE];
};

#endif  /* ZIP_HANDLER_HXX */
