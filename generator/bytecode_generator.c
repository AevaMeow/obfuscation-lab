#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OP_LOAD_CHAR  0xA7
#define OP_PUSH_CONST 0x3D
#define OP_XOR        0xE1
#define OP_CMP_EQ     0x64
#define OP_AND        0x9B
#define OP_RET        0x2C
#define OP_JUNK       0xD1
#define OP_DEAD       0x6D

int main(int argc, char **argv)
{
    const char *key;
    size_t n, i, p = 0;
    unsigned char code[8192];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s KEY\n", argv[0]);
        return 1;
    }

    key = argv[1];
    n = strlen(key);

    for (i = 0; i < n; i++) {
        unsigned char mask = (unsigned char)(0x6Du + 29u * i);

        code[p++] = OP_JUNK;
        code[p++] = (unsigned char)(0x30u + i * 13u);
        if (i % 3 == 1) {
            code[p++] = OP_DEAD;
            code[p++] = 0;
        }
        code[p++] = OP_LOAD_CHAR;
        code[p++] = (unsigned char)i;
        code[p++] = OP_PUSH_CONST;
        code[p++] = mask;
        code[p++] = OP_XOR;
        code[p++] = OP_JUNK;
        code[p++] = (unsigned char)(0x80u ^ i * 7u);
        code[p++] = OP_PUSH_CONST;
        code[p++] = (unsigned char)((unsigned char)key[i] ^ mask);
        code[p++] = OP_CMP_EQ;
        if (i) {
            code[p++] = OP_AND;
        }
    }
    code[p++] = OP_RET;

    printf("static unsigned char encrypted_vm_bytecode[] = {\n   ");
    for (i = 0; i < p; i++) {
        code[i] ^= (unsigned char)(((i * 17u) + 0x5Au) ^ 0xA5u);
        printf(" 0x%02X,", code[i]);
        if ((i + 1) % 8 == 0 && i + 1 < p) {
            printf("\n   ");
        }
    }
    printf("\n};\n");
    return 0;
}
