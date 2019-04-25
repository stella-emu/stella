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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FS_NODE_LIBRETRO_HXX
#define FS_NODE_LIBRETRO_HXX

#include "FSNode.hxx"

// TODO - fix isFile() functionality so that it actually determines if something
//        is a file; for now, it assumes a file if it isn't a directory

/*
 * Implementation of the Stella file system API based on LIBRETRO API.
 *
 * Parts of this class are documented in the base interface class,
 * AbstractFSNode.
 */
class FilesystemNodeLIBRETRO : public AbstractFSNode
{
  public:
    FilesystemNodeLIBRETRO();

    explicit FilesystemNodeLIBRETRO(const string& path);

    bool exists() const override;
    const string& getName() const override   { return _displayName; }
    const string& getPath() const override   { return _path; }
    string getShortPath() const override;
    bool isDirectory() const override { return _isDirectory; }
    bool isFile() const override      { return _isFile;      }
    bool isReadable() const override;
    bool isWritable() const override;
    bool makeDir() override;
    bool rename(const string& newfile) override;

    bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const override;
    AbstractFSNodePtr getParent() const override;

    uInt32 read(ByteBuffer& image) const override;

  protected:
    string _displayName;
    string _path;
    bool _isDirectory;
    bool _isFile;
    bool _isPseudoRoot;
    bool _isValid;
};

#endif
