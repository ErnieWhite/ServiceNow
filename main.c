#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [ticket number]", argv[0]);
        return 0;
    }

    int i;
    for (i=1; i < argc; i++) {
        printf("Arg %d\t%s\n",i, argv[i]);
    }

    return 0;



}
