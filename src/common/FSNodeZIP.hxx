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

#ifndef FS_NODE_ZIP_HXX
#define FS_NODE_ZIP_HXX

#include "StringList.hxx"
#include "FSNode.hxx"

/*
 * Implementation of the Stella file system API based on ZIP archives.
 * ZIP archives are treated as directories if the contain more than one ROM
 * file, as a file if they contain a single ROM file, and as neither if the
 * archive is empty.  Hence, if a ZIP archive isn't a directory *or* a file,
 * it is invalid.
 *
 * Parts of this class are documented in the base interface class, AbstractFSNode.
 */
class FilesystemNodeZIP : public AbstractFSNode
{
  public:
    /**
     * Creates a FilesystemNodeZIP with the root node as path.
     */
    FilesystemNodeZIP();

    /**
     * Creates a FilesystemNodeZIP for a given path.
     *
     * @param path  String with the path the new node should point to.
     * @param node  Raw pointer to use for the internal FSNode
     */
    FilesystemNodeZIP(const string& path);

    bool exists() const      { return _realNode && _realNode->exists(); }
    const string& getName() const { return _virtualFile; }
    const string& getPath() const { return _path;        }
    string getShortPath() const   { return _shortPath;   }
    bool isDirectory() const { return _numFiles > 1;  }
    bool isFile() const      { return _numFiles == 1; }
    bool isReadable() const  { return _realNode && _realNode->isReadable(); }
    bool isWritable() const  { return false; }

    //////////////////////////////////////////////////////////
    // For now, ZIP files cannot be modified in any way
    bool makeDir() { return false; }
    bool rename(const string& newfile) { return false; }
    //////////////////////////////////////////////////////////

    bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const;
    AbstractFSNode* getParent() const;

    uInt32 read(uInt8*& image) const;

  private:
    FilesystemNodeZIP(const string& zipfile, const string& virtualfile,
        Common::SharedPtr<AbstractFSNode> realnode);

    void setFlags(const string& zipfile, const string& virtualfile,
        Common::SharedPtr<AbstractFSNode> realnode);

  private:
    /* Error types */
    enum zip_error
    {
      ZIPERR_NONE = 0,
      ZIPERR_NOT_A_FILE,
      ZIPERR_NOT_READABLE,
      ZIPERR_NO_ROMS
    };

    Common::SharedPtr<AbstractFSNode> _realNode;
    string _zipFile, _virtualFile;
    string _path, _shortPath;
    zip_error _error;
    uInt32 _numFiles;
};

#endif
