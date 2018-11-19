lib:  $(LIBRARY)

TEST_BIN = $(OBJ_DIR)/test$(BUILD_FPRINT).bin
CLIENT_BIN = $(OBJ_DIR)/client$(BUILD_FPRINT).bin
SERVER_BIN = $(OBJ_DIR)/server$(BUILD_FPRINT).bin

BINARIES += $(TEST_BIN)
BINARIES += $(CLIENT_BIN)
BINARIES += $(SERVER_BIN)

all: $(BINARIES)

server: $(SERVER_BIN)
	./$(OBJ_DIR)/$@$(BUILD_FPRINT).elf

client: $(CLIENT_BIN)
	./$(OBJ_DIR)/$@$(BUILD_FPRINT).elf

test: $(TEST_BIN)
	./$(OBJ_DIR)/$@$(BUILD_FPRINT).elf
