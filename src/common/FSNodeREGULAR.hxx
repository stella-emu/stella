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

#ifndef FS_NODE_REGULAR_HXX
#define FS_NODE_REGULAR_HXX

#include "FSNode.hxx"

/*
 * Implementation of the Stella filesystem API for regular files,
 * based on the std::filesystem API in C++17.
 *
 * Parts of this class are documented in the base interface classes,
 * AbstractFSNode and FSNode.  See those classes for more information.
 */
class FSNodeREGULAR : public AbstractFSNode
{
  public:
    /**
     * Creates a FSNodeREGULAR with the root node as path.
     */
    FSNodeREGULAR();

    /**
     * Creates a FSNodeREGULAR for a given path.
     *
     * @param path    String with the path the new node should point to.
     * @param verify  true if the isValid and isDirectory/isFile flags should
     *                be verified during the construction.
     */
    explicit FSNodeREGULAR(const string& path, bool verify = true);

    bool exists() const override  { return fs::exists(_fspath); }
    const string& getName() const override    { return _displayName; }
    void setName(const string& name) override { _displayName = name; }
    const string& getPath() const override { return _path; }
    string getShortPath() const override;
    bool hasParent() const override;
    bool isDirectory() const override { return _isDirectory; }
    bool isFile() const override      { return _isFile;      }
    bool isReadable() const override  { return _isReadable;  }
    bool isWritable() const override  { return _isWriteable; }
    bool makeDir() override;
    bool rename(const string& newfile) override;

    size_t getSize() const override { return _size; }
    size_t read(ByteBuffer& buffer, size_t size) const override;
    size_t read(stringstream& buffer) const override;
    size_t write(const ByteBuffer& buffer, size_t size) const override;
    size_t write(const stringstream& buffer) const override;

    bool getChildren(AbstractFSList& list, ListMode mode) const override;
    AbstractFSNodePtr getParent() const override;

  protected:
    fs::path _fspath;
    string _path, _displayName;
    bool _isFile{false}, _isDirectory{false},
         _isReadable{false}, _isWriteable{false};
    size_t _size{0};

  private:
    /**
     * Tests and sets the various flags for a file/directory.
     */
    void setFlags();
};

#endif
