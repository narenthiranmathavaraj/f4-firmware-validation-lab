CROSS   ?= arm-none-eabi-
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
SIZE     = $(CROSS)size

TARGET   = build/f4-bringup
SRCS     = $(wildcard src/*.c src/*/*.c)
OBJS     = $(patsubst %.c,build/%.o,$(SRCS))

# soft-float on purpose: no FPU enable sequence needed for bring-up work
ARCH     = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS   = $(ARCH) -Og -g3 -ffreestanding -fno-common -ffunction-sections -fdata-sections \
           -Wall -Wextra -Wshadow -Isrc
LDFLAGS  = $(ARCH) -nostdlib -nostartfiles -T linker/stm32f4.ld -Wl,--gc-sections \
           -Wl,-Map=$(TARGET).map

.PHONY: all clean run test gdb
all: $(TARGET).elf
	@$(SIZE) $<

$(TARGET).elf: $(OBJS) linker/stm32f4.ld
	$(CC) $(OBJS) $(LDFLAGS) -lgcc -o $@

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# interactive: opens Renode monitor with the UART analyzer
run: all
	renode platform/f4_bringup.resc

sim: all
	renode --disable-xwt --plain --console platform/run_headless.resc
	@cat build/uart.log
# headless Robot tests
test: all
	renode-test tests/smoke.robot tests/tmp108.robot tests/flash.robot tests/faults.robot

# GDB against Renode's gdb server (start `make run`, then `gdb` in another shell)
gdb: all
	gdb-multiarch $(TARGET).elf -ex "target remote :3333"

clean:
	rm -rf build

-include $(OBJS:.o=.d)
