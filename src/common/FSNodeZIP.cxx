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

#include <set>

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

  // Expand '~' to the users 'home' directory
  if(_zipFile[0] == '~')
  {
#if defined(BSPF_UNIX) || defined(BSPF_MACOS)
    _zipFile.replace(0, 1, XDGPaths::instance().home());
#elif defined(BSPF_WINDOWS)
    _zipFile.replace(0, 1, HomeFinder::getHomePath());
#endif
  }

// cerr << " => p: " << p << '\n';

  // Open file at least once to initialize the virtual file count
  try
  {
    zipHandler()->open(_zipFile);
  }
  catch(const std::runtime_error&)
  {
    // TODO: Actually present the error passed in back to the user
    //       For now, we just indicate that no ROMs were found
    _error = zip_error::NO_ROMS;
  }
  _numFiles = zipHandler()->romFiles();
  if(_numFiles == 0)
  {
    _error = zip_error::NO_ROMS;
  }

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
    bool found = false;
    while(zipHandler()->hasNext() && !found)
    {
      const auto& [name, size] = zipHandler()->next();
      if(Bankswitch::isValidRomName(name))
      {
        _virtualPath = name;
        _size = size;
        _isFile = true;

        found = true;
      }
    }
    if(!found)
      return;
  }
  else if(_numFiles > 1)
    _isDirectory = true;

  // Create a concrete FSNode to use
  // This *must not* be a ZIP file; it must be a real FSNode object that
  // has direct access to the actual filesystem (aka, a 'System' node)
  // Behind the scenes, this node is actually a platform-specific object
  // for whatever system we are running on
  _realNode = FSNodeFactory::create(_zipFile,
      FSNodeFactory::Type::SYSTEM);

  setFlags(_zipFile, _virtualPath, _realNode);
// cerr << "==============================================================\n";
// cerr << _name << ", file: " << _isFile << ", dir: " << _isDirectory << "\n\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeZIP::FSNodeZIP(const string& zipfile, const string& virtualpath,
                     const AbstractFSNodePtr& realnode, size_t size, bool isdir)
  : _size{size},
    _isDirectory{isdir},
    _isFile{!isdir}
{
// cerr << "=> c'tor 2: " << zipfile << ", " << virtualpath << ", " << isdir << '\n';
  setFlags(zipfile, virtualpath, realnode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNodeZIP::setFlags(const string& zipfile, const string& virtualpath,
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
    _path += ("/" + _virtualPath);
    _shortPath += ("/" + _virtualPath);
  }
  _name = AsciiFold::toAscii(lastPathComponent(_path));

  if(!_realNode->isFile())
    _error = zip_error::NOT_A_FILE;
  if(!_realNode->isReadable())
    _error = zip_error::NOT_READABLE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeZIP::exists() const
{
  if(_realNode && _realNode->exists())
  {
    // We need to inspect the actual path, not just the ZIP file itself
    try
    {
      zipHandler()->open(_zipFile);
      while(zipHandler()->hasNext())
      {
        const auto& [name, size] = zipHandler()->next();
        if(BSPF::startsWithIgnoreCase(name, _virtualPath))
          return true;
      }
    }
    catch(const std::runtime_error&)
    {
      // TODO: Actually present the error passed in back to the user
      cerr << "ERROR: FSNodeZIP::exists()\n";
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeZIP::getChildren(AbstractFSList& myList, ListMode mode) const
{
  // Files within ZIP archives don't contain children
  if(!isDirectory() || _error != zip_error::NONE)
    return false;

  std::set<string> dirs;
  zipHandler()->open(_zipFile);
  while(zipHandler()->hasNext())
  {
    // Only consider entries that start with '_virtualPath'
    // Ignore empty filenames and '__MACOSX' virtual directories
    const auto& [name, size] = zipHandler()->next();
    if(BSPF::startsWithIgnoreCase(name, "__MACOSX") || name == EmptyString())
      continue;
    if(BSPF::startsWithIgnoreCase(name, _virtualPath))
    {
      // First strip off the leading directory
      const string& curr = name.substr(
          _virtualPath.empty() ? 0 : _virtualPath.size()+1);

      // Only add sub-directory entries once
      const auto pos = curr.find_first_of("/\\");
      if(pos != string::npos)
        dirs.emplace(curr.substr(0, pos));
      else
        myList.emplace_back(new FSNodeZIP(_zipFile, name, _realNode, size, false));
    }
  }
  for(const auto& dir: dirs)
  {
    // Prepend previous path
    const string& vpath = !_virtualPath.empty() ? _virtualPath + "/" + dir : dir;
    myList.emplace_back(new FSNodeZIP(_zipFile, vpath, _realNode, 0, true));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeZIP::read(ByteBuffer& buffer, size_t) const
{
  switch(_error)
  {
    using enum zip_error;
    case NONE:         break;
    case NOT_A_FILE:   throw std::runtime_error("ZIP file contains errors/not found");
    case NOT_READABLE: throw std::runtime_error("ZIP file not readable");
    case NO_ROMS:      throw std::runtime_error("ZIP file doesn't contain any ROMs");
    default: throw std::runtime_error("FSNodeZIP::read default case hit");
  }

  zipHandler()->open(_zipFile);

  bool found = false;
  while(zipHandler()->hasNext() && !found)
  {
    const auto& [name, size] = zipHandler()->next();
    found = name == _virtualPath;
  }

  return found ? zipHandler()->decompress(buffer) : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeZIP::read(std::stringstream& buffer) const
{
  // For now, we just read into a buffer and store in the stream
  // TODO: maybe there's a more efficient way to do this?
  ByteBuffer read_buf;
  const size_t size = read(read_buf, 0);
  if(size > 0)
    buffer.write(reinterpret_cast<char*>(read_buf.get()), size);

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodeZIP::getParent() const
{
  if(_virtualPath.empty())
    return _realNode ? _realNode->getParent() : nullptr;

  // TODO: For some reason, getting the stem for normal paths and zip paths
  //       behaves differently.  For now, we'll use the old method here.
  auto STEM_FOR_ZIP = [](string_view s) {
    const char* const start = s.data();
    const char* cur = start + s.size() - 2;

    while (cur >= start && !(*cur == '/' || *cur == '\\'))
      --cur;

    return s.substr(0, (cur + 1) - start - 1);
  };

  return std::make_unique<FSNodeZIP>(STEM_FOR_ZIP(_path));
}

#endif  // ZIP_SUPPORT
