SRC_DIR := $(M)

IGNORE_OBJS := hw_breakpoint.o block_dev.o irqsim.o fileops.o netdev.o net_dev.o pktgen.o readfile.o idt.o syscall.o chdir.o kelf.o thrd_showall.o super_block.o softirq.o sock_proto_ulk.o

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, %.o, $(SRC_FILES))

MODULES := $(filter-out $(IGNORE_OBJS), $(OBJ_FILES))
MODULES := $(filter-out %.mod.o, $(MODULES))

$(info Starting building kernel debug modules)

obj-m := $(MODULES)
