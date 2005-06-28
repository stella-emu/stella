MODULE := src/gui

MODULE_OBJS := \
	src/gui/AboutDialog.o \
	src/gui/AddrValueWidget.o \
	src/gui/AudioDialog.o \
	src/gui/BrowserDialog.o \
	src/gui/CheatWidget.o \
	src/gui/CpuWidget.o \
	src/gui/DataGridWidget.o \
	src/gui/DebuggerDialog.o \
	src/gui/DialogContainer.o \
	src/gui/Dialog.o \
	src/gui/EditableWidget.o \
	src/gui/EditNumWidget.o \
	src/gui/EditTextWidget.o \
	src/gui/EventMappingDialog.o \
	src/gui/Font.o \
	src/gui/GameInfoDialog.o \
	src/gui/GameList.o \
	src/gui/HelpDialog.o \
	src/gui/Launcher.o \
	src/gui/LauncherDialog.o \
	src/gui/LauncherOptionsDialog.o \
	src/gui/ListWidget.o \
	src/gui/Menu.o \
	src/gui/OptionsDialog.o \
	src/gui/PopUpWidget.o \
	src/gui/ProgressDialog.o \
	src/gui/PromptWidget.o \
	src/gui/RamWidget.o \
	src/gui/ScrollBarWidget.o \
	src/gui/TabWidget.o \
	src/gui/ToggleBitWidget.o \
	src/gui/VideoDialog.o \
	src/gui/Widget.o \

MODULE_DIRS += \
	src/gui

# Include common rules 
include $(srcdir)/common.rules
