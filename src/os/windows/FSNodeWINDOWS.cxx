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

#include <array>
#include <io.h>

#include "HomeFinder.hxx"
#include "FSNodeWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeWINDOWS::FSNodeWINDOWS(std::string_view path)
{
  if(path.empty() || path == "~")
  {
    _pathW = HomeFinder::getHomePathW();
  }
  else
  {
    _pathW = utf8ToWide(path);

    if(!_pathW.empty() && _pathW[0] == L'~')
      _pathW.replace(0, 1, HomeFinder::getHomePathW());
  }

  setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeWINDOWS::FSNodeWINDOWS(std::wstring_view path)
{
  if(path.empty() || path == L"~")
  {
    _pathW = HomeFinder::getHomePathW();
  }
  else
  {
    _pathW = path;

    if(!_pathW.empty() && _pathW[0] == L'~')
      _pathW.replace(0, 1, HomeFinder::getHomePathW());
  }

  setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::setFlags()
{
  // Get absolute path
  std::array<wchar_t, MAX_PATH> buf{};
  if(GetFullPathNameW(_pathW.c_str(), MAX_PATH - 1, buf.data(), nullptr))
    _pathW = buf.data();

  const DWORD fileAttribs = GetFileAttributesW(_pathW.c_str());

  if(fileAttribs == INVALID_FILE_ATTRIBUTES)
  {
    _isDirectory = _isFile = false;
    return false;
  }

  // Check whether it is a directory, and whether the file actually exists
  _isDirectory = (fileAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
  _isFile = !_isDirectory;

  // Add a trailing backslash, if necessary
  if(_isDirectory && !_pathW.empty() && _pathW.back() != L'\\')
    _pathW += L'\\';

  // Only after making sure that there's a trailing backslash
  // will we update the UTF8 paths
  _pathUtf8 = wideToUtf8(_pathW);
  _displayName = lastPathComponent(_pathUtf8);

  _isPseudoRoot = false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNodeWINDOWS::getShortPath() const
{
  // If the path starts with the home directory, replace it with '~'
  const std::wstring& home = HomeFinder::getHomePathW();
  if(!home.empty() && _wcsnicmp(_pathW.data(), home.data(), home.size()) == 0)
  {
    const wchar_t* const offset = _pathW.c_str() + home.size();
    const bool needsSeparator = (*offset != L'\0' &&
      *offset != FSNode::PATH_SEPARATOR);

    std::wstring pathW;
    pathW.reserve(1 + (needsSeparator ? 1 : 0) + (_pathW.size() - home.size()));

    pathW.push_back(L'~');
    if(needsSeparator)
      pathW.push_back(FSNode::PATH_SEPARATOR);

    pathW.append(offset);

    return wideToUtf8(pathW);
  }

  return wideToUtf8(_pathW);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeWINDOWS::getSize() const
{
  if(!_size && _isFile)
  {
    struct _stat st;
    _size = (_wstat(_pathW.c_str(), &st) == 0) ? st.st_size : 0;
  }
  return _size.value_or(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodeWINDOWS::getParent() const
{
  if(_isPseudoRoot)
    return nullptr;
  else if(_pathW.size() > 3)
    return std::make_unique<FSNodeWINDOWS>(stemPathComponentW(_pathW));
  else
    return std::make_unique<FSNodeWINDOWS>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::getChildren(AbstractFSList& fslist, ListMode mode) const
{
  if(!_isPseudoRoot && _isDirectory) [[likely]]
  {
    std::wstring search = _pathW;
    if(!search.empty() && search.back() != L'\\')
      search += L'\\';
    search += L'*';

    WIN32_FIND_DATAW desc;
    HANDLE handle = FindFirstFileExW(search.c_str(), FindExInfoBasic, &desc,
      FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if(handle == INVALID_HANDLE_VALUE)
      return false;

    do {
      // Skip files starting with '.' (we assume empty filenames never occur)
      if(desc.cFileName[0] == L'.')
        continue;

      const bool isDir = (desc.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      const bool isFile = !isDir;

      if((isFile && mode == FSNode::ListMode::DirectoriesOnly) ||
         (isDir && mode == FSNode::ListMode::FilesOnly))
        continue;

      std::wstring full = _pathW;
      if(!full.empty() && full.back() != L'\\')
        full += L'\\';
      full += desc.cFileName;

      FSNodeWINDOWS entry;
      entry._isPseudoRoot = false;
      entry._isDirectory  = isDir;
      entry._isFile       = isFile;
      entry._pathW        = std::move(full);
      entry._pathUtf8     = wideToUtf8(entry._pathW);
      entry._displayName  = lastPathComponent(entry._pathUtf8);
      entry._size         = (static_cast<size_t>(desc.nFileSizeHigh) << 32) |
                                                 desc.nFileSizeLow;

      fslist.emplace_back(std::make_unique<FSNodeWINDOWS>(std::move(entry)));

    } while(FindNextFileW(handle, &desc));

    FindClose(handle);
  }
  else
  {
    // Drives enumeration
    const DWORD driveMask = GetLogicalDrives();

    for(int i = 0; i < 26; ++i)
    {
      if(!(driveMask & (1 << i)))
        continue;

      FSNodeWINDOWS entry;
      entry._isPseudoRoot = false;
      entry._isDirectory  = true;
      entry._isFile       = false;
      entry._pathW        = { static_cast<wchar_t>(L'A' + i), L':', L'\\' };
      entry._pathUtf8     = wideToUtf8(entry._pathW);
      entry._displayName  = string(1, static_cast<char>('A' + i));

      fslist.emplace_back(std::make_unique<FSNodeWINDOWS>(std::move(entry)));
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::exists() const
{
  return _waccess(_pathW.c_str(), 0 /*F_OK*/) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::isReadable() const
{
  return _waccess(_pathW.c_str(), 4 /*R_OK*/) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::isWritable() const
{
  return _waccess(_pathW.c_str(), 2 /*W_OK*/) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::makeDir()
{
  if(!_isPseudoRoot && CreateDirectoryW(_pathW.c_str(), nullptr) != 0)
    return setFlags();

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::rename(string_view newfile)
{
  if(!_isPseudoRoot)
  {
    std::wstring newfilePath = utf8ToWide(newfile);
    if(MoveFileW(_pathW.c_str(), newfilePath.c_str()) != 0) {
      _pathW = std::move(newfilePath);
      _size.reset();
      if(_pathW[0] == '~')
        _pathW.replace(0, 1, HomeFinder::getHomePathW());

      return setFlags();
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::wstring FSNodeWINDOWS::utf8ToWide(std::string_view s)
{
  if(s.empty())
    return {};

  const int size = MultiByteToWideChar(
    CP_UTF8, 0,
    s.data(), static_cast<int>(s.size()),
    nullptr, 0);

  std::wstring w(size, L'\0');

  MultiByteToWideChar(
    CP_UTF8, 0,
    s.data(), static_cast<int>(s.size()),
    w.data(), size);

  return w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string FSNodeWINDOWS::wideToUtf8(const std::wstring_view w)
{
  if(w.empty())
    return {};

  const int needed = WideCharToMultiByte(
    CP_UTF8, 0,
    w.data(), static_cast<int>(w.size()),
    nullptr, 0, nullptr, nullptr);

  std::string out(needed, '\0');

  WideCharToMultiByte(
    CP_UTF8, 0,
    w.data(), static_cast<int>(w.size()),
    out.data(), needed,
    nullptr, nullptr);

  return out;
}
