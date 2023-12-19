PREFIX=arm-none-eabi
CC=$(PREFIX)-gcc
OBJCOPY=$(PREFIX)-objcopy
AR=$(PREFIX)-ar

COMMON_INCLUDES=-I.
COMMON_CFLAGS=-Os -ggdb3 -std=gnu99 -fno-common -ffunction-sections -fdata-sections -Wstrict-prototypes -Wundef -Wextra -Wshadow -Wredundant-decls #-Waddress-of-packed-member
COMMON_LDFLAGS=--static -lc -lm -Wl,--cref -Wl,--gc-sections #-Wl,--print-gc-sections

PHONY:all

all: vw_nc03.bin volvo_od2.bin qemu.bin

include Makefile_stm32f1
include Makefile_volvo_od2
include Makefile_audi_stm32feb
include Makefile_vw_nc03
include Makefile_qemu


%.vo : %.c
	$(CC) $(F103_CFLAGS) -c $< -o $@

example.bin: example.vo $(LIBOPENCM3_OBJS)
	$(CC) $^ -L. -Taudi_stm32feb/fw/x32feb.ld -L$(LIBOPENCM3_DIR)/lib/stm32/f1 $(F103_LDFLAGS) $(F103_INCLUDES) -Wl,-Map=example.map -o example.elf
	$(OBJCOPY) -O binary example.elf $@

flash_example: example.bin
	# openocd -f audi_stm32feb/fw/stm32feb_ftdi2232hl.cfg -c "init; reset halt" -c "mww 0x40022004 0x45670123" -c "mww 0x40022004 0xCDEF89AB" -c "mww 0x40022008 0x45670123" -c "mww 0x40022008 0xCDEF89AB" -c "mww 0x40022010 0x220" -c "mww 0x40022010 0x260" -c "sleep 100" -c "mww 0x40022010 0x230" -c "mwh 0x1ffff800 0x5AA5" -c "sleep 1000" -c "mww 0x40022010 0x2220" -c "sleep 100" -c "mdw 0x40022010" -c "mdw 0x4002201c" -c "mdw 0x1ffff800" -c targets -c "halt" -c "stm32f1x unlock 0;exit"
	# openocd -f audi_stm32feb/fw/stm32feb_ftdi2232hl.cfg -c "init; reset halt" -c "mww 0x40022004 0x45670123" -c "mww 0x40022004 0xCDEF89AB" -c "mww 0x40022008 0x45670123" -c "mww 0x40022008 0xCDEF89AB" -c targets -c "halt" -c "stm32f1x unlock 0; exit"
	openocd -f audi_stm32feb/fw/stm32feb_ftdi2232hl.cfg -c "init; mdw 0x1ffff7e0; mdw 0x1ffff800; reset halt; stm32f1x unlock 0; sleep 1; program example.bin 0x8000000; reset;exit"



clean:
	rm -rf *.bin *.map $(NUC131_BSP_OBJS) $(VW_NC03_OBJS) *.vwo $(LIBOPENCM3_OBJS) $(VOLVO_OD2_OBJS) *.vo $(QEMU_OBJS) *.qemu *.d *.elf

