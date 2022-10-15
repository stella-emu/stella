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

#include <cassert>
#pragma warning(disable : 4091)
#include <shlobj.h>
#include <io.h>
#include <stdio.h>
#include <tchar.h>

#include "Windows.hxx"
#include "FSNodeWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNodeWINDOWS::FSNodeWINDOWS(const string& p)
  : _path{p.length() > 0 ? p : "~"}  // Default to home directory
{
  // Expand '~' to the users 'home' directory
  if (_path[0] == '~')
    _path.replace(0, 1, HomeFinder::getHomePath());

  setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::setFlags()
{
  // Get absolute path
  TCHAR buf[MAX_PATH];
  if (GetFullPathName(_path.c_str(), MAX_PATH - 1, buf, NULL))
    _path = buf;

  _displayName = lastPathComponent(_path);

  // Check whether it is a directory, and whether the file actually exists
  const DWORD fileAttribs = GetFileAttributes(_path.c_str());

  if (fileAttribs == INVALID_FILE_ATTRIBUTES)
  {
    _isDirectory = _isFile = false;
    return false;
  }
  else
  {
    _isDirectory = ((fileAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0);
    _isFile = !_isDirectory;//((fileAttribs & FILE_ATTRIBUTE_NORMAL) != 0);

    // Add a trailing backslash, if necessary
    if (_isDirectory && _path.length() > 0 &&
        _path[_path.length()-1] != FSNode::PATH_SEPARATOR)
      _path += FSNode::PATH_SEPARATOR;
  }
  _isPseudoRoot = false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNodeWINDOWS::getShortPath() const
{
  // If the path starts with the home directory, replace it with '~'
  const string& home = HomeFinder::getHomePath();
  if (home != "" && BSPF::startsWithIgnoreCase(_path, home))
  {
    string path = "~";
    const char* offset = _path.c_str() + home.length();
    if (*offset != FSNode::PATH_SEPARATOR)
      path += FSNode::PATH_SEPARATOR;
    path += offset;
    return path;
  }
  return _path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNodeWINDOWS::getSize() const
{
  if (_size == 0)
  {
    struct _stat st;
    _size = _stat(_path.c_str(), &st) == 0 ? st.st_size : 0;
  }
  return _size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FSNodeWINDOWS::getParent() const
{
  if (_isPseudoRoot)
    return nullptr;

  if (_path.size() > 3)
  {
    const char* start = _path.c_str();
    const char* end = lastPathComponent(_path);

    return make_shared<FSNodeWINDOWS>(string(start, static_cast<size_t>(end - start)));
  }
  else
    return make_shared<FSNodeWINDOWS>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::getChildren(AbstractFSList& myList, ListMode mode) const
{
  if (_isPseudoRoot)
  {
    // Drives enumeration
    TCHAR drive_buffer[100];
    GetLogicalDriveStrings(sizeof(drive_buffer) / sizeof(TCHAR), drive_buffer);

    for (TCHAR *current_drive = drive_buffer; *current_drive;
         current_drive += _tcslen(current_drive) + 1)
    {
      FSNodeWINDOWS entry;
      char drive_name[2] = { 0, 0 };

      drive_name[0] = current_drive[0];
      drive_name[1] = '\0';
      entry._displayName = drive_name;
      entry._isDirectory = true;
      entry._isFile = false;
      entry._isPseudoRoot = false;
      entry._path = current_drive;
      myList.emplace_back(make_shared<FSNodeWINDOWS>(entry));
    }
  }
  else
  {
    // Files enumeration
    WIN32_FIND_DATA desc;
    HANDLE handle = FindFirstFile((_path + "*").c_str(), &desc);
    if (handle == INVALID_HANDLE_VALUE)
      return false;

    addFile(myList, mode, _path, desc);

    while (FindNextFile(handle, &desc))
      addFile(myList, mode, _path, desc);

    FindClose(handle);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNodeWINDOWS::addFile(AbstractFSList& list, ListMode mode,
                            const string& base, const WIN32_FIND_DATA& find_data)
{
  const char* const asciiName = find_data.cFileName;
  bool isDirectory = false, isFile = false;

  // Skip local directory (.) and parent (..)
  if (!strncmp(asciiName, ".", 1) || !strncmp(asciiName, "..", 2))
    return;

  isDirectory = static_cast<bool>(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  isFile = !isDirectory;//(find_data->dwFileAttributes & FILE_ATTRIBUTE_NORMAL ? true : false);

  if ((isFile && mode == FSNode::ListMode::DirectoriesOnly) ||
      (isDirectory && mode == FSNode::ListMode::FilesOnly))
    return;

  FSNodeWINDOWS entry;
  entry._isDirectory = isDirectory;
  entry._isFile = isFile;
  entry._displayName = asciiName;
  entry._path = base;
  entry._path += asciiName;
  if (entry._isDirectory)
    entry._path += FSNode::PATH_SEPARATOR;
  entry._isPseudoRoot = false;

  list.emplace_back(make_shared<FSNodeWINDOWS>(entry));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::exists() const
{
  return _access(_path.c_str(), 0 /*F_OK*/) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::isReadable() const
{
  return _access(_path.c_str(), 4 /*R_OK*/) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::isWritable() const
{
  return _access(_path.c_str(), 2 /*W_OK*/) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::makeDir()
{
  if (!_isPseudoRoot && CreateDirectory(_path.c_str(), NULL) != 0)
    return setFlags();

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNodeWINDOWS::rename(const string& newfile)
{
  if (!_isPseudoRoot && MoveFile(_path.c_str(), newfile.c_str()) != 0)
    return setFlags();

  return false;
}
