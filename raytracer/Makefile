# directories

# srouce file diretory
SRC_DIR = src
# top-level output directory (for .o and .d files)
TOP_OBJ_DIR = bin

# default to debug
ifeq ($(MODE),)
	MODE = debug
endif

include sources.mk

.PHONY: all clean

all: target

clean:
	rm -rf $(TOP_OBJ_DIR) $(TARGET)

include make.mk

