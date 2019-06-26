OPTS = -c -std=gnu++98 -fPIC -fno-rtti -fno-exceptions -fno-builtin -funsigned-char -fno-delete-null-pointer-checks -fomit-frame-pointer -O2 -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=softfp

all:
	arm-none-eabi-g++ $(OPTS) test_game.cpp -o test_game.o
	arm-none-eabi-ld test_game.o -o test_game.elf
	arm-none-eabi-objcopy -O binary test_game.elf test_game.bin