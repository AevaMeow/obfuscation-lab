#include <stdio.h>
#include <string.h>

enum vm_opcode {
    VM_OP_LOAD_CHAR = 0xA7,
    VM_OP_PUSH_CONST = 0x3D,
    VM_OP_XOR = 0xE1,
    VM_OP_CMP_EQ = 0x64,
    VM_OP_AND = 0x9B,
    VM_OP_RET = 0x2C,
    VM_OP_JUNK = 0xD1,
    VM_OP_DEAD = 0x6D
};

static unsigned char encrypted_vm_bytecode[] = {
    0x2E, 0xFE, 0x7E, 0x28, 0x06, 0x67, 0x84, 0xA5, 
    0xC7, 0x6B, 0x87, 0xD4, 0x52, 0xAF, 0x80, 0xFC, 
    0x68, 0xDF, 0x14, 0xB2, 0xEA, 0xCB, 0xF2, 0x79, 
    0xEC, 0xC2, 0x2A, 0x51, 0xD9, 0x45, 0xFF, 0xF1, 
    0x78, 0xCF, 0xE8, 0x86, 0x26, 0x9F, 0x21, 0xCF, 
    0x76, 0xE1, 0x26, 0x93, 0xDE, 0x36, 0x2C, 0x0D, 
    0xBA, 0x03, 0xFC, 0x7C, 0xF0, 0xAB, 0x31, 0xC9, 
    0xB7, 0x21, 0x95, 0xDD, 0x12, 0x23, 0x0C, 0xB0, 
    0x02, 0xC2, 0x7D, 0xF3, 0xAA, 0x3B, 0x02, 0xB1, 
    0xBA, 0x68, 0x00, 0x21, 0x60, 0xEF, 0xE3, 0x58, 
    0x94, 0xCF, 0x17, 0xDF, 0x4D, 0x67, 0xAE, 0x65, 
    0x46, 0x4C, 0xCC, 0x99, 0xB7, 0xB9, 0xEC, 0x87, 
    0x72, 0x6E, 0xDE, 0x4F, 0x66, 0x92, 0x64, 0x45, 
    0x56, 0xCB, 0xBF, 0xB4, 0xB8, 0xE3, 0x95, 0xBB, 
    0x67, 0x43, 0x1C, 0xB9, 0x7A, 0x02, 0xA8, 0x84, 
    0x93, 0x5D, 0x00, 0x85, 0x94, 0x0B, 0x20, 0x1E, 
    0x9E, 0x9F, 0xE6, 0x95, 0xE4, 0xEE, 0x7E, 0x25, 
    0x75, 0xBB, 0x21, 0x97, 0x09, 0x2F, 0xE2, 0x9D, 
    0x9E, 0x98, 0x94, 0x61, 0xEF, 0x01, 0x24, 0x7B, 
    0x70, 0x2D, 0x0C, 0xAC, 0xF2, 0xB3, 0xB0, 0x71, 
    0xBE, 0xCA, 0x22, 0x59, 0x57, 0x4D, 0xC9, 0xE9, 
    0xEE, 0xD7, 0xD0, 0xC4, 0x5E, 0x8F, 0x29, 0xC7, 
    0x7E, 0x67, 0xE4, 0x98, 0x4C, 0xF7, 0xE8, 0xC2, 
    0xD6, 0xD7, 0xCA, 0x5D, 0xA7, 0x26, 0xC6, 0x7D, 
    0x59, 0x29, 0x97, 0xD5, 0xF8, 0x2B, 0xF4, 0xD6, 
    0x3A, 0x38, 0x05, 0xEB, 0x92, 0xA1, 0x0A, 0xB3, 
    0xB2, 0xBE, 0x08, 0x29, 0x22, 0xE7, 0x56, 0x60, 
    0x8C, 0xB7, 0x71, 0x2D, 0x53, 0x05, 0xAD, 0xB1, 
    0xA2, 0x0F, 0x28, 0x38, 0xE6, 0x27, 0x61, 0x8F, 
    0xB6, 0x7B, 0xE6, 0x41, 0x9E, 0xE8, 0x6C, 0x4D, 
    0x18, 0xC3, 0xD7, 0xBC, 0xB0, 0xEB, 0x0F, 0xC3, 
    0x65, 0x7B, 0x26, 0x41, 0x62, 0x7C, 0xA0, 0xDE, 
    0x9B, 0x55, 0xF5
};

static volatile unsigned int opaque_seed = 0x9E3779B9u;

static unsigned char encrypted_correct_message[] = {
    0x1F, 0x33, 0x2E, 0x2E, 0x39, 0x3F, 0x28, 0x56
};

static unsigned char encrypted_wrong_message[] = {
    0x7E, 0x5B, 0x46, 0x47, 0x4E, 0x23
};

static unsigned char encrypted_usage_message[] = {
    0x62, 0x44, 0x56, 0x50, 0x52, 0x0D, 0x17, 0x54,
    0x45, 0x56, 0x54, 0x5C, 0x5A, 0x52, 0x17, 0x0B,
    0x5C, 0x52, 0x4E, 0x09, 0x3D
};

