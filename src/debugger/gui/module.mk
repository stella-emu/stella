MODULE := src/debugger/gui

MODULE_OBJS := \
	src/debugger/gui/AudioWidget.o \
	src/debugger/gui/CpuWidget.o \
	src/debugger/gui/PromptWidget.o \
	src/debugger/gui/RamWidget.o \
	src/debugger/gui/RiotWidget.o \
	src/debugger/gui/RomWidget.o \
	src/debugger/gui/RomListWidget.o \
	src/debugger/gui/RomListSettings.o \
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
	src/debugger/gui/Cart0840Widget.o \
	src/debugger/gui/Cart2KWidget.o \
	src/debugger/gui/Cart3EWidget.o \
	src/debugger/gui/Cart3FWidget.o \
	src/debugger/gui/Cart4A50Widget.o \
	src/debugger/gui/Cart4KWidget.o \
	src/debugger/gui/Cart4KSCWidget.o \
	src/debugger/gui/CartARWidget.o \
	src/debugger/gui/CartCMWidget.o \
	src/debugger/gui/CartCTYWidget.o \
	src/debugger/gui/CartCVWidget.o \
	src/debugger/gui/CartDPCWidget.o \
	src/debugger/gui/CartDPCPlusWidget.o \
	src/debugger/gui/CartE0Widget.o \
	src/debugger/gui/CartE7Widget.o \
	src/debugger/gui/CartEFWidget.o \
	src/debugger/gui/CartEFSCWidget.o \
	src/debugger/gui/CartBFWidget.o \
	src/debugger/gui/CartBFSCWidget.o \
	src/debugger/gui/CartDFWidget.o \
	src/debugger/gui/CartDFSCWidget.o \
	src/debugger/gui/CartF0Widget.o \
	src/debugger/gui/CartF4Widget.o \
	src/debugger/gui/CartF6Widget.o \
	src/debugger/gui/CartF8Widget.o \
	src/debugger/gui/CartF4SCWidget.o \
	src/debugger/gui/CartF6SCWidget.o \
	src/debugger/gui/CartF8SCWidget.o \
	src/debugger/gui/CartFAWidget.o \
	src/debugger/gui/CartFA2Widget.o \
	src/debugger/gui/CartFEWidget.o \
	src/debugger/gui/CartMCWidget.o \
	src/debugger/gui/CartSBWidget.o \
	src/debugger/gui/CartUAWidget.o \
	src/debugger/gui/CartX07Widget.o \
	src/debugger/gui/JoystickWidget.o \
	src/debugger/gui/PaddleWidget.o \
	src/debugger/gui/BoosterWidget.o \
	src/debugger/gui/DrivingWidget.o \
	src/debugger/gui/KeyboardWidget.o \
	src/debugger/gui/GenesisWidget.o

MODULE_DIRS += \
	src/debugger/gui

# Include common rules 
include $(srcdir)/common.rules
