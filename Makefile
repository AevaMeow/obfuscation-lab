CC = gcc
CFLAGS = -O0 -fno-stack-protector -no-pie -Wall -Wextra

.PHONY: all regenerate clean

all: crackme_clean crackme_ida solver

crackme_clean: src/crackme_clean.c
	$(CC) $(CFLAGS) src/crackme_clean.c -o crackme_clean

bytecode_generator: src/bytecode_generator.c
	$(CC) $(CFLAGS) src/bytecode_generator.c -o bytecode_generator

src/generated_bytecode.h: src/crackme_clean.c bytecode_generator
	./bytecode_generator src/crackme_clean.c src/generated_bytecode.h

regenerate: bytecode_generator
	./bytecode_generator src/crackme_clean.c src/generated_bytecode.h

crackme_ida: src/crackme_vm.c src/generated_bytecode.h
	$(CC) $(CFLAGS) src/crackme_vm.c -o crackme_ida
	strip crackme_ida

solver: src/solver.c
	$(CC) $(CFLAGS) src/solver.c -o solver

clean:
	rm -f bytecode_generator src/generated_bytecode.h crackme_clean crackme_ida solver
