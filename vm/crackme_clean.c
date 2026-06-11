#include <stdio.h>
#include <string.h>

static int check_format(const char *key)
{
    if (strlen(key) != 19) {
        return 0;
    }

    if (key[4] != '-' || key[9] != '-' || key[14] != '-') {
        return 0;
    }

    return 1;
}

int check_key(const char *key)
{
    const char *expected = "K1R1-0BF5-VM42-C0DE";

    if (!check_format(key)) {
        return 0;
    }

    return strcmp(key, expected) == 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <key>\n", argv[0]);
        return 1;
    }

    if (check_key(argv[1])) {
        puts("Correct");
        return 0;
    }

    puts("Wrong");
    return 1;
}
