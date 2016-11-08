MODULE := src/emucore/tia/core_6502ts

MODULE_OBJS := \
	src/emucore/tia/core_6502ts/TIA.o \
	src/emucore/tia/core_6502ts/DelayQueueMember.o \
	src/emucore/tia/core_6502ts/DelayQueue.o


MODULE_DIRS += \
	src/emucore/tia/core_6502ts

# Include common rules
include $(srcdir)/common.rules
