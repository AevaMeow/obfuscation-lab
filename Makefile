CC = gcc
CFLAGS = -O0 -fno-stack-protector -no-pie -Wall -Wextra

.PHONY: all clean

all: bin/crackme_ida solver/solver generator/bytecode_generator

bin/crackme_ida: vm/crackme_vm.c
	mkdir -p bin
	$(CC) $(CFLAGS) vm/crackme_vm.c -o bin/crackme_ida
	strip --strip-all bin/crackme_ida

solver/solver: solver/solver.c
	$(CC) $(CFLAGS) solver/solver.c -o solver/solver -lz3

generator/bytecode_generator: generator/bytecode_generator.c
	$(CC) $(CFLAGS) generator/bytecode_generator.c -o generator/bytecode_generator

clean:
	rm -f bin/crackme_clean bin/crackme_ida solver/solver generator/bytecode_generator
