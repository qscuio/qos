
HEAD = head.o
SYSTEM = ../tools/zSystem
#LD = gcc
#TEST = -DTEST_DRIVER

zOBJECTS = $(HEAD) inflate.o unzip.o misc.o

ZCFLAGS = $(CFLAGS) -DSTDC_HEADERS $(TEST)
HOSTCFLAGS ?= -O2
OBJCOPY ?= objcopy

.c.s:
	$(CC) $(ZCFLAGS) -S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(ZCFLAGS) -c -o $*.o $<

all:	zSystem

zSystem:	piggy.o $(zOBJECTS)
		$(LD) $(LDFLAGS) -o zSystem.elf -Ttext 1000 $(zOBJECTS) piggy.o
		$(OBJCOPY) -O binary zSystem.elf zSystem
		rm -f zSystem.elf

head.o:	head.s

head.s: head.S ../include/linux/tasks.h
	$(CPP) -traditional head.S -o head.s

piggy.o:	$(SYSTEM)
		$(OBJCOPY) -O binary $(SYSTEM) piggy.bin
		gzip -9 < piggy.bin > piggy.bin.gz
		$(OBJCOPY) -I binary -O elf32-i386 -B i386 piggy.bin.gz piggy.o
		rm -f piggy.bin
		rm -f piggy.bin.gz

xtract: xtract.c
	$(HOSTCC) $(HOSTCFLAGS) -DSTDC_HEADERS -I../include -o $@ $<

piggyback: piggyback.c
	$(HOSTCC) $(HOSTCFLAGS) -DSTDC_HEADERS -I../include -o $@ $<

$(SYSTEM):
		$(MAKE) -C .. tools/zSystem
