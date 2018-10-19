.PHONY: clean lib test
.SECONDARY:
.DEFAULT_GOAL = lib

# load build configuration
include config.mk

# toolchain options
include mk/toolchain.mk

# source file discovery
include src/conf.mk

OBJECTS = $(SRCS:.c=.o)

# up-to-date dependency trees
DEPS += $(SRCS:.c=.d)
-include $(DEPS)

# useful target declarations
include mk/rules.mk
