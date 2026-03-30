SRC_FILES := $(wildcard *.c)
OBJ_FILES := $(patsubst %.c, %.o, $(SRC_FILES))
OBJ_FILES := $(filter-out plthook_osx.o plthook_win32.o, $(OBJ_FILES))
LIB_FILES := $(patsubst %.o, %.so, $(filter-out app.o, $(OBJ_FILES)))
LIB_FILES := $(addprefix lib, $(LIB_FILES))

CFLAGS = -fPIC -I. -I$(TOP)/userspace/funchook/ -I$(TOP)/userspace/funchook/include
LDFLAGS = -Wl,-rpath=$(TOP)/userspace/plthook -L$(TOP)/userspace/plthook  \
		  -Wl,-rpath=$(TOP)/userspace/funchook/build -L$(TOP)/userspace/funchook/build


all: $(LIB_FILES) app

app: app.o
	$(CC) -o app.out app.o $(LDFLAGS) -lplthook_elf -lfoo -lbar

libfh.so: libplthook_elf.so fh.o
	$(CC) -shared $(CFLAGS) fh.o $(LDFLAGS) -lbar -lplthook_elf -lfunchook -o $@

lib%.so: %.o

	$(CC) -shared $(CFLAGS) $< $(LDFLAGS) -lbar -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJ_FILES) $(LIB_FILES) app.out

