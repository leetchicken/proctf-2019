OPTS = -c -std=gnu++98 -fPIC -fno-rtti -fno-exceptions -fno-builtin -funsigned-char -fno-delete-null-pointer-checks -fomit-frame-pointer -O2 -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=softfp -Wall
NAME = main_screen

all:
	arm-none-eabi-g++ $(OPTS) main_screen.cpp -o main_screen.o
	arm-none-eabi-g++ $(OPTS) notifications.cpp -o notifications.o
	arm-none-eabi-ld main_screen.o notifications.o -N -Ttext 0 -o $(NAME).elf
	arm-none-eabi-objcopy -O binary $(NAME).elf $(NAME).bin
	python3 merge.py $(NAME)

clean:
	rm *.o *.elf *.bin