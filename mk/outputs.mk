lib: $(LIBRARY)

run-%: $(OBJ_DIR)/%$(BUILD_FPRINT).elf
	./$<

# automatically discover source files in src/app and build them
BINARY_SRCS := $(patsubst src/app/%, %, $(patsubst %.c, %, $(wildcard src/app/*.c)))
BINARIES := $(foreach BINARY, $(BINARY_SRCS), $(OBJ_DIR)/$(BINARY)$(BUILD_FPRINT).bin)

all: $(BINARIES)
