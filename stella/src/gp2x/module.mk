MODULE := src/gp2x

MODULE_OBJS := \
	src/gp2x/FSNodeGP2X.o \
	src/gp2x/OSystemGP2X.o \
	src/gp2x/SettingsGP2X.o
MODULE_DIRS += \
	src/gp2x

# Include common rules 
include $(srcdir)/common.rules
