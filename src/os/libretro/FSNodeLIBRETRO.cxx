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

#include "bspf.hxx"
#include "Cart.hxx"
#include "FSNodeLIBRETRO.hxx"
#include "libretro.h"

// Declared in libretro.cxx
extern retro_vfs_interface* libretro_vfs;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
extern string libretro_save_dir;           // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
extern string libretro_rom_path;           // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

extern uInt32 libretro_read_rom(void* data);
extern uInt32 libretro_get_rom_size();

namespace {

/**
  Read an entire file through the frontend VFS.

  @param path    The file to read (as the frontend understands it)
  @param buffer  Receives the data
  @param size    Maximum number of bytes to read (0 means the whole file)

  @return  The number of bytes read, or 0 if the VFS could not read the file
*/
size_t vfsReadFile(const string& path, ByteArray& buffer, size_t size)
{
  if(!libretro_vfs || !libretro_vfs->open || !libretro_vfs->read)
    return 0;

  retro_vfs_file_handle* file = libretro_vfs->open(path.c_str(),
      RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
  if(!file)
    return 0;

  const int64_t fileSize = libretro_vfs->size(file);
  if(fileSize <= 0)
  {
    libretro_vfs->close(file);
    return 0;
  }

  // If a requested size to read is provided (size > 0), honour it
  const size_t sizeToRead = (size > 0)
    ? std::min(static_cast<size_t>(fileSize), size)
    : static_cast<size_t>(fileSize);

  buffer.resize(sizeToRead);
  const int64_t bytesRead = libretro_vfs->read(file, buffer.data(), sizeToRead);
  libretro_vfs->close(file);

  if(bytesRead <= 0)
    return 0;

  buffer.resize(static_cast<size_t>(bytesRead));
  return static_cast<size_t>(bytesRead);
}

/**
  Write a buffer to a file through the frontend VFS.

  @return  The number of bytes written, or 0 if the VFS could not write it
*/
size_t vfsWriteFile(const string& path, const void* data, size_t size)
{
  if(!libretro_vfs || !libretro_vfs->open || !libretro_vfs->write)
    return 0;

  retro_vfs_file_handle* file = libretro_vfs->open(path.c_str(),
      RETRO_VFS_FILE_ACCESS_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE);
  if(!file)
    return 0;

  const int64_t bytesWritten = libretro_vfs->write(file, data, size);
  if(libretro_vfs->flush)
    libretro_vfs->flush(file);
  libretro_vfs->close(file);

  return bytesWritten > 0 ? static_cast<size_t>(bytesWritten) : 0;
}

}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeLIBRETRO::FSNodeLIBRETRO()
  : _path{libretro_save_dir.empty()
      ? string(".") + FSNode::PATH_SEPARATOR
      : libretro_save_dir},
    _displayName{"."}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeLIBRETRO::FSNodeLIBRETRO(string_view path, bool verify)
  : _path{!path.empty() ? path : "."}
{
  _displayName = string(lastPathComponent(_path));
  if(_displayName.empty())
    _displayName = _path;

  if(verify)
    setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeLIBRETRO::setFlags()
{
  if(libretro_vfs && libretro_vfs->stat)
  {
    int32_t file_size = 0;
    const int flags = libretro_vfs->stat(_path.c_str(), &file_size);

    if(flags & RETRO_VFS_STAT_IS_VALID)
    {
      _isDirectory = (flags & RETRO_VFS_STAT_IS_DIRECTORY) != 0;
      _isFile = !_isDirectory && !(flags & RETRO_VFS_STAT_IS_CHARACTER_SPECIAL);
      _size = static_cast<size_t>(file_size);

      if(_isDirectory && !_path.empty() && _path.back() != FSNode::PATH_SEPARATOR)
        _path += FSNode::PATH_SEPARATOR;

      return true;
    }
    else
    {
      // Not present on disk, but it may still be the in-memory ROM: either the
      // path isn't directly readable (e.g. Android), or the frontend extracted
      // the ROM from an archive and handed us an archive-relative path
      // (e.g. 'game.zip#game.a26')
      _isDirectory = false;
      _isFile = isMemoryROM();
      _size = _isFile ? libretro_get_rom_size() : 0;
      return _isFile;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeLIBRETRO::isMemoryROM() const
{
  return !_path.empty() && _path == libretro_rom_path &&
         libretro_get_rom_size() > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeLIBRETRO::exists() const
{
  if(libretro_vfs && libretro_vfs->stat)
  {
    int32_t unused = 0;
    if(libretro_vfs->stat(_path.c_str(), &unused) & RETRO_VFS_STAT_IS_VALID)
      return true;

    // File not found on disk; it may still be the in-memory ROM on platforms
    // where the ROM path isn't directly accessible (e.g. Android)
    return isMemoryROM();
  }
  return true;  // VFS unavailable: assume exists (backward-compatible fallback)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeLIBRETRO::hasParent() const
{
  return !_path.empty() && _path != string(1, FSNode::PATH_SEPARATOR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodeLIBRETRO::getParent() const
{
  if(_path == string(1, FSNode::PATH_SEPARATOR))
    return nullptr;

  return std::make_unique<FSNodeLIBRETRO>(stemPathComponent(_path));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeLIBRETRO::getSize() const
{
  if(!_size.has_value() && _isFile && libretro_vfs && libretro_vfs->stat)
  {
    int32_t file_size = 0;
    libretro_vfs->stat(_path.c_str(), &file_size);
    _size = static_cast<size_t>(file_size);
  }
  return _size.value_or(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeLIBRETRO::getChildren(AbstractFSList& myList, ListMode mode) const
{
  if(!_isDirectory || !libretro_vfs || !libretro_vfs->opendir)
    return false;

  retro_vfs_dir_handle* dirp = libretro_vfs->opendir(_path.c_str(), false);
  if(!dirp)
    return false;

  while(libretro_vfs->readdir(dirp))
  {
    const char* entry_name = libretro_vfs->dirent_get_name(dirp);
    if(!entry_name || entry_name[0] == '.')
      continue;

    string newPath(_path);
    if(!newPath.empty() && newPath.back() != FSNode::PATH_SEPARATOR)
      newPath += FSNode::PATH_SEPARATOR;
    newPath += entry_name;

    FSNodeLIBRETRO entry(newPath, false);
    entry._isDirectory = libretro_vfs->dirent_is_dir(dirp);
    entry._isFile = !entry._isDirectory;

    if(entry._isDirectory)
      entry._path += FSNode::PATH_SEPARATOR;

    if((mode == FSNode::ListMode::FilesOnly && !entry._isFile) ||
       (mode == FSNode::ListMode::DirectoriesOnly && !entry._isDirectory))
      continue;

    myList.emplace_back(std::make_unique<FSNodeLIBRETRO>(std::move(entry)));
  }
  libretro_vfs->closedir(dirp);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeLIBRETRO::makeDir()
{
  if(libretro_vfs && libretro_vfs->mkdir)
  {
    // Strip trailing separator before passing to VFS mkdir
    string p = _path;
    if(!p.empty() && p.back() == FSNode::PATH_SEPARATOR)
      p.pop_back();

    const int result = libretro_vfs->mkdir(p.c_str());
    if(result == 0 || result == -2)  // 0 = created, -2 = already exists
    {
      _displayName = string(lastPathComponent(_path));
      return setFlags();
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeLIBRETRO::rename(string_view newfile)
{
  if(libretro_vfs && libretro_vfs->rename)
  {
    string newPath{newfile};
    if(libretro_vfs->rename(_path.c_str(), newPath.c_str()) == 0)
    {
      _path = std::move(newPath);
      _displayName = string(lastPathComponent(_path));
      return setFlags();
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeLIBRETRO::read(ByteArray& image, size_t size) const
{
  // Read through the frontend: the path may only be meaningful to it (Android
  // SAF URIs, network shares), and where it is an ordinary path this still
  // ends up as an ordinary file read
  if(const size_t bytesRead = vfsReadFile(_path, image, size); bytesRead > 0)
    return bytesRead;

  // File not accessible through the VFS — serve the in-memory ROM buffer.
  // This handles platforms (e.g. Android) where need_fullpath=false means
  // RetroArch loads the ROM into memory but the path isn't directly readable.
  if(isMemoryROM())
  {
    image.resize(Cartridge::maxSize());
    const size_t romSize = libretro_read_rom(image.data());

    // Trim to what was actually there: callers hash and probe the buffer
    // itself, so one padded out to maxSize() gives the wrong MD5 (and with
    // it the wrong properties) and the wrong bankswitch type
    image.resize(romSize);
    return romSize;
  }

  // Let the base class fall back to a normal C++ stream
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeLIBRETRO::read(std::stringstream& buffer) const
{
  ByteArray data;

  const size_t bytesRead = vfsReadFile(_path, data, 0);
  if(bytesRead == 0)
    return 0;

  buffer.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(bytesRead));

  return buffer ? bytesRead : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeLIBRETRO::write(ByteSpan buffer) const
{
  return vfsWriteFile(_path, buffer.data(), buffer.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeLIBRETRO::write(string_view buffer) const
{
  return vfsWriteFile(_path, buffer.data(), buffer.size());
}
