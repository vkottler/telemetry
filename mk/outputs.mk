lib: $(LIBRARY)

run-%: $(OBJ_DIR)/%$(BUILD_FPRINT).elf
	./$<

all: $(BINARIES)
