MODULE := src/common/repository/sqlite

MODULE_OBJS := \
	src/common/repository/sqlite/KeyValueRepositorySqlite.o \
	src/common/repository/sqlite/SettingsDb.o \
	src/common/repository/sqlite/SqliteDatabase.o \
	src/common/repository/sqlite/SqliteStatement.o \
	src/common/repository/sqlite/SqliteTransaction.o

MODULE_DIRS += \
	src/common/repository/sqlite

# Include common rules
include $(srcdir)/common.rules
