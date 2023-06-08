#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "common.h"

int main(int argc, char** argv){
    if (argc > 3){
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }
    list_t *myls = list_new();
    size_t len = 0;
    char *line;
    FILE* fout, *fp;
    //no arg, read from stdin
    if (argc == 1) {
        fout = fopen("output.txt", "ab+");
        if (fout == NULL){
            fprintf(stderr, "open output.txt fail\n");
            exit(1);
        }
        while (1){
            getline(&line, &len, stdin);
            if (strcmp(line, "\n") == 0)
                break;
            else
                list_lpush(myls, list_node_new(strdup(line)));
        }
    } else {
        if (argc == 2) fout = fopen("output.txt", "w+");
        else if (argc == 3) {
            if (strcmp(argv[1], argv[2]) == 0)
            {
                fprintf(stderr, "reverse: input and output file must differ\n");
                exit(1);
            }

            fout = fopen(argv[2], "w+");
        }
        fp = fopen(argv[1], "r");
        if (fout == NULL){
            fprintf(stderr, "open output.txt fail\n");
            exit(1);
        }
        if (fp == NULL){
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            exit(1);
        }
        while ((getline(&line, &len, fp)) != -1){
            list_lpush(myls, list_node_new(strdup(line)));
        }

    }
    //for iteration
    list_node_t *nd;
    list_iterator_t *it = list_iterator_new(myls, LIST_HEAD);
    while ((nd = list_iterator_next(it))){
        fputs(nd->val, fout);
    }

    list_iterator_destroy(it);
    list_destroy(myls);
    if (argc > 2) fclose(fp);
    fclose(fout);

    free(line);
    return 0;
}
