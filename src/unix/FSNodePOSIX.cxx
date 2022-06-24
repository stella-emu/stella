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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#if defined(RETRON77)
  #define ROOT_DIR "/mnt/games/"
#else
  #define ROOT_DIR "/"
#endif

#include "FSNodePOSIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodePOSIX::FSNodePOSIX()
  : _path{ROOT_DIR},
    _displayName{_path}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodePOSIX::FSNodePOSIX(const string& path, bool verify)
  : _path{path.length() > 0 ? path : "~"}  // Default to home directory
{
  // Expand '~' to the HOME environment variable
  if(_path[0] == '~')
  {
  #if defined(BSPF_WINDOWS)

  #else
    const char* home = std::getenv("HOME");
  #endif
    if (home != nullptr)
      _path.replace(0, 1, home);
  }
  // Get absolute path (only used for relative directories)
  else if(_path[0] == '.')
  {
    std::array<char, MAXPATHLEN> buf;
    if(realpath(_path.c_str(), buf.data()))
      _path = buf.data();
  }

  _fspath = _path;
  _path = _fspath.string();
  _displayName = lastPathComponent(_path);

  if(verify)
    setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNodePOSIX::setFlags()
{
  std::error_code ec;
  const auto s = fs::status(_fspath, ec);
  if (!ec)
  {
    const auto p = s.permissions();

    _isFile      = fs::is_regular_file(s);
    _isDirectory = fs::is_directory(s);
    _isReadable  = (p & (fs::perms::owner_read |
                         fs::perms::group_read |
                         fs::perms::others_read)) != fs::perms::none;
    _isWriteable = (p & (fs::perms::owner_write |
                         fs::perms::group_write |
                         fs::perms::others_write)) != fs::perms::none;
    _size = _isFile ? fs::file_size(_fspath) : 0;
  }
  else
  {
    _isFile = _isDirectory = _isReadable = _isWriteable = false;
    _size = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNodePOSIX::getShortPath() const
{
  // If the path starts with the home directory, replace it with '~'
#if defined(BSPF_WINDOWS)

#else
  const char* env_home = std::getenv("HOME");
#endif
  const string& home = env_home != nullptr ? env_home : EmptyString;

  if(home != EmptyString && BSPF::startsWithIgnoreCase(_path, home))
  {
    string path = "~";
    const char* offset = _path.c_str() + home.size();
    if(*offset != '/') path += "/";
    path += offset;
    return path;
  }
  return _path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::hasParent() const
{
  return _path != "" && _path != ROOT_DIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::getChildren(AbstractFSList& myList, ListMode mode) const
{
  std::error_code ec;
  for (const auto& entry: fs::directory_iterator{_fspath,
         fs::directory_options::follow_directory_symlink |
         fs::directory_options::skip_permission_denied,
         ec
        })
  {
    const auto& path = entry.path();

    // Ignore files with errors, or any that start with '.'
    if (ec || path.filename().string()[0] == '.')
      continue;

    // Honor the chosen mode
    const bool isFile = entry.is_regular_file(),
               isDir  = entry.is_directory();
    if ((mode == FSNode::ListMode::FilesOnly && !isFile) ||
        (mode == FSNode::ListMode::DirectoriesOnly && !isDir))
      continue;

    // Only create the object and add it to the list when absolutely
    // necessary
    FSNodePOSIX node(path.string(), false);
    node._isFile      = isFile;
    node._isDirectory = isDir;
    node._size = isFile ? entry.file_size() : 0;

    const auto p = entry.status().permissions();
    node._isReadable  = (p & (fs::perms::owner_read |
                              fs::perms::group_read |
                              fs::perms::others_read)) != fs::perms::none;
    node._isWriteable = (p & (fs::perms::owner_write |
                              fs::perms::group_write |
                              fs::perms::others_write)) != fs::perms::none;

    myList.emplace_back(make_shared<FSNodePOSIX>(node));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodePOSIX::read(ByteBuffer& buffer, size_t size) const
{
  std::ifstream in(_fspath, std::ios::binary);
  if (in)
  {
    size_t sizeRead = fs::file_size(_fspath);
    if (sizeRead == 0)
      throw runtime_error("Zero-byte file");
    else if (size > 0)  // If a requested size to read is provided, honour it
      sizeRead = std::min(sizeRead, size);

    buffer = make_unique<uInt8[]>(sizeRead);
    in.read(reinterpret_cast<char*>(buffer.get()), sizeRead);
    return sizeRead;
  }
  else
    throw runtime_error("File open/read error");

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodePOSIX::read(stringstream& buffer) const
{
  std::ifstream in(_fspath);
  if (in)
  {
    size_t sizeRead = fs::file_size(_fspath);
    if (sizeRead == 0)
      throw runtime_error("Zero-byte file");

    buffer << in.rdbuf();
    return sizeRead;
  }
  else
    throw runtime_error("File open/read error");

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodePOSIX::write(const ByteBuffer& buffer, size_t size) const
{
  std::ofstream out(_fspath, std::ios::binary);
  if (out)
  {
    out.write(reinterpret_cast<const char*>(buffer.get()), size);
    out.close();
    return fs::file_size(_fspath);
  }
  else
    throw runtime_error("File open/write error");

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodePOSIX::write(const stringstream& buffer) const
{
  std::ofstream out(_fspath);
  if (out)
  {
    out << buffer.rdbuf();
    out.close();
    return fs::file_size(_fspath);
  }
  else
    throw runtime_error("File open/write error");

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::makeDir()
{
  if(mkdir(_path.c_str(), 0777) == 0)
  {
    // Get absolute path
    std::array<char, MAXPATHLEN> buf;
    if(realpath(_path.c_str(), buf.data()))
      _path = buf.data();

    _displayName = lastPathComponent(_path);
    setFlags();

    // Add a trailing slash, if necessary
    if (_path.length() > 0 && _path[_path.length()-1] != '/')
      _path += '/';

    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::rename(const string& newfile)
{
  if(std::rename(_path.c_str(), newfile.c_str()) == 0)
  {
    _path = newfile;

    // Get absolute path
    std::array<char, MAXPATHLEN> buf;
    if(realpath(_path.c_str(), buf.data()))
      _path = buf.data();

    _displayName = lastPathComponent(_path);
    setFlags();

    // Add a trailing slash, if necessary
    if (_isDirectory && _path.length() > 0 && _path[_path.length()-1] != '/')
      _path += '/';

    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodePOSIX::getParent() const
{
  if (_path == ROOT_DIR)
    return nullptr;

  const char* start = _path.c_str();
  const char* end = lastPathComponent(_path);

  return make_unique<FSNodePOSIX>(string(start, size_t(end - start)));
}
