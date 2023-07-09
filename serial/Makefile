CROSS_COMPILE=arm-none-eabi-
#CROSS_COMPILE =/opt/OSELAS.Toolchain-2011.11.0/arm-cortexm3-eabi/gcc-4.6.2-newlib-1.19.0-binutils-2.21.1a/bin/arm-cortexm3-eabi-
#CROSS_COMPILE =/opt/arm-2011.03/bin/arm-none-eabi-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
AS=$(CROSS_COMPILE)as
OC=$(CROSS_COMPILE)objcopy
OD=$(CROSS_COMPILE)objdump
SZ=$(CROSS_COMPILE)size

CFLAGS= -c -fno-common \
	-ffunction-sections \
	-fdata-sections \
	-O3 \
	-mcpu=cortex-m3 -Wall -Wno-main \
	-mthumb \

LDSCRIPT=./stm32_serial.ld
LDFLAGS = --gc-sections,-T$(LDSCRIPT),-lnosys
OCFLAGS = -Obinary
ODFLAGS = -S
OUTPUT_DIR = ./
TARGET  = $(OUTPUT_DIR)/serial

# INCLUDE = -I./ \
#       -I./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/inc \
#       -I./FreeRTOSv9.0.0/FreeRTOS/Source/portable/GCC/ARM_CM3 \
#       -I./FreeRTOSv9.0.0/FreeRTOS/Source/include \
#       -I./CMSIS/CM3/DeviceSupport/ST/STM32F10x \
#       -I./CMSIS/CM3/CoreSupport \
#       -I./src \
#       -I./src/console \
#       -I./src/console/rcli \
#       -I./src/hw_rev_adc \
#       -I./src/hw_voltage_adc \
#       -I./src/mdb

SRCS =  ./serial.c

OBJS=$(SRCS:.c=.o)

.PHONY : all clean cleanall

all: $(TARGET).bin  $(TARGET).list
	$(SZ) $(TARGET).elf

cleanall:
	-find . -name '*.o'   -exec rm {} \;
	-find . -name '*.elf' -exec rm {} \;
	-find . -name '*.lst' -exec rm {} \;
	-find . -name '*.out' -exec rm {} \;
	-find . -name '*.bin' -exec rm {} \;
	-find . -name '*.map' -exec rm {} \;

clean:
	rm -fv ./*.elf
	rm -fv ./*.lst
	rm -fv ./*.out
	rm -fv ./*.bin
	rm -fv ./*.map

$(TARGET).list: $(TARGET).elf
	$(OD) $(ODFLAGS) $< > $(TARGET).lst

$(TARGET).bin: $(TARGET).elf
	$(OC) $(OCFLAGS) $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(OBJS)
	@$(CC) -mcpu=cortex-m3 -mthumb -Wl,$(LDFLAGS),-o$(TARGET).elf,-Map,$(TARGET).map $(OBJS)

%.o: %.c
	@echo "  CC $<"
	@$(CC) $(INCLUDE) $(CFLAGS)  $< -o $*.o

%.o: %.S
	@echo "  CC $<"
	@$(CC) $(INCLUDE) $(CFLAGS)  $< -o $*.o