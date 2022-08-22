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

#if defined(BSPF_UNIX) || defined(BSPF_MACOS)
  #if defined(RETRON77)
    #define ROOT_DIR "/mnt/games/"
  #else
    #define ROOT_DIR "/"
  #endif
#elif defined(BSPF_WINDOWS)
  #define ROOT_DIR "C:\\"
#else
  #define ROOT_DIR ""
#endif

#include "FSNodeREGULAR.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeREGULAR::FSNodeREGULAR()
  : _path{ROOT_DIR},
    _displayName{_path}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeREGULAR::FSNodeREGULAR(const string& path, bool verify)
  : _path{path.length() > 0 ? path : "~"}  // Default to home directory
{
  // Expand '~' to the HOME environment variable
  if (_path[0] == '~')
  {
    const char* home = std::getenv("HOME");  // NOLINT (not thread safe)
    if (home != nullptr)
      _path.replace(0, 1, home);
  }
  _fspath = _path;

  if (fs::exists(_fspath))  // get absolute path whenever possible
    _fspath = fs::canonical(_fspath);

  if (verify)
    setFlags();

  setPathAndDisplayNames();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNodeREGULAR::setFlags()
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
string FSNodeREGULAR::getShortPath() const
{
  // If the path starts with the home directory, replace it with '~'
  const char* env_home = std::getenv("HOME");  // NOLINT (not thread safe)
  const string& home = env_home != nullptr ? env_home : EmptyString;

  if (home != EmptyString && BSPF::startsWithIgnoreCase(_path, home))
  {
    string path = "~";
    const char* offset = _path.c_str() + home.size();
    if(*offset != FSNode::PATH_SEPARATOR)
      path += FSNode::PATH_SEPARATOR;
    path += offset;
    return path;
  }
  return _path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeREGULAR::hasParent() const
{
  return !_path.empty() && _path != ROOT_DIR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodeREGULAR::getParent() const
{
  if (_path == ROOT_DIR)
    return nullptr;

  const char* start = _path.c_str();
  const char* end = lastPathComponent(_path);
// cerr << " => path: " << _path << ", new: " << string(start, size_t(end - start)) << endl;
  return make_unique<FSNodeREGULAR>(string(start, static_cast<size_t>(end - start)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeREGULAR::getChildren(AbstractFSList& myList, ListMode mode) const
{
  if (!_isDirectory)
    return false;

  std::error_code ec;
  const auto di = fs::directory_iterator{_fspath, ec};
  const auto di_end = fs::end(di);
  for (auto it = fs::begin(di); it != di_end; ++it)
  {
    const auto& path = it->path();

    // Ignore files with errors, or any that start with '.'
    if (ec || path.filename().string()[0] == '.')
      continue;

    // Honor the chosen mode
    const bool isFile = it->is_regular_file(),
               isDir  = it->is_directory();
    if ((mode == FSNode::ListMode::FilesOnly && !isFile) ||
        (mode == FSNode::ListMode::DirectoriesOnly && !isDir))
      continue;

    // Only create the object and add it to the list when absolutely
    // necessary
    FSNodeREGULAR node(path.string(), false);
    node._isFile      = isFile;
    node._isDirectory = isDir;
    node._size = isFile ? it->file_size() : 0;

    const auto p = it->status().permissions();
    node._isReadable  = (p & (fs::perms::owner_read |
                              fs::perms::group_read |
                              fs::perms::others_read)) != fs::perms::none;
    node._isWriteable = (p & (fs::perms::owner_write |
                              fs::perms::group_write |
                              fs::perms::others_write)) != fs::perms::none;

    myList.emplace_back(make_shared<FSNodeREGULAR>(node));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeREGULAR::read(ByteBuffer& buffer, size_t size) const
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeREGULAR::read(stringstream& buffer) const
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeREGULAR::write(const ByteBuffer& buffer, size_t size) const
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeREGULAR::write(const stringstream& buffer) const
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeREGULAR::makeDir()
{
  if (!(exists() && _isDirectory) && fs::create_directory(_fspath))
  {
    _fspath = fs::canonical(_fspath);

    setFlags();
    setPathAndDisplayNames();

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeREGULAR::rename(const string& newfile)
{
  fs::path newpath = newfile;

  std::error_code ec;
  fs::rename(_fspath, newpath, ec);
  if (!ec)
  {
    _fspath = fs::canonical(newpath);

    setFlags();
    setPathAndDisplayNames();

    return true;
  }
  return false;
}
