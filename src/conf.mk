# this invocation's build configuration
include src/arch/$(ARCH).mk
include src/target/$(TARGET).mk

SRCS += $(wildcard src/lib/*.c)
