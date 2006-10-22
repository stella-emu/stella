MODULE := src/common/scaler

MODULE_OBJS := \
	src/common/scaler/hq2x.o \
	src/common/scaler/hq3x.o \
	src/common/scaler/scaler.o \
	src/common/scaler/scalebit.o \
	src/common/scaler/scale2x.o \
	src/common/scaler/scale3x.o

ifdef HAVE_NASM
MODULE_OBJS += \
	src/common/scaler/hq2x_i386.o \
	src/common/scaler/hq3x_i386.o
endif

MODULE_DIRS += \
	src/common/scaler

# Include common rules 
include $(srcdir)/common.rules
