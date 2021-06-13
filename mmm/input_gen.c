/*
 * mmm.cpp input file format generator:
 *  Reduce trouble of generating adjacent weight matrix used by MMM.
 * 
 *  Usage: ./a.out [task num(s)]
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: ./a.out [task num(s)]");
        return -1;
    }

    int num = atoi(argv[1]);
    FILE *f = fopen("./input.txt", "w");

    fprintf(f, "%d\n", num);
    for (int i = 1; i <= num; ++i) {
        for (int j = i + 1; j <= num; ++j) {
            fprintf(f, "%d %d\n", i, j);
        }
    }

    fclose(f);
    return 0;
}
