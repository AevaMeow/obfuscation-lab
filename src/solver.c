#include <elf.h>
#include <stdint.h>
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

#define KEY_LEN 19
#define STACK_SIZE 256

struct file_image {
    unsigned char *data;
    size_t size;
};

struct section_view {
    const unsigned char *data;
    size_t size;
    uint64_t file_offset;
};

enum value_kind {
    VALUE_INPUT,
    VALUE_CONSTANT,
    VALUE_BOOLEAN
};

struct value {
    enum value_kind kind;
    unsigned int value;
    unsigned int checks;
};

static unsigned char vm_keystream(size_t position)
{
    return (unsigned char)(((position * 17u) + 0x5Au) ^ 0xA5u);
}

static unsigned char decode_at(const unsigned char *stream, size_t position)
{
    return stream[position] ^ vm_keystream(position);
}

static int read_file(const char *path, struct file_image *image)
{
    FILE *file = fopen(path, "rb");
    long size;

    if (!file) {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        fclose(file);
        return 0;
    }

    image->data = (unsigned char *)malloc((size_t)size);
    image->size = (size_t)size;

    if (!image->data ||
        fread(image->data, 1, image->size, file) != image->size) {
        free(image->data);
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

static int find_data_section(const struct file_image *image,
                             struct section_view *data)
{
    const Elf64_Ehdr *header;
    const Elf64_Shdr *sections;
    const Elf64_Shdr *names_section;
    const char *names;
    size_t i;

    if (image->size < sizeof(Elf64_Ehdr)) {
        return 0;
    }

    header = (const Elf64_Ehdr *)image->data;
    if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0 ||
        header->e_ident[EI_CLASS] != ELFCLASS64 ||
        header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return 0;
    }

    if (header->e_shoff +
            (uint64_t)header->e_shnum * sizeof(Elf64_Shdr) >
        image->size ||
        header->e_shstrndx >= header->e_shnum) {
        return 0;
    }

    sections = (const Elf64_Shdr *)(image->data + header->e_shoff);
    names_section = &sections[header->e_shstrndx];

    if (names_section->sh_offset + names_section->sh_size > image->size) {
        return 0;
    }

    names = (const char *)(image->data + names_section->sh_offset);

    for (i = 0; i < header->e_shnum; i++) {
        if (sections[i].sh_name >= names_section->sh_size ||
            strcmp(names + sections[i].sh_name, ".data") != 0) {
            continue;
        }

        if (sections[i].sh_offset + sections[i].sh_size > image->size) {
            return 0;
        }

        data->data = image->data + sections[i].sh_offset;
        data->size = sections[i].sh_size;
        data->file_offset = sections[i].sh_offset;
        return 1;
    }

    return 0;
}

static int push(struct value *stack, int *stack_p, struct value value)
{
    if (*stack_p >= STACK_SIZE) {
        return 0;
    }

    stack[(*stack_p)++] = value;
    return 1;
}

static int pop(struct value *stack, int *stack_p, struct value *value)
{
    if (*stack_p <= 0) {
        return 0;
    }

    *value = stack[--(*stack_p)];
    return 1;
}

static int execute_vm(const unsigned char *stream, size_t size, char *key)
{
    struct value stack[STACK_SIZE];
    unsigned char known[KEY_LEN] = {0};
    size_t p = 0;
    int stack_p = 0;

    memset(key, '?', KEY_LEN);
    key[KEY_LEN] = '\0';

    while (p < size) {
        unsigned char opcode = decode_at(stream, p++);
        struct value a;
        struct value b;
        struct value result;

        switch (opcode) {
        case OP_LOAD_CHAR:
            if (p >= size) {
                return 0;
            }
            result.kind = VALUE_INPUT;
            result.value = decode_at(stream, p++);
            result.checks = 0;
            if (!push(stack, &stack_p, result)) {
                return 0;
            }
            break;

        case OP_PUSH_CONST:
            if (p >= size) {
                return 0;
            }
            result.kind = VALUE_CONSTANT;
            result.value = decode_at(stream, p++);
            result.checks = 0;
            if (!push(stack, &stack_p, result)) {
                return 0;
            }
            break;

        case OP_CMP_EQ:
            if (!pop(stack, &stack_p, &b) ||
                !pop(stack, &stack_p, &a) ||
                a.kind != VALUE_INPUT ||
                b.kind != VALUE_CONSTANT ||
                a.value >= KEY_LEN) {
                return 0;
            }

            key[a.value] = (char)b.value;
            known[a.value] = 1;

            result.kind = VALUE_BOOLEAN;
            result.value = 0;
            result.checks = 1;
            if (!push(stack, &stack_p, result)) {
                return 0;
            }
            break;

        case OP_AND:
            if (!pop(stack, &stack_p, &b) ||
                !pop(stack, &stack_p, &a) ||
                a.kind != VALUE_BOOLEAN ||
                b.kind != VALUE_BOOLEAN) {
                return 0;
            }

            result.kind = VALUE_BOOLEAN;
            result.value = 0;
            result.checks = a.checks + b.checks;
            if (!push(stack, &stack_p, result)) {
                return 0;
            }
            break;

        case OP_JUNK:
        case OP_DEAD:
            if (p >= size) {
                return 0;
            }
            p++;
            break;

        case OP_RET:
            if (!pop(stack, &stack_p, &result) ||
                result.kind != VALUE_BOOLEAN ||
                result.checks != KEY_LEN) {
                return 0;
            }

            for (p = 0; p < KEY_LEN; p++) {
                if (!known[p]) {
                    return 0;
                }
            }
            return 1;

        default:
            return 0;
        }
    }

    return 0;
}

static int recover_key(const struct section_view *data,
                       char *key,
                       size_t *bytecode_offset)
{
    size_t offset;

    for (offset = 0; offset < data->size; offset++) {
        if (execute_vm(data->data + offset, data->size - offset, key)) {
            *bytecode_offset = offset;
            return 1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *path = "crackme_ida";
    int quiet = 0;
    struct file_image image = {0};
    struct section_view data = {0};
    char key[KEY_LEN + 1];
    size_t bytecode_offset;
    size_t i;

    if (argc == 2) {
        if (strcmp(argv[1], "-q") == 0) {
            quiet = 1;
        } else {
            path = argv[1];
        }
    } else if (argc == 3 && strcmp(argv[1], "-q") == 0) {
        quiet = 1;
        path = argv[2];
    } else if (argc > 3) {
        return 1;
    }

    if (!read_file(path, &image) ||
        !find_data_section(&image, &data) ||
        !recover_key(&data, key, &bytecode_offset)) {
        free(image.data);
        return 1;
    }

    if (!quiet) {
        printf("[1] Opened ELF64: %s (%zu bytes)\n", path, image.size);
        printf("[2] Found .data: file offset 0x%llX, size %zu bytes\n",
               (unsigned long long)data.file_offset,
               data.size);
        printf("[3] Found VM bytecode: file offset 0x%llX\n",
               (unsigned long long)(data.file_offset + bytecode_offset));
        printf("[4] Recovered constraints:\n");

        for (i = 0; i < KEY_LEN; i++) {
            printf("    input[%2zu] == 0x%02X ('%c')\n",
                   i,
                   (unsigned char)key[i],
                   key[i]);
        }

        printf("[5] Recovered key:\n");
    }

    puts(key);
    free(image.data);
    return 0;
}
