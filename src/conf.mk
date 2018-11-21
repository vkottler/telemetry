# this invocation's build configuration
include src/arch/$(ARCH).mk
include src/target/$(TARGET).mk

# discover source files
SRCS += $(wildcard src/lib/*.c)
SRCS += $(wildcard src/arch/$(ARCH)/*.c)
