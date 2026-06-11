#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <z3.h>

#define OP_LOAD_CHAR  0xA7
#define OP_PUSH_CONST 0x3D
#define OP_XOR        0xE1
#define OP_CMP_EQ     0x64
#define OP_AND        0x9B
#define OP_RET        0x2C
#define OP_JUNK       0xD1
#define OP_DEAD       0x6D

#define KEY_LEN 19
#define STACK_SIZE 256

static unsigned char vm_key(size_t i)
{
    return (unsigned char)(((i * 17u) + 0x5Au) ^ 0xA5u);
}

static unsigned char vm_byte(const unsigned char *code, size_t i)
{
    return code[i] ^ vm_key(i);
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static unsigned char *parse_hex(const char *text, size_t *code_len)
{
    size_t text_len = strlen(text);
    unsigned char *code;
    size_t i;

    if (text_len == 0 || text_len % 2 != 0) {
        return NULL;
    }

    *code_len = text_len / 2;
    code = (unsigned char *)malloc(*code_len);
    if (!code) {
        return NULL;
    }

    for (i = 0; i < *code_len; i++) {
        int high = hex_digit(text[i * 2]);
        int low = hex_digit(text[i * 2 + 1]);

        if (high < 0 || low < 0) {
            free(code);
            return NULL;
        }

        code[i] = (unsigned char)((high << 4) | low);
    }

    return code;
}

static int run_vm(Z3_context ctx,
                  Z3_solver solver,
                  Z3_sort byte_sort,
                  Z3_ast key[KEY_LEN],
                  const unsigned char *code,
                  size_t code_len,
                  int *instruction_count,
                  int *comparison_count)
{
    Z3_ast stack[STACK_SIZE];
    size_t ip = 0;
    int stack_p = 0;

    *instruction_count = 0;
    *comparison_count = 0;

    while (ip < code_len) {
        unsigned char op = vm_byte(code, ip++);
        Z3_ast left;
        Z3_ast right;

        (*instruction_count)++;

        switch (op) {
        case OP_LOAD_CHAR: {
            unsigned char i;

            if (ip >= code_len) {
                return 0;
            }

            i = vm_byte(code, ip++);
            if (i >= KEY_LEN || stack_p >= STACK_SIZE) {
                return 0;
            }

            stack[stack_p++] = key[i];
            break;
        }

        case OP_PUSH_CONST:
            if (ip >= code_len || stack_p >= STACK_SIZE) {
                return 0;
            }
            stack[stack_p++] =
                Z3_mk_unsigned_int(ctx,
                                   vm_byte(code, ip++),
                                   byte_sort);
            break;

        case OP_XOR:
            if (stack_p < 2) {
                return 0;
            }
            right = stack[--stack_p];
            left = stack[--stack_p];
            stack[stack_p++] = Z3_mk_bvxor(ctx, left, right);
            break;

        case OP_CMP_EQ:
            if (stack_p < 2) {
                return 0;
            }
            right = stack[--stack_p];
            left = stack[--stack_p];
            stack[stack_p++] = Z3_mk_eq(ctx, left, right);
            (*comparison_count)++;
            break;

        case OP_AND: {
            Z3_ast args[2];

            if (stack_p < 2) {
                return 0;
            }

            args[1] = stack[--stack_p];
            args[0] = stack[--stack_p];
            stack[stack_p++] = Z3_mk_and(ctx, 2, args);
            break;
        }

        case OP_JUNK:
        case OP_DEAD:
            if (ip >= code_len) {
                return 0;
            }
            ip++;
            break;

        case OP_RET:
            if (stack_p < 1) {
                return 0;
            }
            Z3_solver_assert(ctx, solver, stack[--stack_p]);
            return 1;

        default:
            return 0;
        }
    }

    return 0;
}

static int solve(const unsigned char *code,
                 size_t code_len,
                 char key_text[KEY_LEN + 1],
                 int *instruction_count,
                 int *comparison_count)
{
    Z3_config cfg = Z3_mk_config();
    Z3_context ctx = Z3_mk_context(cfg);
    Z3_solver solver = Z3_mk_solver(ctx);
    Z3_sort byte_sort = Z3_mk_bv_sort(ctx, 8);
    Z3_ast key[KEY_LEN];
    Z3_model model = NULL;
    int ok = 0;
    size_t i;

    Z3_del_config(cfg);
    Z3_solver_inc_ref(ctx, solver);

    for (i = 0; i < KEY_LEN; i++) {
        char name[16];

        snprintf(name, sizeof(name), "key_%02zu", i);
        key[i] = Z3_mk_const(
            ctx,
            Z3_mk_string_symbol(ctx, name),
            byte_sort);
    }

    if (!run_vm(ctx,
                solver,
                byte_sort,
                key,
                code,
                code_len,
                instruction_count,
                comparison_count) ||
        Z3_solver_check(ctx, solver) != Z3_L_TRUE) {
        goto cleanup;
    }

    model = Z3_solver_get_model(ctx, solver);
    Z3_model_inc_ref(ctx, model);

    for (i = 0; i < KEY_LEN; i++) {
        Z3_ast value;
        unsigned int byte;

        if (!Z3_model_eval(ctx, model, key[i], Z3_TRUE, &value) ||
            !Z3_get_numeral_uint(ctx, value, &byte)) {
            goto cleanup;
        }

        key_text[i] = (char)byte;
    }

    key_text[KEY_LEN] = '\0';
    ok = 1;

cleanup:
    if (model) {
        Z3_model_dec_ref(ctx, model);
    }
    Z3_solver_dec_ref(ctx, solver);
    Z3_del_context(ctx);
    return ok;
}

int main(int argc, char **argv)
{
    const char *hex;
    unsigned char *code;
    size_t code_len;
    char key[KEY_LEN + 1];
    int instruction_count;
    int comparison_count;
    int quiet = 0;

    if (argc == 3 && strcmp(argv[1], "-q") == 0) {
        quiet = 1;
        hex = argv[2];
    } else if (argc == 2) {
        hex = argv[1];
    } else {
        fprintf(stderr, "Usage: %s [-q] HEX\n", argv[0]);
        return 1;
    }

    code = parse_hex(hex, &code_len);
    if (!code ||
        !solve(code,
               code_len,
               key,
               &instruction_count,
               &comparison_count)) {
        fprintf(stderr, "Cannot solve VM bytecode\n");
        free(code);
        return 1;
    }

    if (!quiet) {
        printf("Bytecode: %zu bytes\n", code_len);
        printf("Instructions: %d\n", instruction_count);
        printf("Comparisons: %d\n", comparison_count);
        printf("Z3: sat\n");
    }

    puts(key);
    free(code);
    return 0;
}
