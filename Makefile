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
	-O0 \
	-ggdb3 \
	-mcpu=cortex-m3 -Wall -Wno-main \
	-mthumb \
	-DUSE_STDPERIPH_DRIVER \
	-DSTM32F10X_MD

LDSCRIPT=./stm32f103rc.ld
LDFLAGS	= --gc-sections,-T$(LDSCRIPT),-lnosys
OCFLAGS	= -Obinary
ODFLAGS	= -S
OUTPUT_DIR = output
TARGET  = $(OUTPUT_DIR)/main

INCLUDE = -I./ \
	  -I./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/inc \
	  -I./FreeRTOSv9.0.0/FreeRTOS/Source/portable/GCC/ARM_CM3 \
	  -I./FreeRTOSv9.0.0/FreeRTOS/Source/include \
	  -I./CMSIS/CM3/DeviceSupport/ST/STM32F10x \
	  -I./CMSIS/CM3/CoreSupport \
	  -I./src \
	  -I./src/console \
	  -I./src/console/rcli \
	  -I./src/hw_rev_adc \
	  -I./src/hw_voltage_adc \
	  -I./src/mdb \
	  -I./src/cctalk

SRCS = 	./CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c \
	./stm32f10x_it.c \
	./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
	./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
	./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c \
	./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c \
	./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c \
	./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/STM32F10x_StdPeriph_Driver/src/misc.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/croutine.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/portable/MemMang/heap_4.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/list.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/queue.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/event_groups.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/tasks.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/timers.c \
	./FreeRTOSv9.0.0/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c \
	./src/console/console.c \
	./src/console/rcli/rcli.c \
	./src/console/rcli/buf.c \
	./src/syscalls.c \
	./src/hw_rev_adc/hw_rev_adc.c \
	./src/hw_adc.c \
	./src/hw_voltage_adc/hw_voltage_adc.c \
	./src/mdb/mdb.c \
	./src/mdb/cashless.c \
	./src/cctalk/cctalk.c \
	./src/cctalk/coinbox.c \
	./main.c

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
	rm -fv ./*.o ./src/*.o ./src/console/*.o ./src/console/rcli/*.o ./src/mdb/*.o ./src/cctalk/*.o
	rm -fv ./output/*.elf
	rm -fv ./output/*.lst
	rm -fv ./output/*.out
	rm -fv ./output/*.bin
	rm -fv ./output/*.map

$(TARGET).list: $(TARGET).elf
	$(OD) $(ODFLAGS) $< > $(TARGET).lst

$(TARGET).bin: $(TARGET).elf
	$(OC) $(OCFLAGS) $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(OBJS) ./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/gcc_ride7/startup_stm32f10x_md.o
	@$(CC) -mcpu=cortex-m3 -mthumb -Wl,$(LDFLAGS),-o$(TARGET).elf,-Map,$(TARGET).map ./STM32F10x_StdPeriph_Lib_V3.6.0/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/gcc_ride7/startup_stm32f10x_md.o $(OBJS)

%.o: %.c FreeRTOSConfig.h
	@echo "  CC $<"
	@$(CC) $(INCLUDE) $(CFLAGS)  $< -o $*.o

%.o: %.S
	@echo "  CC $<"
	@$(CC) $(INCLUDE) $(CFLAGS)  $< -o $*.o