lib: $(LIBRARY)

run-%: $(OBJ_DIR)/%$(BUILD_FPRINT).elf
	./$<

BINARIES := $(foreach BINARY, $(BINARY_SRCS), $(OBJ_DIR)/$(BINARY)$(BUILD_FPRINT).bin)
all: $(BINARIES)
