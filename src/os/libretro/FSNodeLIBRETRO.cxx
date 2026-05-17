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
extern retro_vfs_interface* libretro_vfs;
extern string libretro_save_dir;
extern string libretro_rom_path;

extern uInt32 libretro_read_rom(void* data);
extern uInt32 libretro_get_rom_size(void);

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
      _isDirectory = _isFile = false;
      _size = 0;
      return false;
    }
  }
  return false;
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
    return _path == libretro_rom_path && libretro_get_rom_size() > 0;
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
    if(libretro_vfs->mkdir(_path.c_str()) == 0)
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
size_t FSNodeLIBRETRO::read(ByteArray& image, size_t) const
{
  // If VFS confirms the file is on disk, return 0 so the base class reads
  // it normally via openIFStream
  if(libretro_vfs && libretro_vfs->stat)
  {
    int32_t unused = 0;
    if(libretro_vfs->stat(_path.c_str(), &unused) & RETRO_VFS_STAT_IS_VALID)
      return 0;
  }

  // File not accessible on disk — serve the in-memory ROM buffer.
  // This handles platforms (e.g. Android) where need_fullpath=false means
  // RetroArch loads the ROM into memory but the path isn't directly readable.
  image.resize(Cartridge::maxSize());
  return libretro_read_rom(image.data());
}
