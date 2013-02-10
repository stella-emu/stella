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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef FS_NODE_ZIP_HXX
#define FS_NODE_ZIP_HXX

#include "StringList.hxx"
#include "FSNode.hxx"

/*
 * Implementation of the Stella file system API based on ZIP archives.
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
     * @param path    String with the path the new node should point to.
     * @param verify  true if the isValid and isDirectory/isFile flags should
     *                be verified during the construction.
     */
    FilesystemNodeZIP(const string& path);

    bool exists() const { return false; }
    const string& getName() const { return _virtualFile; }
    const string& getPath() const { return _path;        }
    string getShortPath() const   { return _shortPath;   }
    bool isDirectory() const { return _isDirectory; }
    bool isFile() const      { return _isFile;      }
    bool isReadable() const  { return false; }
    bool isWritable() const  { return false; }
    bool isAbsolute() const;

    //////////////////////////////////////////////////////////
    // For now, ZIP files cannot be modified in any way
    bool makeDir() { return false; }
    bool rename(const string& newfile) { return false; }
    //////////////////////////////////////////////////////////

    bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const;
    AbstractFSNode* getParent() const;

  protected:
    Common::SharedPtr<AbstractFSNode> _realNode;
    string _zipFile, _virtualFile;
    string _path, _shortPath;
    bool _isDirectory;
    bool _isFile;
    bool _isValid;
    bool _isVirtual;
};

#endif
