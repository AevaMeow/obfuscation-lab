#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OP_LOAD_CHAR  0xA7
#define OP_PUSH_CONST 0x3D
#define OP_CMP_EQ     0x64
#define OP_AND        0x9B
#define OP_RET        0x2C
#define OP_JUNK       0xD1
#define OP_DEAD       0x6D

static unsigned char vm_key(size_t position)
{
    return (unsigned char)(((position * 17u) + 0x5Au) ^ 0xA5u);
}

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *data;

    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        fclose(file);
        return NULL;
    }

    data = (char *)malloc((size_t)size + 1);
    if (!data ||
        fread(data, 1, (size_t)size, file) != (size_t)size) {
        free(data);
        fclose(file);
        return NULL;
    }

    data[size] = '\0';
    fclose(file);
    return data;
}

static char *extract_key(const char *source)
{
    const char *start = strstr(source, "expected");
    const char *end;
    char *key;
    size_t length;

    if (!start ||
        !(start = strchr(start, '=')) ||
        !(start = strchr(start, '"'))) {
        return NULL;
    }

    start++;
    end = strchr(start, '"');
    if (!end) {
        return NULL;
    }

    length = (size_t)(end - start);
    key = (char *)malloc(length + 1);
    if (!key) {
        return NULL;
    }

    memcpy(key, start, length);
    key[length] = '\0';
    return key;
}

static unsigned char *compile_key(const char *key, size_t *code_length)
{
    size_t key_length = strlen(key);
    unsigned char *code =
        (unsigned char *)malloc(key_length * 14 + 8);
    size_t p = 0;
    size_t i;

    if (!code) {
        return NULL;
    }

    for (i = 0; i < key_length; i++) {
        code[p++] = OP_JUNK;
        code[p++] = (unsigned char)(0x30u + i * 13u);

        if (i % 3 == 1) {
            code[p++] = OP_DEAD;
            code[p++] = 0;
        }

        code[p++] = OP_LOAD_CHAR;
        code[p++] = (unsigned char)i;
        code[p++] = OP_JUNK;
        code[p++] = (unsigned char)(0x80u ^ i * 7u);
        code[p++] = OP_PUSH_CONST;
        code[p++] = (unsigned char)key[i];
        code[p++] = OP_CMP_EQ;

        if (i != 0) {
            code[p++] = OP_AND;
        }
    }

    code[p++] = OP_RET;
    *code_length = p;
    return code;
}

static int write_header(const char *path,
                        const unsigned char *code,
                        size_t code_length)
{
    FILE *file = fopen(path, "wb");
    size_t i;

    if (!file) {
        return 0;
    }

    fprintf(file,
            "#ifndef GENERATED_BYTECODE_H\n"
            "#define GENERATED_BYTECODE_H\n\n"
            "static unsigned char b0[] = {\n");

    for (i = 0; i < code_length; i++) {
        fprintf(file,
                "%s0x%02X%s",
                i % 8 == 0 ? "    " : "",
                (unsigned int)(code[i] ^ vm_key(i)),
                i + 1 == code_length ? "" : ", ");

        if (i % 8 == 7 || i + 1 == code_length) {
            fputc('\n', file);
        }
    }

    fprintf(file,
            "};\n\n"
            "#define VM_BYTECODE_SIZE ((unsigned int)sizeof(b0))\n\n"
            "#endif\n");

    fclose(file);
    return 1;
}

int main(int argc, char **argv)
{
    char *source;
    char *key;
    unsigned char *code;
    size_t code_length;
    int result;

    if (argc != 3) {
        return 1;
    }

    source = read_file(argv[1]);
    key = source ? extract_key(source) : NULL;
    free(source);

    code = key ? compile_key(key, &code_length) : NULL;
    result = code && write_header(argv[2], code, code_length);

    free(code);
    free(key);
    return result ? 0 : 1;
}
