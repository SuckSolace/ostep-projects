#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
    if (argc == 1) return 0;
    FILE *fp;
    size_t len;
    char *line;
    for (int i = 1; i < argc; i ++){
        fp = fopen(argv[i], "r");
        if (fp == NULL){
            fprintf(stdout, "wcat: cannot open file\n");
            exit(1);
        }
        len = 0;
        line = NULL;
        while ((getline(&line, &len, fp) != -1))
            fputs(line, stdout);
    }
    fclose(fp);
    return 0;
}
