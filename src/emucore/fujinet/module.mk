MODULE := src/emucore/fujinet

MODULE_OBJS = \
	src/emucore/fujinet/fujibus.o \
	src/emucore/fujinet/fujimail.o \
	src/emucore/fujinet/vcs_render.o \
	src/emucore/fujinet/FujiNetLink.o

MODULE_TEST_OBJS = \
	src/emucore/fujinet/fujibus.o \
	src/emucore/fujinet/FujiBusTest.o

MODULE_DIRS += \
	src/emucore/fujinet

# Include common rules
include $(srcdir)/common.rules
