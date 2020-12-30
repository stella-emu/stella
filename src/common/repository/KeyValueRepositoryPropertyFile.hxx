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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef KEY_VALUE_REPOSITORY_PROPERTY_FILE_HXX
#define KEY_VALUE_REPOSITORY_PROPERTY_FILE_HXX

#include "repository/KeyValueRepository.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"

class KeyValueRepositoryPropertyFile : public KeyValueRepository {
  public:
    KeyValueRepositoryPropertyFile(const FilesystemNode& node);

    std::map<string, Variant> load() override;

    bool save(const std::map<string, Variant>& values) override;

    bool save(const string& key, const Variant& value) override { return false; }

    void remove(const string& key) override {}

  private:

    const FilesystemNode& myNode;
};

#endif
