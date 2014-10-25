talker.bin: talker.c
	gcc -std=c99 -Wall -Wextra -pedantic -DEBUG talker.c -o talker.bin
