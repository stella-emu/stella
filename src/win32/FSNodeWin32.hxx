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

#ifndef FS_NODE_WIN32_HXX
#define FS_NODE_WIN32_HXX

#include <tchar.h>

#include "FSNode.hxx"
#include "HomeFinder.hxx"

static HomeFinder myHomeFinder;

// TODO - fix isFile() functionality so that it actually determines if something
//        is a file; for now, it assumes a file if it isn't a directory

/*
 * Implementation of the Stella file system API based on Windows API.
 *
 * Parts of this class are documented in the base interface class,
 * AbstractFSNode.
 */
class FilesystemNodeWin32 : public AbstractFSNode
{
  public:
    /**
     * Creates a FilesystemNodeWin32 with the root node as path.
     *
     * In regular windows systems, a virtual root path is used "".
     * In windows CE, the "\" root is used instead.
     */
    FilesystemNodeWin32();

    /**
     * Creates a FilesystemNodeWin32 for a given path.
     *
     * Examples:
     *   path=c:\foo\bar.txt, currentDir=false -> c:\foo\bar.txt
     *   path=c:\foo\bar.txt, currentDir=true -> current directory
     *   path=NULL, currentDir=true -> current directory
     *
     * @param path String with the path the new node should point to.
     */
    FilesystemNodeWin32(const string& path);

    bool exists() const;
    const string& getName() const   { return _displayName; }
    const string& getPath() const   { return _path; }
    string getShortPath() const;
    bool isDirectory() const { return _isDirectory; }
    bool isFile() const      { return _isFile;      }
    bool isReadable() const;
    bool isWritable() const;
    bool makeDir();
    bool rename(const string& newfile);

    bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const;
    AbstractFSNode* getParent() const;

  protected:
    string _displayName;
    string _path;
    bool _isDirectory;
    bool _isFile;
    bool _isPseudoRoot;
    bool _isValid;

  private:
    /**
     * Tests and sets the _isValid and _isDirectory/_isFile flags,
     * using the GetFileAttributes() function.
     */
    virtual void setFlags();

    /**
     * Adds a single FilesystemNodeWin32 to a given list.
     * This method is used by getChildren() to populate the directory entries list.
     *
     * @param list       List to put the file entry node in.
     * @param mode       Mode to use while adding the file entry to the list.
     * @param base       String with the directory being listed.
     * @param find_data  Describes a file that the FindFirstFile, FindFirstFileEx, or FindNextFile functions find.
     */
    static void addFile(AbstractFSList& list, ListMode mode, const char* base, WIN32_FIND_DATA* find_data);

    /**
     * Converts a Unicode string to Ascii format.
     *
     * @param str  String to convert from Unicode to Ascii.
     * @return str in Ascii format.
     */
    static char* toAscii(TCHAR *str);

    /**
     * Converts an Ascii string to Unicode format.
     *
     * @param str  String to convert from Ascii to Unicode.
     * @return str in Unicode format.
     */
    static const TCHAR* toUnicode(const char* str);
};

#endif
