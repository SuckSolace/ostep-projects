#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char** argv){
    if (argc == 1) {
        puts("wunzip: file1 [file2 ...]");
        exit(1);
    }
    //read char
    char c;
    //read int
    int num;
    FILE *fp;
    for (int i = 1; i < argc; i ++){
        fp = fopen(argv[i], "r");
        assert(fp != NULL);

        while ((fread(&num, sizeof(int), 1, fp) == 1)){
            fread(&c, sizeof(char), 1, fp);
            while (num --){
                putchar(c);
            }
        }
        fclose(fp);
    }
    return 0;
}
