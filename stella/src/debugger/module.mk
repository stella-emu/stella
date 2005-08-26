MODULE := src/debugger

MODULE_OBJS := \
	src/debugger/Cheat.o \
	src/debugger/CheetahCheat.o \
	src/debugger/Debugger.o \
	src/debugger/DebuggerParser.o \
	src/debugger/EquateList.o \
	src/debugger/Expression.o \
	src/debugger/FunctionExpression.o \
	src/debugger/CpuMethodExpression.o \
	src/debugger/TiaMethodExpression.o \
	src/debugger/ByteDerefExpression.o \
	src/debugger/ByteDerefOffsetExpression.o \
	src/debugger/WordDerefExpression.o \
	src/debugger/ConstExpression.o \
	src/debugger/EquateExpression.o \
	src/debugger/LoByteExpression.o \
	src/debugger/HiByteExpression.o \
	src/debugger/NotEqualsExpression.o \
	src/debugger/EqualsExpression.o \
	src/debugger/PlusExpression.o \
	src/debugger/UnaryMinusExpression.o \
	src/debugger/MinusExpression.o \
	src/debugger/MultExpression.o \
	src/debugger/DivExpression.o \
	src/debugger/ModExpression.o \
	src/debugger/LogAndExpression.o \
	src/debugger/LogOrExpression.o \
	src/debugger/LogNotExpression.o \
	src/debugger/BinNotExpression.o \
	src/debugger/BinAndExpression.o \
	src/debugger/BinOrExpression.o \
	src/debugger/BinXorExpression.o \
	src/debugger/GreaterExpression.o \
	src/debugger/GreaterEqualsExpression.o \
	src/debugger/LessExpression.o \
	src/debugger/LessEqualsExpression.o \
	src/debugger/ShiftRightExpression.o \
	src/debugger/ShiftLeftExpression.o \
	src/debugger/PackedBitArray.o \
	src/debugger/CpuDebug.o \
	src/debugger/RamDebug.o \
	src/debugger/TIADebug.o \
	src/debugger/TiaInfoWidget.o \
	src/debugger/TiaOutputWidget.o \
	src/debugger/CpuWidget.o \
	src/debugger/PromptWidget.o \
	src/debugger/RamWidget.o \
	src/debugger/RomWidget.o \
	src/debugger/RomListWidget.o \
	src/debugger/TiaWidget.o

MODULE_DIRS += \
	src/debugger

# Include common rules 
include $(srcdir)/common.rules
