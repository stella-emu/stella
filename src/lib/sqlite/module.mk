MODULE := src/lib/sqlite

MODULE_OBJS := \
	src/lib/sqlite/sqlite3.o

MODULE_TEST_OBJS =

MODULE_DIRS += \
	src/lib/sqlite

# Include common rules
include $(srcdir)/common.rules