static void print_encrypted_message(const unsigned char *encrypted_data,
                                    unsigned int len,
                                    unsigned char xor_key)
{
    unsigned int i;

    for (i = 0; i < len; i++) {
        putchar((encrypted_data[i] ^ xor_key) ^
                (unsigned char)(opaque_seed & 0u));
    }
}

static unsigned char vm_keystream_byte(unsigned int position)
{
    volatile unsigned int value = position;

    value = (value * 17u) + 0x5Au;
    value ^= 0xA5u;
    value ^= (opaque_seed & 0u);
    return (unsigned char)value;
}

static unsigned char vm_fetch_byte(unsigned int *ip)
{
    unsigned int position = *ip;

    *ip = position + 1;
    return encrypted_vm_bytecode[position] ^
           vm_keystream_byte(position);
}

static int fake_key_check(const char *input)
{
    unsigned int hash = 0x1337u;
    int i;

    for (i = 0; input[i]; i++) {
        hash = ((hash << 5) ^ (unsigned char)input[i]) +
               (unsigned int)i;
        hash ^= hash >> 7;
    }

    return hash == 0x32E91u;
}

static int opaque_predicate(const char *input)
{
    volatile unsigned int input_len =
        (unsigned int)strlen(input) + (opaque_seed & 0u);

    return ((input_len * input_len + input_len) & 1u) != 0u;
}

static int execute_vm(const char *input)
{
    unsigned int ip = 0;
    int stack_p = 0;
    int stack[256];
    volatile unsigned int shadow_state = opaque_seed;

    while (1) {
        unsigned char opcode =
            vm_fetch_byte(&ip);

        switch (opcode) {
        case VM_OP_LOAD_CHAR: {
            int i = vm_fetch_byte(&ip);
            stack[stack_p++] = (unsigned char)input[i];
            break;
        }

        case VM_OP_PUSH_CONST: {
            int constant = vm_fetch_byte(&ip);
            stack[stack_p++] = constant;
            break;
        }

        case VM_OP_XOR: {
            int right = stack[--stack_p];
            int left = stack[--stack_p];
            stack[stack_p++] = left ^ right;
            break;
        }

        case VM_OP_CMP_EQ: {
            int right = stack[--stack_p];
            int left = stack[--stack_p];
            stack[stack_p++] = left == right;
            break;
        }

        case VM_OP_AND: {
            int right = stack[--stack_p];
            int left = stack[--stack_p];
            stack[stack_p++] = left && right;
            break;
        }

        case VM_OP_JUNK: {
            unsigned int junk_value = vm_fetch_byte(&ip);
            shadow_state =
                (shadow_state * 33u) ^ junk_value;
            break;
        }

        case VM_OP_DEAD: {
            unsigned int dead_offset = vm_fetch_byte(&ip);
            volatile unsigned int opaque_dead =
                shadow_state ^ (opaque_seed & 0u);

            if (((opaque_dead * opaque_dead + opaque_dead) & 1u) != 0u) {
                ip += dead_offset;
            }
            break;
        }

        case VM_OP_RET:
            return stack_p > 0 ? stack[--stack_p] : 0;

        default:
            return fake_key_check(input);
        }
    }
}

static int check_key_format_flattened(const char *input)
{
    volatile int zero = input[0] ^ input[0];
    size_t input_len = strlen(input);
    int state = 0;
    int result = 0;
    int fuel = 16;

    while (fuel-- > 0) {
        switch (state) {
        case 0:
            state = input_len ==
                    (size_t)((3 * 7) - 2 + zero) ? 1 : 9;
            break;

        case 1:
            state = input[(2 * 2) + zero] == '-' ? 2 : 9;
            break;

        case 2:
            state = input[((int)input_len ^
                           (int)input_len) + 9] == '-' ? 3 : 9;
            break;

        case 3:
            state = input[(7 + 7) + zero] == '-' ? 8 : 9;
            break;

        case 8:
            result = 1;
            state = 10;
            break;

        case 9:
            result = 0;
            state = 10;
            break;

        case 10:
            return result;

        default:
            return 0;
        }
    }

    return 0;
}

static int check_key(const char *input)
{
    int state = 0;
    int result = 0;
    int fuel = 32;

    while (fuel-- > 0) {
        switch (state) {
        case 0:
            state = check_key_format_flattened(input) ? 1 : 9;
            break;

        case 1:
            state = opaque_predicate(input) ? 4 : 2;
            break;

        case 2:
            result = execute_vm(input);
            state = 8;
            break;

        case 4:
            result = fake_key_check(input);
            state = 8;
            break;

        case 9:
            result = 0;
            state = 8;
            break;

        case 8:
            return result;

        default:
            return 0;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        print_encrypted_message(
            encrypted_usage_message,
            (unsigned int)sizeof(encrypted_usage_message),
            0x37);
        return 1;
    }

    if (check_key(argv[1])) {
        print_encrypted_message(
            encrypted_correct_message,
            (unsigned int)sizeof(encrypted_correct_message),
            0x5C);
        return 0;
    }

    print_encrypted_message(
        encrypted_wrong_message,
        (unsigned int)sizeof(encrypted_wrong_message),
        0x29);
    return 1;
}
