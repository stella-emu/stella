MODULE := src/common/repository/sqlite

MODULE_OBJS := \
	src/common/repository/sqlite/KeyValueRepositorySqlite.o

MODULE_DIRS += \
	src/common/repository/sqlite

# Include common rules
include $(srcdir)/common.rules
