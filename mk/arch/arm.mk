TOOLCHAIN = arm-none-eabi-
CFLAGS   += -Os -fno-builtin
LFLAGS   += --specs=nosys.specs -Wl,--gc-sections
#LFLAGS += -Tproc/link.ld
