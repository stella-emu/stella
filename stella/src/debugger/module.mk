MODULE := src/debugger

MODULE_OBJS := \
	src/debugger/Debugger.o \
	src/debugger/DebuggerParser.o \
	src/debugger/EquateList.o \
	src/debugger/Expression.o \
	src/debugger/PackedBitArray.o \
	src/debugger/CpuDebug.o \
	src/debugger/RamDebug.o \
	src/debugger/TIADebug.o

MODULE_DIRS += \
	src/debugger

# Include common rules 
include $(srcdir)/common.rules
