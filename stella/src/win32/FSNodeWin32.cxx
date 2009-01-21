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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FSNodeWin32.cxx,v 1.24 2009-01-21 12:03:17 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
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
#include <stdlib.h>
#ifndef _WIN32_WCE
  #include <windows.h>
  // winnt.h defines ARRAYSIZE, but we want our own one...
  #undef ARRAYSIZE
#endif
#include <tchar.h>

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

#include "FSNode.hxx"

/*
 * Used to determine the location of the 'My Documents' folder.
 *
 * Win98 and earlier don't have SHGetFolderPath in shell32.dll.
 * Microsoft recommend that we load shfolder.dll at run time and
 * access the function through that.
 *
 * shfolder.dll is loaded dynamically in the constructor, and unloaded in
 * the destructor
 *
 * The class makes SHGetFolderPath available through its function operator.
 * It will work on all versions of Windows >= Win95.
 *
 * This code was borrowed from the Lyx project.
 */
class MyDocumentsFinder
{
  public:
    MyDocumentsFinder() : myFolderModule(0), myFolderPathFunc(0)
    {
      myFolderModule = LoadLibrary("shfolder.dll");
      if(myFolderModule)
        myFolderPathFunc = reinterpret_cast<function_pointer>
           (::GetProcAddress(myFolderModule, "SHGetFolderPathA"));
    }

    ~MyDocumentsFinder() { if(myFolderModule) FreeLibrary(myFolderModule); }

    /** Wrapper for SHGetFolderPathA, returning the 'My Documents' folder
        (or an empty string if the folder couldn't be determined. */
    string getPath() const
    {
      if(!myFolderPathFunc) return "";
      char folder_path[MAX_PATH];
      HRESULT const result = (myFolderPathFunc)
          (NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, folder_path);

      return (result == 0) ? folder_path : "";
    }

    private:
      typedef HRESULT (__stdcall * function_pointer)(HWND, int, HANDLE, DWORD, LPCSTR);

      HMODULE myFolderModule;
      function_pointer myFolderPathFunc;
};
static MyDocumentsFinder myDocsFinder;

/*
 * Implementation of the Stella file system API based on Windows API.
 *
 * Parts of this class are documented in the base interface class, AbstractFilesystemNode.
 */
class WindowsFilesystemNode : public AbstractFilesystemNode
{
  public:
    /**
     * Creates a WindowsFilesystemNode with the root node as path.
     *
     * In regular windows systems, a virtual root path is used "".
     * In windows CE, the "\" root is used instead.
     */
    WindowsFilesystemNode();

    /**
     * Creates a WindowsFilesystemNode for a given path.
     *
     * Examples:
     *   path=c:\foo\bar.txt, currentDir=false -> c:\foo\bar.txt
     *   path=c:\foo\bar.txt, currentDir=true -> current directory
     *   path=NULL, currentDir=true -> current directory
     *
     * @param path String with the path the new node should point to.
     */
    WindowsFilesystemNode(const string& path);

    virtual bool exists() const { return _access(_path.c_str(), F_OK) == 0; }
    virtual string getDisplayName() const { return _displayName; }
    virtual string getName() const   { return _displayName; }
    virtual string getPath() const   { return _path; }
    virtual bool isDirectory() const { return _isDirectory; }
    virtual bool isReadable() const  { return _access(_path.c_str(), R_OK) == 0; }
    virtual bool isWritable() const  { return _access(_path.c_str(), W_OK) == 0; }

    virtual bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const;
    virtual AbstractFilesystemNode* getParent() const;

  protected:
    string _displayName;
    string _path;
    bool _isDirectory;
    bool _isPseudoRoot;
    bool _isValid;

  private:
    /**
     * Adds a single WindowsFilesystemNode to a given list.
     * This method is used by getChildren() to populate the directory entries list.
     *
     * @param list List to put the file entry node in.
     * @param mode Mode to use while adding the file entry to the list.
     * @param base String with the directory being listed.
     * @param find_data Describes a file that the FindFirstFile, FindFirstFileEx, or FindNextFile functions find.
     */
    static void addFile(AbstractFSList& list, ListMode mode, const char* base, WIN32_FIND_DATA* find_data);

    /**
     * Converts a Unicode string to Ascii format.
     *
     * @param str String to convert from Unicode to Ascii.
     * @return str in Ascii format.
     */
    static char* toAscii(TCHAR *str);

