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

#ifdef ZIP_SUPPORT

#ifndef FS_NODE_ZIP_HXX
#define FS_NODE_ZIP_HXX

#include <mutex>
#include <stdexcept>

#include "ZipHandler.hxx"
#include "FSNode.hxx"

/*
 * Implementation of the Stella file system API based on ZIP archives.
 * ZIP archives are treated as directories if the contain more than one ROM
 * file, as a file if they contain a single ROM file, and as neither if the
 * archive is empty.  Hence, if a ZIP archive isn't a directory *or* a file,
 * it is invalid.
 *
 * The above is ZipMode::Rom; in ZipMode::Data the archive is always a
 * directory.  In both modes, a path into the archive is a file if it names an
 * entry, whatever its extension, and a directory if entries lie under it; any
 * other path doesn't exist.
 *
 * Parts of this class are documented in the base interface class, AbstractFSNode.
 */
class FSNodeZIP : public AbstractFSNode
{
  public:
    using ZipError = ZipHandler::ZipError;
    using ZipException = ZipHandler::ZipException;
    using ZipMode = FSNode::ZipMode;

    /**
     * Creates a FSNodeZIP for a given path.
     *
     * @param path  String with the path the new node should point to.
     * @param mode  How the path into the archive is resolved
     */
    FSNodeZIP(string_view path, ZipMode mode);

    bool exists() const override;
    const string& getName() const override  { return _name; }
    void setName(string_view name) override { _name = name; }
    const string& getPath() const override { return _path;      }
    string getShortPath() const   override { return _shortPath; }
    bool hasParent() const override   { return true; }
    bool isDirectory() const override { return _kind == NodeKind::Directory; }
    bool isFile()      const override { return _kind == NodeKind::File;      }
    bool isReadable() const  override { return _realNode && _realNode->isReadable(); }
    bool isWritable() const  override { return false; }

    //////////////////////////////////////////////////////////
    // For now, ZIP files cannot be modified in any way
    bool makeDir() override { return false; }
    bool rename(string_view) override { return false; }
    //////////////////////////////////////////////////////////

    size_t getSize() const override { return _size; }
    bool getChildren(AbstractFSList& list, ListMode mode) const override;
    AbstractFSNodePtr getParent() const override;
    AbstractFSNodePtr getSiblingNode(string_view ext) const override;

    size_t read(ByteArray& buffer, size_t) const override;
    size_t read(std::stringstream& buffer) const override;
    size_t write(ByteSpan) const override {
      throw std::runtime_error("ZIP file writing not implemented");
    }
    size_t write(string_view) const override {
      throw std::runtime_error("ZIP file writing not implemented");
    }

  public:
    // Passkey: only FSNodeZIP internals can construct Key{}, enabling make_shared
    struct Key { explicit Key() = default; };
    FSNodeZIP(Key, string_view zipfile, string_view virtualpath,
        const AbstractFSNodePtr& realnode, size_t size, bool isdir, ZipMode mode);

  private:
    void setFlags(string_view zipfile, string_view virtualpath,
        const AbstractFSNodePtr& realnode);

    static AbstractFSNodePtr makeShared(string_view zipfile, string_view virtualpath,
        const AbstractFSNodePtr& realnode, size_t size, bool isdir, ZipMode mode);

    friend std::ostream& operator<<(std::ostream& os, const FSNodeZIP& node) {
      os << "_zipFile:     " << node._zipFile << '\n'
         << "_virtualPath: " << node._virtualPath << '\n'
         << "_name:        " << node._name << '\n'
         << "_path:        " << node._path << '\n'
         << "_size:        " << node._size << '\n'
         << "_shortPath:   " << node._shortPath << '\n';
      return os;
    }

  private:
    // Since a ZIP file is itself an abstraction, it still needs access to
    // an actual concrete filesystem node
    AbstractFSNodePtr _realNode;

    string _zipFile, _virtualPath;
    string _name, _path, _shortPath;
    size_t _size{0};

    enum class NodeKind : uInt8 { Invalid, File, Directory };
    NodeKind _kind{NodeKind::Invalid};
    ZipMode _mode{ZipMode::Rom};

    // ZipHandler static reference variable responsible for accessing ZIP files
    static ZipHandler& zipHandler() {
      static ZipHandler z;
      return z;
    }

    // Held for every use of zipHandler(), which the emulation thread also uses
    static std::mutex& zipMutex() {
      static std::mutex m;
      return m;
    }
};

#endif  // FS_NODE_ZIP_HXX

#endif  // ZIP_SUPPORT
