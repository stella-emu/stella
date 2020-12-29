#ifndef ABSTRACT_KEY_VALUE_REPOSITORY_SQLITE_HXX
#define ABSTRACT_KEY_VALUE_REPOSITORY_SQLITE_HXX

#include "bspf.hxx"
#include "repository/KeyValueRepository.hxx"
#include "SqliteDatabase.hxx"
#include "SqliteStatement.hxx"

class AbstractKeyValueRepositorySqlite : public KeyValueRepository
{
  public:

    std::map<string, Variant> load() override;

    void save(const std::map<string, Variant>& values) override;

    void save(const string& key, const Variant& value) override;

  protected:

    virtual SqliteStatement& stmtInsert(const string& key, const string& value) = 0;
    virtual SqliteStatement& stmtSelect() = 0;
    virtual SqliteDatabase& database() = 0;
};

#endif // ABSTRACT_KEY_VALUE_REPOSITORY_SQLITE_HXX
