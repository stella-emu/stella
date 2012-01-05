MODULE := src/debugger/gui

MODULE_OBJS := \
	src/debugger/gui/AudioWidget.o \
	src/debugger/gui/CpuWidget.o \
	src/debugger/gui/PromptWidget.o \
	src/debugger/gui/RamWidget.o \
	src/debugger/gui/RiotWidget.o \
	src/debugger/gui/RomWidget.o \
	src/debugger/gui/RomListWidget.o \
	src/debugger/gui/TiaWidget.o \
	src/debugger/gui/TiaInfoWidget.o \
	src/debugger/gui/TiaOutputWidget.o \
	src/debugger/gui/TiaZoomWidget.o \
	src/debugger/gui/ColorWidget.o \
	src/debugger/gui/DataGridOpsWidget.o \
	src/debugger/gui/DataGridWidget.o \
	src/debugger/gui/DebuggerDialog.o \
	src/debugger/gui/ToggleBitWidget.o \
	src/debugger/gui/TogglePixelWidget.o \
	src/debugger/gui/ToggleWidget.o \
	src/debugger/gui/JoystickWidget.o

MODULE_DIRS += \
	src/debugger/gui

# Include common rules 
include $(srcdir)/common.rules
