CFLAGS+=-fPIC -Wno-implicit-function-declaration -Wno-unused-function
FILES=$(wildcard *.c **/*.c **/*/*.c **/*/*/*.c *.s **/*.s *.S **/*.S)
#DEPS=$(FILES:.c=.d)
OBJS=$(FILES:.c=.o)

# SUBDIRS:=frr vppinfra self iperf

-include $(DEPS)

all: $(OBJS)
	@$(CC) -shared $^ -o libqsc.so $(LDFLAGS)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	rm -rf *.o *.so *.d
	@for dir in $(SUBDIRS); do\
		$(MAKE) -C $$dir $@;\
	done

.PHONY: all clean

