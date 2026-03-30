ifeq ($(OS),Windows_NT)
  SRC_DIR = src/windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    SRC_DIR = src/macos
  else
    SRC_DIR = src/linux
  endif
endif

ifneq ($(filter arm64,$(MAKECMDGOALS)),)
	include make/arm64.mk
else ifneq ($(filter arm,$(MAKECMDGOALS)),)
	include make/arm.mk
endif

$(info $(CC))
export

arm arm64: all

all:
	cd $(SRC_DIR) && $(MAKE)
	cd cmd && $(MAKE)

check:
	cd tests && $(MAKE) check

clean:
	cd $(SRC_DIR) && $(MAKE) clean
	cd cmd && $(MAKE) clean
	cd tests && $(MAKE) clean
