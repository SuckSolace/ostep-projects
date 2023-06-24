#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "common.h"
#include "common_threads.h"

void *run(void *args){
    char* filename = (char*) args;
    char c, prev = '\0';
    int cnt = 0;
    FILE *fp = fopen(filename, "r");
    assert(fp != NULL);
    while ((c = fgetc(fp)) && !feof(fp)){
        if (prev == c){
            cnt ++;
        } else {
            if (prev) {
                fwrite(&cnt, sizeof(int), 1, stdout);
                fputc(prev, stdout);
            }
            cnt = 1;
            prev = c;

        }
    }
    fclose(fp);
    if (cnt > 0){
        fwrite(&cnt, sizeof(int), 1, stdout);
        fputc(prev, stdout);
    }
}

int main(int argc, char** argv){
    if (argc == 1) {
        puts("pzip: file1 [file2 ...]");
        exit(1);
    }
    pthread_t threads[argc - 1];
    for (int i = 0; i < argc - 1; i ++ )
        Pthread_create(&threads[i], NULL, &run, (void *)argv[i + 1]);

    for (int i = 0; i < argc - 1; i ++ )
        Pthread_join(threads[i], NULL);
    return 0;
}
