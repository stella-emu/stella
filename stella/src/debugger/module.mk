MODULE := src/debugger

MODULE_OBJS := \
	src/debugger/Debugger.o \
	src/debugger/DebuggerParser.o \
	src/debugger/EquateList.o \
	src/debugger/Expression.o \
	src/debugger/ConstExpression.o \
	src/debugger/EqualsExpression.o \
	src/debugger/PlusExpression.o \
	src/debugger/MinusExpression.o \
	src/debugger/MultExpression.o \
	src/debugger/DivExpression.o \
	src/debugger/ModExpression.o \
	src/debugger/LogAndExpression.o \
	src/debugger/LogOrExpression.o \
	src/debugger/BinAndExpression.o \
	src/debugger/BinOrExpression.o \
	src/debugger/BinXorExpression.o \
	src/debugger/PackedBitArray.o \
	src/debugger/CpuDebug.o \
	src/debugger/RamDebug.o \
	src/debugger/TIADebug.o

MODULE_DIRS += \
	src/debugger

# Include common rules 
include $(srcdir)/common.rules
