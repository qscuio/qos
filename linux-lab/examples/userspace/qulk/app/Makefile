FILES=$(wildcard *.c)
OBJS=$(FILES:.c=.o)
BINS=binaries
BIN_OBJS=$(addprefix $(BINS)/,$(OBJS))
BIN_EXES=$(basename $(BIN_OBJS))
LINKER_FLAGS="-TLinker.lds"
APP_LDFLAGS+=-lplthook_elf -lfunchook -linjector -lpulp -lurcu -lurcu-bp \
	     -lurcu-mb -lurcu-common -lurcu-memb -lurcu-cds -lurcu-qsbr \
	     -lunicorn -lelfmaster -luv -lev -lssh

RUST_FILES=$(wildcard *.rs)
RUST_BIN_OBJS=$(addprefix $(BINS)/rust-,$(RUST_FILES))
RUST_BIN_EXES=$(basename $(RUST_BIN_OBJS))

ASAN_ENV=ASAN_OPTIONS=$(ASAN)

all: _prepare $(BIN_OBJS) $(BIN_EXES) $(RUST_BIN_EXES)

_prepare:
	mkdir -p $(BINS)

$(BINS)/%.o: %.c
	$(CC) -c $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@

$(BINS)/%: $(BINS)/%.o
	$(CC) $(LINKER_FLAGS) -o $@ $< $(APP_LDFLAGS)
	nm $@ > $@.map

$(BINS)/rust-%:
	rustc ./${@:$(BINS)/rust-%=%}.rs -o $@

%: $(BINS)/%
	@

r-%:
	$(ASAN_ENV) ./$(BINS)/${@:r-%=%}

t-%:
	sudo uftrace -a -b 128K -D 30 -l --no-pager -e --event-full -R --retval ./$(BINS)/${@:t-%=%}

kt-%:
	sudo uftrace -a -b 128K -D 30 -l --no-pager -e --event-full -k -K 10 -R --retval ./$(BINS)/${@:kt-%=%}

run: $(BIN_EXES)
	@for exe in $(BIN_EXES); do \
		echo "==== Running $$exe start====="; \
		$(ASAN_ENV) ./$$exe; \
	done

clean:
	rm -rf $(BINS) *.o *.so *.log *.log.* core *.txt *.d

.PHONY: all clean

