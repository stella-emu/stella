###############################################################################
## C compiler to use
###############################################################################
CC = gcc

CFLAGS = -DWIN32 -O -ansi -Wall

OBJS = TIASound.o

all:
	@echo ""
	@echo "To build stella-sound, type: 'make <version>'"
	@echo ""
	@echo "where <version> is one of:"
	@echo ""
	@echo "  linux   Linux (same as uss)"
	@echo "  uss     Systems with the Unix Sound System (same as oss)"
	@echo "  oss     systems with the Open Sound System"
	@echo "  bsdi    BSD/OS 4.0"
	@echo ""
	@echo "Hopefully new versions will be added soon!"
	@echo ""

bsdi:
	make oss CFLAGS="-DWIN32 -O3 -Wall"

linux: oss

uss: oss

oss: $(OBJS) OSS.o
	$(CC) -o stella-sound OSS.o $(OBJS)

clean:
	rm -f *.o stella-sound

