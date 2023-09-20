MODULE := src/os/rtstella

MODULE_OBJS := \
	src/os/rtstella/Spinlock.o \
	src/os/rtstella/RTEmulationWorker.o \
	src/os/rtstella/OSystemRTStella.o

MODULE_DIRS += \
	src/os/rtstella

# Include common rules
include $(srcdir)/common.rules
