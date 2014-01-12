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

#include <cctype>
#include <cstdlib>
#include <zlib.h>

#include "ZipHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::ZipHandler()
  : myZip(NULL)
{
  for (int cachenum = 0; cachenum < ZIP_CACHE_SIZE; cachenum++)
    myZipCache[cachenum] = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::~ZipHandler()
{
  zip_file_cache_clear();
  free_zip_file(myZip);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::open(const string& filename)
{
  // Close already open file
  if(myZip)
    zip_file_close(myZip);

  // And open a new one
  zip_file_open(filename.c_str(), &myZip);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::reset()
{
  /* reset the position and go from there */
  if(myZip)
    myZip->cd_pos = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::hasNext()
{
  return myZip && (myZip->cd_pos < myZip->ecd.cd_size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ZipHandler::next()
{
  if(myZip)
  {
    bool valid = false;
    const zip_file_header* header = NULL;
    do {
      header = zip_file_next_file(myZip);

      // Ignore zero-length files and '__MACOSX' virtual directories
      valid = header && (header->uncompressed_length > 0) &&
              !BSPF_startsWithIgnoreCase(header->filename, "__MACOSX");
    }
    while(!valid && myZip->cd_pos < myZip->ecd.cd_size);

    return valid ? header->filename : EmptyString;
  }
  else
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 ZipHandler::decompress(uInt8*& image)
{
  static const char* zip_error_s[] = {
    "ZIPERR_NONE",
    "ZIPERR_OUT_OF_MEMORY",
    "ZIPERR_FILE_ERROR",
    "ZIPERR_BAD_SIGNATURE",
    "ZIPERR_DECOMPRESS_ERROR",
    "ZIPERR_FILE_TRUNCATED",
    "ZIPERR_FILE_CORRUPT",
    "ZIPERR_UNSUPPORTED",
    "ZIPERR_BUFFER_TOO_SMALL"
  };

  if(myZip)
  {
    uInt32 length = myZip->header.uncompressed_length;
    image = new uInt8[length];

    ZipHandler::zip_error err = zip_file_decompress(myZip, image, length);
    if(err == ZIPERR_NONE)
      return length;
    else
    {
      delete[] image;  image = 0;
      length = 0;

      throw zip_error_s[err];
    }
  }
  else
    throw "Invalid ZIP archive";
}

/*-------------------------------------------------
    replaces functionality of various osd_xxx
    file access functions
-------------------------------------------------*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::stream_open(const char* filename, fstream** stream,
                             uInt64& length)
{
  fstream* in = new fstream(filename, fstream::in | fstream::binary);
  if(!in || !in->is_open())
  {
    *stream = NULL;
    length = 0;
    return false;
  }
  else
  {
    in->exceptions( ios_base::failbit | ios_base::badbit | ios_base::eofbit );
    *stream = in;
    in->seekg(0, ios::end);
    length = in->tellg();
    in->seekg(0, ios::beg);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::stream_close(fstream** stream)
{
  if(*stream)
  {
    if((*stream)->is_open())
      (*stream)->close();
    delete *stream;
    *stream = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::stream_read(fstream* stream, void* buffer, uInt64 offset,
                             uInt32 length, uInt32& actual)
{
  try
  {
    stream->seekg(offset);
    stream->read((char*)buffer, length);

    actual = stream->gcount();
    return true;
  }
  catch(...)
  {
    return false;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*-------------------------------------------------
    zip_file_open - opens a ZIP file for reading
-------------------------------------------------*/
ZipHandler::zip_error ZipHandler::zip_file_open(const char *filename, zip_file **zip)
{
  zip_error ziperr = ZIPERR_NONE;
  uInt32 read_length;
  zip_file *newzip;
  char *string;
  int cachenum;
  bool success;

  /* ensure we start with a NULL result */
  *zip = NULL;

  /* see if we are in the cache, and reopen if so */
  for (cachenum = 0; cachenum < ZIP_CACHE_SIZE; cachenum++)
  {
    zip_file *cached = myZipCache[cachenum];

    /* if we have a valid entry and it matches our filename, use it and remove
       from the cache */
    if (cached != NULL && cached->filename != NULL &&
        strcmp(filename, cached->filename) == 0)
    {
      *zip = cached;
      myZipCache[cachenum] = NULL;
      return ZIPERR_NONE;
    }
  }

  /* allocate memory for the zip_file structure */
  newzip = (zip_file *)malloc(sizeof(*newzip));
  if (newzip == NULL)
    return ZIPERR_OUT_OF_MEMORY;
  memset(newzip, 0, sizeof(*newzip));

  /* open the file */
  if(!stream_open(filename, &newzip->file, newzip->length))
  {
    ziperr = ZIPERR_FILE_ERROR;
    goto error;
  }

  /* read ecd data */
  ziperr = read_ecd(newzip);
  if (ziperr != ZIPERR_NONE)
    goto error;

  /* verify that we can work with this zipfile (no disk spanning allowed) */
  if (newzip->ecd.disk_number != newzip->ecd.cd_start_disk_number ||
      newzip->ecd.cd_disk_entries != newzip->ecd.cd_total_entries)
  {
    ziperr = ZIPERR_UNSUPPORTED;
    goto error;
  }

  /* allocate memory for the central directory */
  newzip->cd = (uInt8 *)malloc(newzip->ecd.cd_size + 1);
  if (newzip->cd == NULL)
  {
    ziperr = ZIPERR_OUT_OF_MEMORY;
    goto error;
  }

  /* read the central directory */
  success = stream_read(newzip->file, newzip->cd, newzip->ecd.cd_start_disk_offset,
                        newzip->ecd.cd_size, read_length);
  if (!success || read_length != newzip->ecd.cd_size)
  {
    ziperr = success ? ZIPERR_FILE_TRUNCATED : ZIPERR_FILE_ERROR;
    goto error;
  }

  /* make a copy of the filename for caching purposes */
  string = (char *)malloc(strlen(filename) + 1);
  if (string == NULL)
  {
    ziperr = ZIPERR_OUT_OF_MEMORY;
    goto error;
  }

  strcpy(string, filename);
  newzip->filename = string;
  *zip = newzip;

  // Count ROM files (we do it at this level so it will be cached)
  while(hasNext())
  {
    const std::string& file = next();
    if(BSPF_endsWithIgnoreCase(file, ".a26") ||
       BSPF_endsWithIgnoreCase(file, ".bin") ||
       BSPF_endsWithIgnoreCase(file, ".rom"))
      (*zip)->romfiles++;
  }

  return ZIPERR_NONE;

error:
  free_zip_file(newzip);
  return ziperr;
}


/*-------------------------------------------------
    zip_file_close - close a ZIP file and add it
    to the cache
-------------------------------------------------*/
void ZipHandler::zip_file_close(zip_file *zip)
{
  int cachenum;

  /* close the open files */
  if (zip->file)
    stream_close(&zip->file);

  /* find the first NULL entry in the cache */
  for (cachenum = 0; cachenum < ZIP_CACHE_SIZE; cachenum++)
    if (myZipCache[cachenum] == NULL)
      break;

  /* if no room left in the cache, free the bottommost entry */
  if (cachenum == ZIP_CACHE_SIZE)
    free_zip_file(myZipCache[--cachenum]);

  /* move everyone else down and place us at the top */
  if (cachenum != 0)
    memmove(&myZipCache[1], &myZipCache[0], cachenum * sizeof(myZipCache[0]));
  myZipCache[0] = zip;
}


/*-------------------------------------------------
    zip_file_cache_clear - clear the ZIP file
    cache and free all memory
-------------------------------------------------*/
void ZipHandler::zip_file_cache_clear(void)
{
  /* clear call cache entries */
  for (int cachenum = 0; cachenum < ZIP_CACHE_SIZE; cachenum++)
    if (myZipCache[cachenum] != NULL)
    {
      free_zip_file(myZipCache[cachenum]);
      myZipCache[cachenum] = NULL;
    }
}


/***************************************************************************
    CONTAINED FILE ACCESS
***************************************************************************/

/*-------------------------------------------------
    zip_file_next_entry - return the next entry
    in the ZIP
-------------------------------------------------*/
const ZipHandler::zip_file_header* ZipHandler::zip_file_next_file(zip_file *zip)
{
  /* fix up any modified data */
  if (zip->header.raw != NULL)
  {
    zip->header.raw[ZIPCFN + zip->header.filename_length] = zip->header.saved;
    zip->header.raw = NULL;
  }

  /* if we're at or past the end, we're done */
  if (zip->cd_pos >= zip->ecd.cd_size)
    return NULL;

  /* extract file header info */
  zip->header.raw                 = zip->cd + zip->cd_pos;
  zip->header.rawlength           = ZIPCFN;
  zip->header.signature           = read_dword(zip->header.raw + ZIPCENSIG);
  zip->header.version_created     = read_word (zip->header.raw + ZIPCVER);
  zip->header.version_needed      = read_word (zip->header.raw + ZIPCVXT);
  zip->header.bit_flag            = read_word (zip->header.raw + ZIPCFLG);
  zip->header.compression         = read_word (zip->header.raw + ZIPCMTHD);
  zip->header.file_time           = read_word (zip->header.raw + ZIPCTIM);
  zip->header.file_date           = read_word (zip->header.raw + ZIPCDAT);
  zip->header.crc                 = read_dword(zip->header.raw + ZIPCCRC);
  zip->header.compressed_length   = read_dword(zip->header.raw + ZIPCSIZ);
  zip->header.uncompressed_length = read_dword(zip->header.raw + ZIPCUNC);
  zip->header.filename_length     = read_word (zip->header.raw + ZIPCFNL);
  zip->header.extra_field_length  = read_word (zip->header.raw + ZIPCXTL);
  zip->header.file_comment_length = read_word (zip->header.raw + ZIPCCML);
  zip->header.start_disk_number   = read_word (zip->header.raw + ZIPDSK);
  zip->header.internal_attributes = read_word (zip->header.raw + ZIPINT);
  zip->header.external_attributes = read_dword(zip->header.raw + ZIPEXT);
  zip->header.local_header_offset = read_dword(zip->header.raw + ZIPOFST);
  zip->header.filename            = (char *)zip->header.raw + ZIPCFN;

  /* make sure we have enough data */
  zip->header.rawlength += zip->header.filename_length;
  zip->header.rawlength += zip->header.extra_field_length;
  zip->header.rawlength += zip->header.file_comment_length;
  if (zip->cd_pos + zip->header.rawlength > zip->ecd.cd_size)
    return NULL;

  /* NULL terminate the filename */
  zip->header.saved = zip->header.raw[ZIPCFN + zip->header.filename_length];
  zip->header.raw[ZIPCFN + zip->header.filename_length] = 0;

  /* advance the position */
  zip->cd_pos += zip->header.rawlength;
  return &zip->header;
}

/*-------------------------------------------------
    zip_file_decompress - decompress a file
    from a ZIP into the target buffer
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::zip_file_decompress(zip_file *zip, void *buffer, uInt32 length)
{
  zip_error ziperr;
  uInt64 offset;

  /* if we don't have enough buffer, error */
  if (length < zip->header.uncompressed_length)
    return ZIPERR_BUFFER_TOO_SMALL;

  /* make sure the info in the header aligns with what we know */
  if (zip->header.start_disk_number != zip->ecd.disk_number)
    return ZIPERR_UNSUPPORTED;

  /* get the compressed data offset */
  ziperr = get_compressed_data_offset(zip, &offset);
  if (ziperr != ZIPERR_NONE)
    return ziperr;

  /* handle compression types */
  switch (zip->header.compression)
  {
    case 0:
      ziperr = decompress_data_type_0(zip, offset, buffer, length);
      break;

    case 8:
      ziperr = decompress_data_type_8(zip, offset, buffer, length);
      break;

    default:
      ziperr = ZIPERR_UNSUPPORTED;
      break;
  }
  return ziperr;
}

/***************************************************************************
    CACHE MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    free_zip_file - free all the data for a
    zip_file
-------------------------------------------------*/
void ZipHandler::free_zip_file(zip_file *zip)
{
  if (zip != NULL)
  {
    if (zip->file)
      stream_close(&zip->file);
    if (zip->filename != NULL)
      free((void *)zip->filename);
    if (zip->ecd.raw != NULL)
      free(zip->ecd.raw);
    if (zip->cd != NULL)
      free(zip->cd);
    free(zip);
  }
}

/***************************************************************************
    ZIP FILE PARSING
***************************************************************************/

/*-------------------------------------------------
    read_ecd - read the ECD data
-------------------------------------------------*/

ZipHandler::zip_error ZipHandler::read_ecd(zip_file *zip)
{
  uInt32 buflen = 1024;
  uInt8 *buffer;

  /* we may need multiple tries */
  while (buflen < 65536)
  {
    uInt32 read_length;
    Int32 offset;

    /* max out the buffer length at the size of the file */
    if (buflen > zip->length)
      buflen = zip->length;

    /* allocate buffer */
    buffer = (uInt8 *)malloc(buflen + 1);
    if (buffer == NULL)
      return ZIPERR_OUT_OF_MEMORY;

    /* read in one buffers' worth of data */
    bool success = stream_read(zip->file, buffer, zip->length - buflen,
                               buflen, read_length);
    if (!success || read_length != buflen)
    {
      free(buffer);
      return ZIPERR_FILE_ERROR;
    }

    /* find the ECD signature */
    for (offset = buflen - 22; offset >= 0; offset--)
      if (buffer[offset + 0] == 'P' && buffer[offset + 1] == 'K' &&
          buffer[offset + 2] == 0x05 && buffer[offset + 3] == 0x06)
        break;

    /* if we found it, fill out the data */
    if (offset >= 0)
    {
      /* reuse the buffer as our ECD buffer */
      zip->ecd.raw = buffer;
      zip->ecd.rawlength = buflen - offset;

      /* append a NULL terminator to the comment */
      memmove(&buffer[0], &buffer[offset], zip->ecd.rawlength);
      zip->ecd.raw[zip->ecd.rawlength] = 0;

      /* extract ecd info */
      zip->ecd.signature            = read_dword(zip->ecd.raw + ZIPESIG);
      zip->ecd.disk_number          = read_word (zip->ecd.raw + ZIPEDSK);
      zip->ecd.cd_start_disk_number = read_word (zip->ecd.raw + ZIPECEN);
      zip->ecd.cd_disk_entries      = read_word (zip->ecd.raw + ZIPENUM);
      zip->ecd.cd_total_entries     = read_word (zip->ecd.raw + ZIPECENN);
      zip->ecd.cd_size              = read_dword(zip->ecd.raw + ZIPECSZ);
      zip->ecd.cd_start_disk_offset = read_dword(zip->ecd.raw + ZIPEOFST);
      zip->ecd.comment_length       = read_word (zip->ecd.raw + ZIPECOML);
      zip->ecd.comment              = (const char *)(zip->ecd.raw + ZIPECOM);
      return ZIPERR_NONE;
    }

    /* didn't find it; free this buffer and expand our search */
    free(buffer);
    if (buflen < zip->length)
      buflen *= 2;
    else
      return ZIPERR_BAD_SIGNATURE;
  }
  return ZIPERR_OUT_OF_MEMORY;
}

/*-------------------------------------------------
    get_compressed_data_offset - return the
    offset of the compressed data
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::get_compressed_data_offset(zip_file *zip, uInt64 *offset)
{
  uInt32 read_length;

  /* make sure the file handle is open */
  if (zip->file == NULL && !stream_open(zip->filename, &zip->file, zip->length))
    return ZIPERR_FILE_ERROR;

  /* now go read the fixed-sized part of the local file header */
  bool success = stream_read(zip->file, zip->buffer, zip->header.local_header_offset,
                             ZIPNAME, read_length);
  if (!success || read_length != ZIPNAME)
    return success ? ZIPERR_FILE_TRUNCATED : ZIPERR_FILE_ERROR;

  /* compute the final offset */
  *offset = zip->header.local_header_offset + ZIPNAME;
  *offset += read_word(zip->buffer + ZIPFNLN);
  *offset += read_word(zip->buffer + ZIPXTRALN);

  return ZIPERR_NONE;
}

/***************************************************************************
    DECOMPRESSION INTERFACES
***************************************************************************/

/*-------------------------------------------------
    decompress_data_type_0 - "decompress"
    type 0 data (which is uncompressed)
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::decompress_data_type_0(zip_file *zip, uInt64 offset,
                                       void *buffer, uInt32 length)
{
  uInt32 read_length;

  /* the data is uncompressed; just read it */
  bool success = stream_read(zip->file, buffer, offset, zip->header.compressed_length,
                             read_length);
  if (!success)
    return ZIPERR_FILE_ERROR;
  else if (read_length != zip->header.compressed_length)
    return ZIPERR_FILE_TRUNCATED;
  else
    return ZIPERR_NONE;
}

/*-------------------------------------------------
    decompress_data_type_8 - decompress
    type 8 data (which is deflated)
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::decompress_data_type_8(zip_file *zip, uInt64 offset,
                                       void *buffer, uInt32 length)
{
  uInt32 input_remaining = zip->header.compressed_length;
  uInt32 read_length;
  z_stream stream;
  int zerr;

#if 0
  // TODO - check newer versions of ZIP, and determine why this specific
  //        version (0x14) is important
  /* make sure we don't need a newer mechanism */
  if (zip->header.version_needed > 0x14)
    return ZIPERR_UNSUPPORTED;
#endif

  /* reset the stream */
  memset(&stream, 0, sizeof(stream));
  stream.next_out = (Bytef *)buffer;
  stream.avail_out = length;

  /* initialize the decompressor */
  zerr = inflateInit2(&stream, -MAX_WBITS);
  if (zerr != Z_OK)
    return ZIPERR_DECOMPRESS_ERROR;

  /* loop until we're done */
  while (1)
  {
    /* read in the next chunk of data */
    bool success = stream_read(zip->file, zip->buffer, offset,
                      BSPF_min(input_remaining, (uInt32)sizeof(zip->buffer)),
                      read_length);
    if (!success)
    {
      inflateEnd(&stream);
      return ZIPERR_FILE_ERROR;
    }
    offset += read_length;

    /* if we read nothing, but still have data left, the file is truncated */
    if (read_length == 0 && input_remaining > 0)
    {
      inflateEnd(&stream);
      return ZIPERR_FILE_TRUNCATED;
    }

    /* fill out the input data */
    stream.next_in = zip->buffer;
    stream.avail_in = read_length;
    input_remaining -= read_length;

    /* add a dummy byte at end of compressed data */
    if (input_remaining == 0)
      stream.avail_in++;

    /* now inflate */
    zerr = inflate(&stream, Z_NO_FLUSH);
    if (zerr == Z_STREAM_END)
      break;
    if (zerr != Z_OK)
    {
      inflateEnd(&stream);
      return ZIPERR_DECOMPRESS_ERROR;
    }
  }

  /* finish decompression */
  zerr = inflateEnd(&stream);
  if (zerr != Z_OK)
    return ZIPERR_DECOMPRESS_ERROR;

  /* if anything looks funny, report an error */
  if (stream.avail_out > 0 || input_remaining > 0)
    return ZIPERR_DECOMPRESS_ERROR;

  return ZIPERR_NONE;
}
