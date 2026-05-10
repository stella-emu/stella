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

#ifdef ZIP_SUPPORT

#include <unordered_set>

#ifdef BSPF_WINDOWS
  #include "HomeFinder.hxx"
#else
  #include "XDGPaths.hxx"
#endif

#include "bspf.hxx"
#include "AsciiFold.hxx"
#include "Bankswitch.hxx"
#include "FSNodeFactory.hxx"
#include "FSNodeZIP.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeZIP::FSNodeZIP(string_view p)
{
  // Extract ZIP file and virtual file (if specified)
  const size_t pos = BSPF::findIgnoreCase(p, ".zip");
  if(pos == string::npos)
    return;

  _zipFile = p.substr(0, pos+4);

  // Create a concrete FSNode to use
  // This *must not* be a ZIP file; it must be a real FSNode object that
  // has direct access to the actual filesystem (aka, a 'System' node)
  // Behind the scenes, this node is actually a platform-specific object
  // for whatever system we are running on
  _realNode = FSNodeFactory::create(_zipFile, FSNodeFactory::Type::SYSTEM);

  // Update _zipFile with the fully resolved path from the real node
  _zipFile = _realNode->getPath();

  // Open file at least once to initialize the virtual file count
  try
  {
    zipHandler()->open(_zipFile);

    // Count ROMs
    _numFiles = zipHandler()->romFiles();
  }
  catch(const ZipException&)
  {
    return;
  }
  if(_numFiles == 0)
    return;

  // We always need a virtual file/path
  // Either one is given, or we use the first one
  if(pos+5 < p.length())  // if something comes after '.zip'
  {
    _virtualPath = p.substr(pos+5);
    _isFile = Bankswitch::isValidRomName(_virtualPath);
    _isDirectory = !_isFile;
  }
  else if(_numFiles == 1)
  {
    try
    {
      if(const auto rom = zipHandler()->firstRom(); rom)
      {
        _virtualPath = rom->first;
        _size = rom->second;
        _isFile = true;
      }
    }
    catch(const ZipException&)
    {
      return;
    }
  }
  else if(_numFiles > 1)
    _isDirectory = true;

  setFlags(_zipFile, _virtualPath, _realNode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeZIP::FSNodeZIP(string_view zipfile, string_view virtualpath,
                     const AbstractFSNodePtr& realnode, size_t size, bool isdir)
  : _size{size},
    _isDirectory{isdir},
    _isFile{!isdir}
{
  setFlags(zipfile, virtualpath, realnode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNodeZIP::setFlags(string_view zipfile, string_view virtualpath,
                         const AbstractFSNodePtr& realnode)
{
  _zipFile = zipfile;
  _virtualPath = virtualpath;
  _realNode = realnode;

  _path = _realNode->getPath();
  _shortPath = _realNode->getShortPath();

  // Is a file component present?
  if(!_virtualPath.empty())
  {
    _path += '/';
    _path += _virtualPath;
    _shortPath += '/';
    _shortPath += _virtualPath;
  }
  _name = AsciiFold::toAscii(lastPathComponent(_path));

  if(!_realNode->isFile())
    throw ZipException(ZipError::FILE_ERROR);
  if(!_realNode->isReadable())
    throw ZipException(ZipError::FILE_ERROR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeZIP::exists() const
{
  if(!_realNode || !_realNode->exists())
    return false;

  // An empty virtual path means the ZIP itself is the node (directory case)
  if(_virtualPath.empty() || _isDirectory)
    return true;

  // We need to inspect the actual path, not just the ZIP file itself
  try
  {
    zipHandler()->open(_zipFile);
    return zipHandler()->find(_virtualPath).second;
  }
  catch(const ZipException&)
  {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeZIP::getChildren(AbstractFSList& myList, ListMode) const
{
  // Files within ZIP archives don't contain children
  if(!isDirectory())
    return false;

  std::unordered_set<string> dirs;

  try
  {
    zipHandler()->open(_zipFile);
    zipHandler()->forEachEntry([&](string_view name, uInt64 size)
    {
      // Ignore empty filenames and '__MACOSX' virtual directories
      if(name.empty() || BSPF::startsWithIgnoreCase(name, "__MACOSX"))
        return;

      string_view remainder = name;

      // Handle virtual path cleanly
      if(!_virtualPath.empty())
      {
        if(!BSPF::startsWithIgnoreCase(name, _virtualPath))
          return;

        const size_t baseLen = _virtualPath.size();

        // Must be exact match or followed by '/'
        if(name.size() == baseLen)
          return;  // directory itself

        if(name[baseLen] != '/' && name[baseLen] != '\\')
          return;

        remainder = name.substr(baseLen + 1);
      }

      if(remainder.empty())
        return;

      // Only add sub-directory entries once
      const auto pos = remainder.find_first_of("/\\");
      if(pos != string_view::npos)
        dirs.emplace(remainder.substr(0, pos));
      else
        myList.emplace_back(new FSNodeZIP(_zipFile, name, _realNode, size, false));
    });
  }
  catch(const ZipException&)
  {
    return false;
  }

  for(const auto& dir: dirs)
  {
    string vpath;
    if(_virtualPath.empty())
      vpath = dir;
    else
    {
      vpath.reserve(_virtualPath.size() + 1 + dir.size());
      vpath = _virtualPath;
      vpath += '/';
      vpath += dir;
    }

    myList.emplace_back(new FSNodeZIP(_zipFile, vpath, _realNode, 0, true));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeZIP::read(ByteArray& buffer, size_t) const
{
  zipHandler()->open(_zipFile);
  return zipHandler()->decompress(_virtualPath, buffer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeZIP::read(std::stringstream& buffer) const
{
  // For now, we just read into a buffer and store in the stream
  // TODO: maybe there's a more efficient way to do this?
  ByteArray read_buf;
  const size_t size = read(read_buf, 0);
  if(size > 0)
    buffer.write(reinterpret_cast<const char*>(read_buf.data()), size);

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodeZIP::getParent() const
{
  if(_virtualPath.empty())
    return _realNode ? _realNode->getParent() : nullptr;

  const auto stem = stemPathComponent(_path);
  // stemPathComponent includes trailing slash; strip it for ZIP constructor
  return std::make_unique<FSNodeZIP>(
    stem.empty() ? stem : stem.substr(0, stem.size() - 1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodeZIP::getSiblingNode(string_view ext) const
{
  // Replace extension of the virtual path component
  const string_view stem = stemPathComponent(_virtualPath);
  const string_view name = lastPathComponent(_virtualPath);
  const size_t dot = name.find_last_of('.');

  string newVirtual;
  newVirtual.reserve(stem.size() + name.size() + ext.size());
  newVirtual = stem;
  if(dot != string_view::npos)
    newVirtual += name.substr(0, dot);
  else
    newVirtual += name;
  newVirtual += ext;

  return AbstractFSNodePtr(new FSNodeZIP(_zipFile, newVirtual, _realNode, 0, false));
}

#endif  // ZIP_SUPPORT
