MODULE := src/debugger/gui/memview

MODULE_OBJS := \
	src/debugger/gui/memview/MemViewAccessLayer.o \
	src/debugger/gui/memview/MemViewDataLayer.o \
	src/debugger/gui/memview/MemViewLayer.o \
	src/debugger/gui/memview/MemViewMarkerLayer.o \
	src/debugger/gui/memview/MemViewParams.o \
	src/debugger/gui/memview/MemViewWidget.o \
	src/debugger/gui/memview/MemViewWindow.o \
	src/debugger/gui/memview/MemViewWindowDialog.o

MODULE_TEST_OBJS =

MODULE_DIRS += \
	src/debugger/gui/memview

# Include common rules
include $(srcdir)/common.rules