    /**
     * Converts an Ascii string to Unicode format.
     *
     * @param str String to convert from Ascii to Unicode.
     * @return str in Unicode format.
     */
    static const TCHAR* toUnicode(const char* str);
};

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
void WindowsFilesystemNode::addFile(AbstractFSList& list, ListMode mode,
                                    const char* base, WIN32_FIND_DATA* find_data)
{
  WindowsFilesystemNode entry;
  char* asciiName = toAscii(find_data->cFileName);
  bool isDirectory;

  // Skip local directory (.) and parent (..)
  if (!strcmp(asciiName, ".") || !strcmp(asciiName, ".."))
    return;

  isDirectory = (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? true : false);

  if ((!isDirectory && mode == FilesystemNode::kListDirectoriesOnly) ||
      (isDirectory && mode == FilesystemNode::kListFilesOnly))
    return;

  entry._isDirectory = isDirectory;
  entry._displayName = asciiName;
  entry._path = base;
  entry._path += asciiName;
  if (entry._isDirectory)
    entry._path += "\\";
  entry._isValid = true;
  entry._isPseudoRoot = false;

  list.push_back(new WindowsFilesystemNode(entry));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* WindowsFilesystemNode::toAscii(TCHAR* str)
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
const TCHAR* WindowsFilesystemNode::toUnicode(const char* str)
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
WindowsFilesystemNode::WindowsFilesystemNode()
{
  _isDirectory = true;
#ifndef _WIN32_WCE
  // Create a virtual root directory for standard Windows system
  _isValid = false;
  _path = "";
  _isPseudoRoot = true;
#else
  _displayName = "Root";
  // No need to create a pseudo root directory on Windows CE
  _isValid = true;
  _path = "\\";
  _isPseudoRoot = false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WindowsFilesystemNode::WindowsFilesystemNode(const string& p)
{
  // Expand "~\\" to the 'My Documents' directory
  if ( p.length() >= 2 && p[0] == '~' && p[1] == '\\')
  {
    _path = myDocsFinder.getPath();
    // Skip over the tilda.  We know that p contains at least
    // two chars, so this is safe:
    _path += p.c_str() + 1;
  }
  // Expand ".\\" to the current directory
  else if ( p.length() >= 2 && p[0] == '.' && p[1] == '\\')
  {
    char path[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, path);
    _path = path;
    // Skip over the tilda.  We know that p contains at least
    // two chars, so this is safe:
    _path += p.c_str() + 1;
  }
  else
  {
    assert(p.size() > 0);
    _path = p;
  }

  _displayName = lastPathComponent(_path);

  // Check whether it is a directory, and whether the file actually exists
  DWORD fileAttribs = GetFileAttributes(toUnicode(_path.c_str()));

  if (fileAttribs == INVALID_FILE_ATTRIBUTES)
  {
    _isDirectory = false;
    _isValid = false;
  }
  else
  {
    _isDirectory = ((fileAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0);
    _isValid = true;

    // Add a trailing slash, if necessary.
    if (_isDirectory && _path.length() > 0 && _path[_path.length()-1] != '\\')
      _path += '\\';
  }
  _isPseudoRoot = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WindowsFilesystemNode::
    getChildren(AbstractFSList& myList, ListMode mode, bool hidden) const
{
  assert(_isDirectory);

  //TODO: honor the hidden flag

  if (_isPseudoRoot)
  {
#ifndef _WIN32_WCE
    // Drives enumeration
    TCHAR drive_buffer[100];
    GetLogicalDriveStrings(sizeof(drive_buffer) / sizeof(TCHAR), drive_buffer);

    for (TCHAR *current_drive = drive_buffer; *current_drive;
         current_drive += _tcslen(current_drive) + 1)
    {
      WindowsFilesystemNode entry;
      char drive_name[2];

      drive_name[0] = toAscii(current_drive)[0];
      drive_name[1] = '\0';
      entry._displayName = drive_name;
      entry._isDirectory = true;
      entry._isValid = true;
      entry._isPseudoRoot = false;
      entry._path = toAscii(current_drive);
      myList.push_back(new WindowsFilesystemNode(entry));
    }
#endif
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
AbstractFilesystemNode* WindowsFilesystemNode::getParent() const
{
//  assert(_isValid || _isPseudoRoot);

  if (!_isValid || _isPseudoRoot)
    return 0;

  WindowsFilesystemNode* p = new WindowsFilesystemNode();
  if (_path.size() > 3)
  {
    const char *start = _path.c_str();
    const char *end = lastPathComponent(_path);

    p->_path = string(start, end - start);
    p->_isValid = true;
    p->_isDirectory = true;
    p->_displayName = lastPathComponent(p->_path);
    p->_isPseudoRoot = false;
  }

  return p;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeRootFileNode()
{
  return new WindowsFilesystemNode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeCurrentDirectoryFileNode()
{
  return new WindowsFilesystemNode(".\\");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeHomeDirectoryFileNode()
{
  return new WindowsFilesystemNode("~\\");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeFileNodePath(const string& path)
{
  return new WindowsFilesystemNode(path);
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFilesystemNode::makeDir(const string& path)
{
  return CreateDirectory(path.c_str(), NULL) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFilesystemNode::renameFile(const string& oldfile,
                                        const string& newfile)
{
  return MoveFile(oldfile.c_str(), newfile.c_str()) != 0;
}
