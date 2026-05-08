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

#include "AsciiFold.hxx"
#include "XDGPaths.hxx"
#include "FSNodePOSIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodePOSIX::FSNodePOSIX()
  : _path{"/"},
    _displayName{AsciiFold::toAscii(_path)}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodePOSIX::FSNodePOSIX(string_view path, bool verify)
  : _path{!path.empty() ? path : "~"}  // Default to home directory
{
  // Expand '~' to the HOME environment variable
  if(_path[0] == '~')
  {
    // Skip the '/' after '~' if present to avoid double slash
    const size_t replaceLen = (_path.size() > 1 && _path[1] == '/') ? 2 : 1;
    _path.replace(0, replaceLen, XDGPaths::instance().home());
  }
  else if(_path[0] == '.')
  {
    if(auto resolved = std::unique_ptr<char, decltype(&free)>
        (realpath(_path.c_str(), nullptr), free))
      _path = resolved.get();
  }

  _displayName = AsciiFold::toAscii(lastPathComponent(_path));

  if(verify)
    setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::setFlags()
{
  struct stat st{};
  if(stat(_path.c_str(), &st) == 0)
  {
    _isDirectory = S_ISDIR(st.st_mode);
    _isFile = S_ISREG(st.st_mode);
    _size = st.st_size;

    // Add a trailing slash, if necessary
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNodePOSIX::getShortPath() const
{
  // If the path starts with the home directory, replace it with '~'
  const string& home = XDGPaths::instance().home();

  if(BSPF::startsWithIgnoreCase(_path, home))
  {
    string path = "~";
    const char* const offset = _path.c_str() + home.size();
    if(*offset != FSNode::PATH_SEPARATOR)
      path += FSNode::PATH_SEPARATOR;
    path += offset;
    return path;
  }
  return _path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodePOSIX::getSize() const
{
  if(_size == 0 && _isFile)
  {
    struct stat st{};
    _size = (stat(_path.c_str(), &st) == 0) ? st.st_size : 0;
  }
  return _size.value_or(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::hasParent() const
{
  return !_path.empty() && _path != "/";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodePOSIX::getParent() const
{
  if(_path == "/")
    return nullptr;

  return std::make_unique<FSNodePOSIX>(stemPathComponent(_path));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::getChildren(AbstractFSList& myList, ListMode mode) const
{
  if(!_isDirectory)
    return false;

  DIR* dirp = opendir(_path.c_str());
  if(dirp == nullptr)
    return false;

  // Loop over dir entries using readdir
  struct dirent* dp = nullptr;
  while((dp = readdir(dirp)) != nullptr)  // NOLINT(concurrency-mt-unsafe)
  {
    // Ignore all hidden files
    if(dp->d_name[0] == '.')
      continue;

    string newPath(_path);
    if(!newPath.empty() && newPath.back() != FSNode::PATH_SEPARATOR)
      newPath += FSNode::PATH_SEPARATOR;
    newPath += dp->d_name;

    FSNodePOSIX entry(newPath, false);
    bool valid = true;

    if(dp->d_type == DT_UNKNOWN || dp->d_type == DT_LNK)
    {
      // Fall back to stat()
      valid = entry.setFlags();
    }
    else
    {
      entry._isDirectory = (dp->d_type == DT_DIR);
      entry._isFile = (dp->d_type == DT_REG);
      // entry._size will be calculated first time ::getSize() is called

      if(entry._isDirectory)
        entry._path += FSNode::PATH_SEPARATOR;
    }

    // Skip files that are invalid for some reason (e.g. because we couldn't
    // properly stat them).
    if(!valid)
      continue;

    // Honor the chosen mode
    if((mode == FSNode::ListMode::FilesOnly && !entry._isFile) ||
       (mode == FSNode::ListMode::DirectoriesOnly && !entry._isDirectory))
      continue;

    myList.emplace_back(std::make_unique<FSNodePOSIX>(std::move(entry)));
  }
  closedir(dirp);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::makeDir()
{
  if(mkdir(_path.c_str(), 0777) == 0)
  {
    if(auto resolved = std::unique_ptr<char, decltype(&free)>
        (realpath(_path.c_str(), nullptr), free))
      _path = resolved.get();

    _displayName = AsciiFold::toAscii(lastPathComponent(_path));
    return setFlags();
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodePOSIX::rename(string_view newfile)
{
  string newPath{newfile};
  if(std::rename(_path.c_str(), newPath.c_str()) == 0)
  {
    _path = std::move(newPath);

    if(auto resolved = std::unique_ptr<char, decltype(&free)>
        (realpath(_path.c_str(), nullptr), free))
      _path = resolved.get();

    _displayName = AsciiFold::toAscii(lastPathComponent(_path));
    return setFlags();
  }
  return false;
}
