MODULE := src/win32

MODULE_OBJS := \
	src/win32/FSNodeWin32.o \
	src/win32/OSystemWin32.o \
	src/win32/SettingsWin32.o

MODULE_DIRS += \
	src/win32

# Include common rules 
include $(srcdir)/common.rules
