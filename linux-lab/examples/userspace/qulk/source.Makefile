CUR_DIR=$(shell pwd)

CC=gcc

CURDIR=`pwd`
DEPDIR=$(CURDIR)/.deps

DEP_CFLAGS:= -MD

CFLAGS:= -rdynamic
#CLFAGS+= $(DEP_CFLAGS)
CFLAGS+= -MD
CFLAGS+= -Wall -Werror -pg -mfentry -g -fpatchable-function-entry=8,4 -Wno-deprecated-declarations -Wno-unknown-pragmas -Wno-unused-variable -Wno-attributes
# CFLAGS+= -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fsanitize-address-use-after-scope -static-libasan
CFLAGS+= -I$(CUR_DIR)/lib  \
	 -I$(CUR_DIR)/qsc \
	 -I$(CUR_DIR)/plthook -I$(CUR_DIR)/funchook/include -I$(CUR_DIR)/injector/include \
	 -I$(CUR_DIR)/libpulp/include -I$(CUR_DIR)/libuv/include\
	 -I$(CUR_DIR)/userspace-rcu/include \
	 -I$(CUR_DIR)/unicorn/include -I$(CUR_DIR)/unicorn/tests/unit/ \
	 -I$(CUR_DIR)/libelfmaster/include -I$(CUR_DIR)/libev \
	 -I$(CUR_DIR)/lib/frr -I$(CUR_DIR)/lib/jobd -I$(CUR_DIR)/lib/jobd/vendor \
	 -I$(CUR_DIR)/lib/pnotify -I$(CUR_DIR)/lib/libkqueue/ -I$(CUR_DIR)/lib/libkqueue/src/common \
	 -I$(CUR_DIR)/lib/libkqueue/test -I$(CUR_DIR)/lib/libkqueue/include/ -I$(CUR_DIR)/lib/iperf/src/ \
	 -I$(CUR_DIR)/lib/shelf/src -I$(CUR_DIR)/lib/libccli/include \

EXTRA_CFLAGS:= -Wa,-alh,-L -fverbose-asm

# LDFLAGS:= -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fsanitize-address-use-after-scope -static-libasan
LDFLAGS+= -Wl,-rpath=$(CUR_DIR)/lib -L$(CUR_DIR)/lib \
	  -Wl,-rpath=$(CUR_DIR)/qsc -L$(CUR_DIR)/qsc \
	  -Wl,-rpath=$(CUR_DIR)/plthook -L$(CUR_DIR)/plthook \
	  -Wl,-rpath=$(CUR_DIR)/funchook/build -L$(CUR_DIR)/funchook/build \
	  -Wl,-rpath=$(CUR_DIR)/injector/src/linux -L$(CUR_DIR)/injector/src/linux/ \
	  -Wl,-rpath=$(CUR_DIR)/libpulp/lib/.libs/ -L$(CUR_DIR)/libpulp/lib/.libs/ \
	  -Wl,-rpath=$(CUR_DIR)/libuv/build/ -L$(CUR_DIR)/libuv/build \
	  -Wl,-rpath=$(CUR_DIR)/unicorn/build/ -L$(CUR_DIR)/unicorn/build \
	  -Wl,-rpath=$(CUR_DIR)/libelfmaster/src -L$(CUR_DIR)/libelfmaster/src \
	  -Wl,-rpath=$(CUR_DIR)/libev/.libs -L$(CUR_DIR)/libev/.libs \
	  -Wl,-rpath=$(CUR_DIR)/userspace-rcu/src/.libs/ -L$(CUR_DIR)/userspace-rcu/src/.libs/ \
	  -Wl,-rpath=$(CUR_DIR)/lib/libccli -L$(CUR_DIR)/lib/libccli \
	  -lpthread -lunbound -lrt -lssl -lcrypto -lcunit -lunwind -luv -lcurl -lbsd -lelf -ldw -lcapstone -lm 
APP_LDFLAGS+= $(LDFLAGS) -lw -lqsc


# Print log does not exist
#ASAN="ASAN_OPTIONS=quarantine_size_mb=1024,log_path=./asan.log,log_to_syslog=true,fast_unwind_on_malloc=0,disable_coredump=false,halt_on_error=false"

# Print log and panic
ASAN="ASAN_OPTIONS=quarantine_size_mb=1024,log_path=./asan.log,log_to_syslog=true,fast_unwind_on_malloc=0,disable_coredump=false,abort_on_error=true"

# sudo apt-get install libcunit1 libcunit1-doc libcunit1-dev
# sudo apt-get install libaio-dev
# sudo apt-get install libunbound-dev
# sudo apt-get install libssl-dev
# sudo apt-get install libunwind-dev
export

# ulimit -c unlimited
# sysctl kernel.core_pattern

APP=app

SUBDIRS:=lib qsc patches plthook funchook injector libpulp libuv userspace-rcu libev libelfmaster app 

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

run: $(SUBDIRS)
	$(MAKE) -C $(APP) $@

app-%:
	make -C $(APP) ${@:app-%=%} 

r-%:
	make -C $(APP) $@

t-%:
	make -C $(APP) $@

kt-%:
	make -C $(APP) $@

clean-%:
	make -C ${@:clean-%=%} clean

clean:
	rm -rf core .gdb_history temp*
	@for dir in $(SUBDIRS); do\
		$(MAKE) -C $$dir $@;\
	done

.PHONY: all clean $(SUBDIRS)
