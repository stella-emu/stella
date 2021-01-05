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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Cart.hxx"
#include "FSNodeLIBRETRO.hxx"

#ifdef _WIN32
  const string slash = "\\";
#else
  const string slash = "/";
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeLIBRETRO::FilesystemNodeLIBRETRO()
  : _name{"rom"},
    _path{"." + slash}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeLIBRETRO::FilesystemNodeLIBRETRO(const string& p)
  : _name{p},
    _path{p}
{
  // TODO: use retro_vfs_mkdir_t (file) or RETRO_MEMORY_SAVE_RAM (stream) or libretro save path
  if(p == "." + slash + "nvram")
    _path = "." + slash;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeLIBRETRO::exists() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeLIBRETRO::isReadable() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeLIBRETRO::isWritable() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNodeLIBRETRO::getShortPath() const
{
  return ".";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeLIBRETRO::
    getChildren(AbstractFSList& myList, ListMode mode) const
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeLIBRETRO::makeDir()
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeLIBRETRO::rename(const string& newfile)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FilesystemNodeLIBRETRO::getParent() const
{
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FilesystemNodeLIBRETRO::read(ByteBuffer& image) const
{
  image = make_unique<uInt8[]>(Cartridge::maxSize());

  extern uInt32 libretro_read_rom(void* data);
  return libretro_read_rom(image.get());
}
