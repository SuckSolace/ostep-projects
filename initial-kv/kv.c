#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "hashmap.h"
#include "common.h"

#define BUFF_SIZE 1024

typedef HASHMAP(char, char) string_map;

void write_all(string_map map1){
    assert((remove("database.txt") == 0));
    FILE* fp = fopen("database.txt", "w");
    assert(fp != NULL);
    const char * key, *val;

    int len;
    hashmap_foreach(key, val, &map1){
        //add ',' and '\n' and null character
        len = strlen(key) + strlen(val) + 3;
        char pre[len];
        snprintf(pre, len, "%s,%s\n", key, val);
        fwrite(pre, sizeof(char), len - 1, fp);
    }
}

void handle(char**params, string_map map1){
    int res;
    FILE* fp = fopen("database.txt", "ab+");
    if (strcmp(params[0], "p") == 0){
        res = hashmap_put(&map1, params[1], params[2]);
        if (res == -EEXIST) {
            //check if record needs to be updated
            const char * v = hashmap_get(&map1, params[1]);
            if (strcmp(params[2], v) == 0) printf("put key already exists\n");
            else {
                hashmap_remove(&map1, params[1]);
                hashmap_put(&map1, params[1], params[2]);
                printf("update key %s: %s => %s\n", params[1], v, params[2]);
            }
        }else printf("put %s: %s\n", params[1], params[2]);
    } else if (strcmp(params[0], "g") == 0){
        const char * v = (char *) hashmap_get(&map1, params[1]);
        if (v == NULL) printf("%s not found\n", params[1]);
        else printf("%s,%s\n", params[1], v);
    } else if (strcmp(params[0], "a") == 0){
        const char *key, *val;
        hashmap_foreach(key, val, &map1) {
            printf("%s => %s\n", key, val);
        }
    } else if (strcmp(params[0], "c") == 0){
        hashmap_clear(&map1);
        hashmap_reset(&map1);
        puts("remove all key-value pairs from the database");
    } else if (strcmp(params[0], "d") == 0){
        const char * removed = hashmap_remove(&map1, params[1]);
        if (removed != NULL){
            printf("remove %s: %s\n", params[1], removed);
        }
    } else {
        printf("no such argument: %s\n", params[0]);
    }
    puts("==========================");
    fclose(fp);
}

int main(int argc, char**argv){
    if (argc < 2) return 0;
    //single operator or operand
    char *ptr, *ptr2;
    FILE *fp = fopen("database.txt", "ab+");

    size_t len = 0;
    char *line = NULL;
    //hashmap string->string
    string_map map1;
    /* Initialize a map with case-sensitive string keys */
    hashmap_init(&map1, hashmap_hash_string, strcmp);

    //read data from file
    while ((getline(&line, &len, fp) != -1)){
        line[strlen(line) - 1] = '\0';
        ptr = strsep(&line, ",");
        ptr2 = strsep(&line, ",");
        //printf("k: %s->v: %s\n", ptr, ptr2);
        hashmap_put(&map1, ptr, ptr2);
    }

    fclose(fp);
    char **params = (char **) Malloc(sizeof(char*) * 4);
    int i;
    for (int j = 1; j < argc; j ++){
        i = 0;
        memset(params, 0, 4 * sizeof(char *));
        //sep param
        while ((ptr = strsep(&argv[j], ",")) != NULL)
            params[i++] = ptr;
        handle(params, map1);
    }
    //flush all records to disk
    write_all(map1);
    hashmap_cleanup(&map1);
    free(params);
    return 0;
}
