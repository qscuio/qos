PYTHON ?= python3
BUILD_TOOL := $(CURDIR)/tools/build_all.py

.PHONY: all strict clean

all:
	$(PYTHON) $(BUILD_TOOL)

strict:
	$(PYTHON) $(BUILD_TOOL) --strict

clean:
	rm -rf $(CURDIR)/build

