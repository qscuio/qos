obj-m += ftracehook.o
SRCDIR = $(PWD)
ftracehook-objs := main.o hook.o
MCFLAGS += -std=gnu11
ccflags-y += -ggdb ${MCFLAGS}
CC += ${MCFLAGS}
KDIR := /lib/modules/$(shell uname -r)/build
KOUTPUT := $(PWD)/build
KOUTPUT_MAKEFILE := $(KOUTPUT)/Makefile

all: $(KOUTPUT_MAKEFILE)
	make -C $(KDIR) M=$(KOUTPUT) src=$(SRCDIR) modules

$(KOUTPUT):
	mkdir -p "$@"

$(KOUTPUT_MAKEFILE): $(KOUTPUT)
	touch "$@"

clean:
	make -C $(KDIR) M=$(KOUTPUT) src=$(SRCDIR) clean
	$(shell rm $(KOUTPUT_MAKEFILE))
	rmdir $(KOUTPUT)
