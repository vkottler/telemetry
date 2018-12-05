# global configurations
OBJ_DIR      = bin
BUILD_FPRINT = _$(ARCH)_$(TARGET)
LIBRARY_NAME = telemetry$(BUILD_FPRINT)
LIBRARY      = $(OBJ_DIR)/lib$(LIBRARY_NAME).a
INCLUDES    += -I include
CFLAGS      += $(INCLUDES) -Wall -Wextra -Werror -pedantic -std=c99
CFLAGS      += -D_POSIX_C_SOURCE=200809L -Wno-format
CXXFLAGS    += $(CFLAGS) -std=gnu++14
LFLAGS      += -L$(OBJ_DIR) -l$(LIBRARY_NAME) -lpthread

# this invocation's build configuration
include mk/arch/$(ARCH).mk
include mk/target/$(TARGET).mk
