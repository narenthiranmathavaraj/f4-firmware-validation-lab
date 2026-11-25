CROSS   ?= arm-none-eabi-
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
SIZE     = $(CROSS)size

TARGET   = build/f4-bringup
SRCS     = $(wildcard src/*.c src/*/*.c)
OBJS     = $(patsubst %.c,build/%.o,$(SRCS))

# soft-float on purpose: no FPU enable sequence needed for bring-up work
ARCH     = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CMSIS_CORE      ?= /opt/vendor/cmsis_core
CMSIS_DEVICE_F4 ?= /opt/vendor/cmsis_device_f4
FREERTOS_KERNEL ?= /opt/vendor/FreeRTOS-Kernel
FRT_INC = -Iconfig -isystem $(FREERTOS_KERNEL)/include -isystem $(FREERTOS_KERNEL)/portable/GCC/ARM_CM3

VENDOR_INC       = -DSTM32F407xx -isystem $(CMSIS_CORE)/Include -isystem $(CMSIS_DEVICE_F4)/Include
CFLAGS   = $(ARCH) $(VENDOR_INC) $(FRT_INC) -Og -g3 -ffreestanding -fno-common -ffunction-sections -fdata-sections \
           -Wall -Wextra -Wshadow -Isrc

LDFLAGS  = $(ARCH) -nostdlib -nostartfiles -T linker/stm32f4.ld -Wl,--gc-sections \
           -Wl,-Map=$(TARGET).map

FRT_SRCS = tasks.c queue.c list.c portable/MemMang/heap_4.c portable/GCC/ARM_CM3/port.c
FRT_OBJS = $(patsubst %.c,build/freertos/%.o,$(FRT_SRCS))

.PHONY: all clean run sim test gdb
all: $(TARGET).elf
	@$(SIZE) $<

$(TARGET).elf: $(OBJS) $(FRT_OBJS) linker/stm32f4.ld
	$(CC) $(OBJS) $(FRT_OBJS) $(LDFLAGS) -lgcc -o $@

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

build/freertos/%.o: $(FREERTOS_KERNEL)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# interactive: opens Renode monitor (needs a GUI)
run: all
	renode platform/f4_bringup.resc

# headless run: UART output goes to build/uart.log
sim: all
	@rm -f build/uart.log
	renode --disable-xwt --plain --console platform/run_headless.resc
	@cat build/uart.log

# headless Robot tests
test: all
	renode-test tests/smoke.robot tests/tmp108.robot tests/flash.robot tests/faults.robot

# GDB against Renode's gdb server (start `make run`, then `make gdb` in another shell)
gdb: all
	gdb-multiarch $(TARGET).elf -ex "target remote :3333"

clean:
	rm -rf build

-include $(OBJS:.o=.d) $(FRT_OBJS:.o=.d)