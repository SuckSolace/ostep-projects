#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "common.h"

#define PERR fprintf(stderr, "An error has occurred\n")

char **paths, **tokens;
int paths_len;
int max_tokens, token_count;

char ** split(char* line);
int handle_redirect(char **tokens);
void handlecmd(char* line);
int handle_builtin(char** tokens);
void run(char **tokens);

char** split(char* line){
    const char delimiter[] = " ";

    max_tokens = 8;
    // Allocate memory for an array of char* pointers
    char** tokens = (char**)malloc(sizeof(char*) * max_tokens);
    token_count = 0;

    char* token = strtok(line, delimiter);

    while (token != NULL) {
        if (token_count >= max_tokens){
            max_tokens *= 2;
            tokens = (char**)realloc(tokens, sizeof(char*) * max_tokens);
        }
        // Allocate memory for each token and copy it
        tokens[token_count] = (char*)malloc(sizeof(char) * (strlen(token) + 1));
        strcpy(tokens[token_count], token);
        token_count++;

        token = strtok(NULL, delimiter);
    }
    return tokens;
}

//0: no redirect
int handle_redirect(char** tokens){
    char**p = tokens;
    int flag = 0;
    while (*p != NULL){
        if (flag == 1){
            //multiple redirect symbols || multiple files
            if (strcmp(*p, ">") == 0 || *(p + 1) != NULL){
                PERR;
                exit(0);
            }
        }else if(strcmp(*p, ">") == 0)
            flag = 1;
        p ++;
    }
    if (flag){
        int fd = open(*(p - 1), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
        *(p - 1) = NULL;
        *(p - 2) = NULL;
        return fd;
    }
    return 0;
}

void run(char** tokens){
    int pid = fork();
    if (pid == -1) {
        PERR;
        return;
    }
    if (pid == 0) { //child process
        char* full_path = NULL;
        //sleep(15);
        size_t full_path_len;
        int valid_path = 0;
        for (int i = 0; i < paths_len; i ++){
            full_path_len = sizeof(char) * (strlen(paths[i]) + strlen(*tokens) + 2);
            full_path = realloc(full_path, full_path_len);
            snprintf(full_path, full_path_len, "%s/%s", paths[i], *tokens);
            if (access(full_path, X_OK) == -1){
                PERR;
                return;
            } else {
                valid_path = 1;
                break;
            }
        }
        if (!valid_path) return;
        int fd = handle_redirect(tokens);
        int stdoutcp, stderrcp;
        if (fd) {
            stdoutcp = dup(1);
            stderrcp = dup(2);
            dup2(1, fd);
            dup2(2, fd);
            close(1);
            close(2);
        }
        int res = execvp(full_path, tokens);
        if (fd) {
            dup2(stdoutcp, 1);
            dup2(stderrcp, 2);
            close(stdoutcp);
            close(stderrcp);
        }
        if (res == -1) {
            PERR;
            return;
        }
        free(full_path);
    } else
        waitpid(pid, NULL, 0);
}

int handle_builtin(char **tokens){
    if (strcmp(*tokens, "exit") == 0) {
        //have args, error
        if (*(tokens + 1) != NULL){
            PERR;
            return 0;
        } else
            exit(0);
    } else if (strcmp(*tokens, "cd") == 0) {
        //0 or >1 args should be signaled as an error
        if (*(tokens + 2) != NULL || *(tokens + 1) == NULL)
            PERR;
        else {
            if (chdir(tokens[1]) == -1)
                PERR;
        }
        return 0;
    } else if (strcmp(*tokens, "path") == 0) {
        while (paths_len)
            free(paths[-- paths_len]);
        free(paths);

        //how many new paths
        char **p = tokens + 1;
        while (*p++ != NULL) paths_len ++;

        paths = Malloc(sizeof(char*) * paths_len);
        for (int i = 0; i < paths_len; i ++) {
            paths[i] = Malloc(sizeof(char) * (strlen(tokens[i + 1]) + 1));
            strcpy(paths[i], tokens[i + 1]);
        }
        return 0;
    }
    return 1;
}

void handlecmd(char* line){
    line[strlen(line) - 1] = '\0';
    tokens = split(line);
    int res = handle_builtin(tokens);
    if (res) {
        if (paths_len) run(tokens);
        else PERR;
    }
}

int main(int argc, char* argv[]){
    if (argc > 2){
        PERR;
        exit(1);
    }
    //set search path
    paths = malloc(sizeof(char*));
    paths[0] = malloc(sizeof(char) * (strlen("/bin") + 1));
    strcpy(paths[0], "/bin");
    paths_len = 1;

    char* line;
    size_t len = 0;
    if (argc == 1) { //interactive mode
        while (1){
            fputs("wish> ", stdout);
            if ((getline(&line, &len, stdin)) == -1)
                PERR;
            handlecmd(line);
        }
        free(line);
    } else if (argc == 2) { //batch mode
        FILE *fp = fopen(argv[1], "r");
        if (fp == NULL) {PERR; return 0;}
        while ((getline(&line, &len, fp)) != -1)
            handlecmd(line);
        free(fp);
        free(line);
    }
    if (tokens != NULL) {
        // Free the allocated memory
        for (int i = 0; i < token_count; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
    //free paths
    for (int i = 0; i < paths_len; i ++)
        free(paths[i]);
    free(paths);
    return 0;
}
