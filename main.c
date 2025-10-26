#include <stdio.h>

int main(int argc, char **argv) {
    printf("Arguments: %d\n", argc);
    
    int i = 0;
    for (i = 1; i < argc; i++) {
        printf("Arg %d - %s\n", i, argv[i]);
    };

    return 0;
}

