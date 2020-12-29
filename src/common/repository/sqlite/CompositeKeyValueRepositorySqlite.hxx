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

#ifndef COMPOSITE_KEY_VALUE_REPOSITORY_SQLITE_HXX
#define COMPOSITE_KEY_VALUE_REPOSITORY_SQLITE_HXX

#include "repository/CompositeKeyValueRepository.hxx"

#include "SqliteDatabase.hxx"
#include "SqliteStatement.hxx"
#include "AbstractKeyValueRepositorySqlite.hxx"

class CompositeKeyValueRepositorySqlite : public CompositeKeyValueRepository {
  public:

    CompositeKeyValueRepositorySqlite(SqliteDatabase& db, const string& tableName);

    shared_ptr<KeyValueRepository> get(const string& key) override;

    bool has(const string& key) override;

    void initialize();

  private:

    class ProxyRepository : public AbstractKeyValueRepositorySqlite {
      public:

        ProxyRepository(const CompositeKeyValueRepositorySqlite& repo, const string& key);

      protected:

        SqliteStatement& stmtInsert(const string& key, const string& value) override;
        SqliteStatement& stmtSelect() override;
        SqliteDatabase& database() override;

      private:

        const CompositeKeyValueRepositorySqlite& myRepo;
        const string myKey;

      private:

        ProxyRepository(const ProxyRepository&) = delete;
        ProxyRepository(ProxyRepository&&) = delete;
        ProxyRepository& operator=(const ProxyRepository&) = delete;
        ProxyRepository& operator=(ProxyRepository&&) = delete;
    };

  private:

    string myTableName;
    SqliteDatabase& myDb;

    unique_ptr<SqliteStatement> myStmtInsert;
    unique_ptr<SqliteStatement> myStmtSelect;
    unique_ptr<SqliteStatement> myStmtCount;

   private:

    CompositeKeyValueRepositorySqlite(const CompositeKeyValueRepositorySqlite&) = delete;
    CompositeKeyValueRepositorySqlite(CompositeKeyValueRepositorySqlite&&) = delete;
    CompositeKeyValueRepositorySqlite& operator=(const CompositeKeyValueRepositorySqlite&) = delete;
    CompositeKeyValueRepositorySqlite& operator=(CompositeKeyValueRepositorySqlite&&) = delete;
};

#endif // COMPOSITE_KEY_VALUE_REPOSITORY_SQLITE_HXX
