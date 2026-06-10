#include <stdio.h>
#include <string.h>

#include "generated_bytecode.h"

static volatile unsigned int g0 = 0x9E3779B9u;

static unsigned char s0[] = {
    0x1F, 0x33, 0x2E, 0x2E, 0x39, 0x3F, 0x28, 0x56
};

static unsigned char s1[] = {
    0x7E, 0x5B, 0x46, 0x47, 0x4E, 0x23
};

static unsigned char s2[] = {
    0x62, 0x44, 0x56, 0x50, 0x52, 0x0D, 0x17, 0x54,
    0x45, 0x56, 0x54, 0x5C, 0x5A, 0x52, 0x17, 0x0B,
    0x5C, 0x52, 0x4E, 0x09, 0x3D
};

static void f0(const unsigned char *p, unsigned int n, unsigned char k)
{
    unsigned int i;

    for (i = 0; i < n; i++) {
        putchar((p[i] ^ k) ^ (unsigned char)(g0 & 0u));
    }
}

static unsigned char f1(unsigned int i)
{
    volatile unsigned int x = i;

    x = (x * 17u) + 0x5Au;
    x ^= 0xA5u;
    x ^= (g0 & 0u);
    return (unsigned char)x;
}

static unsigned char f2(unsigned int *ip)
{
    unsigned int p = *ip;

    *ip = p + 1;
    return b0[p] ^ f1(p);
}

static int f4(const char *p)
{
    unsigned int h = 0x1337u;
    int i;

    for (i = 0; p[i]; i++) {
        h = ((h << 5) ^ (unsigned char)p[i]) + (unsigned int)i;
        h ^= h >> 7;
    }

    return h == 0x32E91u;
}

static int f5(const char *p)
{
    volatile unsigned int x = (unsigned int)strlen(p) + (g0 & 0u);

    return ((x * x + x) & 1u) != 0u;
}

static int f6(const char *input)
{
    unsigned int ip = 0;
    int sp = 0;
    int stack[256];
    volatile unsigned int shadow = g0;

    while (1) {
        unsigned char op = f2(&ip);

        switch (op) {
        case 0xA7: {
            int idx = f2(&ip);
            stack[sp++] = (unsigned char)input[idx];
            break;
        }

        case 0x3D: {
            int value = f2(&ip);
            stack[sp++] = value;
            break;
        }

        case 0xE1: {
            int b = stack[--sp];
            int a = stack[--sp];
            stack[sp++] = a ^ b;
            break;
        }

        case 0x64: {
            int b = stack[--sp];
            int a = stack[--sp];
            stack[sp++] = a == b;
            break;
        }

        case 0x9B: {
            int b = stack[--sp];
            int a = stack[--sp];
            stack[sp++] = a && b;
            break;
        }

        case 0xD1: {
            unsigned int v = f2(&ip);
            shadow = (shadow * 33u) ^ v;
            break;
        }

        case 0x6D: {
            unsigned int off = f2(&ip);
            volatile unsigned int x = shadow ^ (g0 & 0u);

            if (((x * x + x) & 1u) != 0u) {
                ip += off;
            }
            break;
        }

        case 0x2C:
            return sp > 0 ? stack[--sp] : 0;

        default:
            return f4(input);
        }
    }
}

static int f3(const char *p)
{
    volatile int z = p[0] ^ p[0];
    size_t n = strlen(p);
    int st = 0;
    int r = 0;
    int fuel = 16;

    while (fuel-- > 0) {
        switch (st) {
        case 0:
            st = n == (size_t)((3 * 7) - 2 + z) ? 1 : 9;
            break;

        case 1:
            st = p[(2 * 2) + z] == '-' ? 2 : 9;
            break;

        case 2:
            st = p[((int)n ^ (int)n) + 9] == '-' ? 3 : 9;
            break;

        case 3:
            st = p[(7 + 7) + z] == '-' ? 8 : 9;
            break;

        case 8:
            r = 1;
            st = 10;
            break;

        case 9:
            r = 0;
            st = 10;
            break;

        case 10:
            return r;

        default:
            return 0;
        }
    }

    return 0;
}

static int f7(const char *key)
{
    int st = 0;
    int r = 0;
    int fuel = 32;

    while (fuel-- > 0) {
        switch (st) {
        case 0:
            st = f3(key) ? 1 : 9;
            break;

        case 1:
            st = f5(key) ? 4 : 2;
            break;

        case 2:
            r = f6(key);
            st = 8;
            break;

        case 4:
            r = f4(key);
            st = 8;
            break;

        case 9:
            r = 0;
            st = 8;
            break;

        case 8:
            return r;

        default:
            return 0;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        f0(s2, (unsigned int)sizeof(s2), 0x37);
        return 1;
    }

    if (f7(argv[1])) {
        f0(s0, (unsigned int)sizeof(s0), 0x5C);
        return 0;
    }

    f0(s1, (unsigned int)sizeof(s1), 0x29);
    return 1;
}
