#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void search(char* needle, char* haystack){
    char *p1, *p2 = haystack;
    int len;
    while (*p2 != '\0'){
        len = 0;
        p1 = needle;
        while (*p1++ == *p2++) len ++;
        if (len == strlen(needle)) {fputs(haystack, stdout); break;};
    }
}

int main(int argc, char** argv){
    if (argc != 2 && argc != 3) {
        printf("wgrep: searchterm [file ...]\n");
        exit(1);
    }
    if (strcmp(argv[1], "") == 0) return 0;
    size_t len = 0;
    char * line;
    if (argc == 2) { //from stdin
        while (1){
            getline(&line, &len, stdin);
            if (strcmp(line, "\n") == 0 || feof(stdin)) break;
            search(argv[1], line);
        }
    }else{
        FILE* fp = fopen(argv[2], "r");
        if (fp == NULL){
            puts("wgrep: cannot open file");
            exit(1);
        }
        while ((getline(&line, &len, fp)) != -1){
            search(argv[1], line);
        }
        fclose(fp);
    }
    return 0;
}
