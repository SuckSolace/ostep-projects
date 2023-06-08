#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

int main(int argc, char** argv){
    if (argc == 1) {
        puts("wzip: file1 [file2 ...]");
        exit(1);
    }
    char c, prev = '\0';
    int cnt = 0;
    FILE *fp;
    for (int i = 1; i < argc; i ++){
        fp = fopen(argv[i], "r");
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
    }
    if (cnt > 0){
        fwrite(&cnt, sizeof(int), 1, stdout);
        fputc(prev, stdout);
    }

    return 0;
}
