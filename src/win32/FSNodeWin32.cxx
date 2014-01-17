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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <shlobj.h>

#ifdef ARRAYSIZE
  #undef ARRAYSIZE
#endif
#ifdef _WIN32_WCE
  #include <windows.h>
  // winnt.h defines ARRAYSIZE, but we want our own one...
  #undef ARRAYSIZE
  #undef GetCurrentDirectory
#endif

#include <io.h>
#include <stdio.h>
#ifndef _WIN32_WCE
  #include <windows.h>
  // winnt.h defines ARRAYSIZE, but we want our own one...
  #undef ARRAYSIZE
#endif

// F_OK, R_OK and W_OK are not defined under MSVC, so we define them here
// For more information on the modes used by MSVC, check:
// http://msdn2.microsoft.com/en-us/library/1w06ktdy(VS.80).aspx
#ifndef F_OK
  #define F_OK 0
#endif

#ifndef R_OK
  #define R_OK 4
#endif

#ifndef W_OK
  #define W_OK 2
#endif

#include "FSNodeWin32.hxx"

/**
 * Returns the last component of a given path.
 *
 * Examples:
 *			c:\foo\bar.txt would return "\bar.txt"
 *			c:\foo\bar\    would return "\bar\"
 *
 * @param str Path to obtain the last component from.
 * @return Pointer to the first char of the last component inside str.
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* lastPathComponent(const string& str)
{
  if(str.empty())
    return "";

  const char *start = str.c_str();
  const char *cur = start + str.size() - 2;

  while (cur >= start && *cur != '\\')
    --cur;

  return cur + 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeWin32::exists() const
{
  return _access(_path.c_str(), F_OK) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeWin32::isReadable() const
{
  return _access(_path.c_str(), R_OK) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeWin32::isWritable() const
{
  return _access(_path.c_str(), W_OK) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilesystemNodeWin32::setFlags()
{
  // Get absolute path
  TCHAR buf[4096];
  if(GetFullPathName(_path.c_str(), 4096, buf, NULL))
    _path = buf;

  _displayName = lastPathComponent(_path);

  // Check whether it is a directory, and whether the file actually exists
  DWORD fileAttribs = GetFileAttributes(toUnicode(_path.c_str()));

  if (fileAttribs == INVALID_FILE_ATTRIBUTES)
  {
    _isDirectory = _isFile = _isValid = false;
  }
  else
  {
    _isDirectory = ((fileAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0);
    _isFile = !_isDirectory;//((fileAttribs & FILE_ATTRIBUTE_NORMAL) != 0);
    _isValid = true;

    // Add a trailing backslash, if necessary
    if (_isDirectory && _path.length() > 0 && _path[_path.length()-1] != '\\')
      _path += '\\';
  }
  _isPseudoRoot = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilesystemNodeWin32::addFile(AbstractFSList& list, ListMode mode,
                                    const char* base, WIN32_FIND_DATA* find_data)
{
  FilesystemNodeWin32 entry;
  char* asciiName = toAscii(find_data->cFileName);
  bool isDirectory, isFile;

  // Skip local directory (.) and parent (..)
  if (!strncmp(asciiName, ".", 1) || !strncmp(asciiName, "..", 2))
    return;

  isDirectory = (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? true : false);
  isFile = !isDirectory;//(find_data->dwFileAttributes & FILE_ATTRIBUTE_NORMAL ? true : false);

  if ((isFile && mode == FilesystemNode::kListDirectoriesOnly) ||
      (isDirectory && mode == FilesystemNode::kListFilesOnly))
    return;

  entry._isDirectory = isDirectory;
  entry._isFile = isFile;
  entry._displayName = asciiName;
  entry._path = base;
  entry._path += asciiName;
  if (entry._isDirectory)
    entry._path += "\\";
  entry._isValid = true;
  entry._isPseudoRoot = false;

  list.push_back(new FilesystemNodeWin32(entry));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* FilesystemNodeWin32::toAscii(TCHAR* str)
{
#ifndef UNICODE
  return (char*)str;
#else
  static char asciiString[MAX_PATH];
  WideCharToMultiByte(CP_ACP, 0, str, _tcslen(str) + 1, asciiString, sizeof(asciiString), NULL, NULL);
  return asciiString;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const TCHAR* FilesystemNodeWin32::toUnicode(const char* str)
{
#ifndef UNICODE
  return (const TCHAR *)str;
#else
  static TCHAR unicodeString[MAX_PATH];
  MultiByteToWideChar(CP_ACP, 0, str, strlen(str) + 1, unicodeString, sizeof(unicodeString) / sizeof(TCHAR));
  return unicodeString;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeWin32::FilesystemNodeWin32()
{
  // Create a virtual root directory for standard Windows system
  _isDirectory = true;
  _isFile = false;
  _isValid = false;
  _path = "";
  _isPseudoRoot = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeWin32::FilesystemNodeWin32(const string& p)
{
  // Default to home directory
  _path = p.length() > 0 ? p : "~";

  // Expand '~' to the users 'home' directory
  if(_path[0] == '~')
    _path.replace(0, 1, myHomeFinder.getHomePath());

  setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNodeWin32::getShortPath() const
{
  // If the path starts with the home directory, replace it with '~'
  const string& home = myHomeFinder.getHomePath();
  if(home != "" && BSPF_startsWithIgnoreCase(_path, home))
  {
    string path = "~";
    const char* offset = _path.c_str() + home.length();
    if(*offset != '\\') path += '\\';
    path += offset;
    return path;
  }
  return _path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeWin32::
    getChildren(AbstractFSList& myList, ListMode mode, bool hidden) const
{
  assert(_isDirectory);

  //TODO: honor the hidden flag

  if (_isPseudoRoot)
  {
    // Drives enumeration
    TCHAR drive_buffer[100];
    GetLogicalDriveStrings(sizeof(drive_buffer) / sizeof(TCHAR), drive_buffer);

    for (TCHAR *current_drive = drive_buffer; *current_drive;
         current_drive += _tcslen(current_drive) + 1)
    {
      FilesystemNodeWin32 entry;
      char drive_name[2];

      drive_name[0] = toAscii(current_drive)[0];
      drive_name[1] = '\0';
      entry._displayName = drive_name;
      entry._isDirectory = true;
      entry._isFile = false;
      entry._isValid = true;
      entry._isPseudoRoot = false;
      entry._path = toAscii(current_drive);
      myList.push_back(new FilesystemNodeWin32(entry));
    }
  }
  else
  {
    // Files enumeration
    WIN32_FIND_DATA desc;
    HANDLE handle;
    char searchPath[MAX_PATH + 10];

    sprintf(searchPath, "%s*", _path.c_str());

    handle = FindFirstFile(toUnicode(searchPath), &desc);

    if (handle == INVALID_HANDLE_VALUE)
      return false;

    addFile(myList, mode, _path.c_str(), &desc);

    while (FindNextFile(handle, &desc))
      addFile(myList, mode, _path.c_str(), &desc);

    FindClose(handle);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeWin32::makeDir()
{
  if(!_isPseudoRoot && CreateDirectory(_path.c_str(), NULL) != 0)
  {
    setFlags();
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeWin32::rename(const string& newfile)
{
  if(!_isPseudoRoot && MoveFile(_path.c_str(), newfile.c_str()) != 0)
  {
    setFlags();
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNode* FilesystemNodeWin32::getParent() const
{
  if (!_isValid || _isPseudoRoot)
    return 0;

  FilesystemNodeWin32* p = new FilesystemNodeWin32();
  if (_path.size() > 3)
  {
    const char *start = _path.c_str();
    const char *end = lastPathComponent(_path);

    p->_path = string(start, end - start);
    p->_isValid = true;
    p->_isDirectory = true;
    p->_isFile = false;
    p->_displayName = lastPathComponent(p->_path);
    p->_isPseudoRoot = false;
  }

  return p;
}
