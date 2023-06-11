#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "common.h"

#define PERR2(func) do { \
    /*fprintf(stdout, "%s: An error has occurred\n", #func);*/ \
    fprintf(stderr, "An error has occurred\n"); \
} while (0)


const char redirect_symbol_char = '>';
const char redirect_symbol_str[] = ">";
const char parallel_symbol_char = '&';
const char parallel_symbol_str[] = "&";
const char command_delimiter[] = " ";

char **paths, **tokens;
int paths_len;
int max_tokens, token_count;

void handle_parallel(char* line);
char ** split(char* line, const char delimiter[], int parallel);
int handle_redirect(char **tokens);
void handlecmd(char* line);
int handle_builtin(char** tokens);
void run(char **tokens); //actually run the cmd


char** split(char* line, const char delimiter[], int parallel){
    max_tokens = 8;
    // Allocate memory for an array of char* pointers
    char** tokens = (char**)Malloc(sizeof(char*) * max_tokens);
    token_count = 0;

    char* token = strtok(line, delimiter);

    while (token != NULL) {
        char * redir = NULL;
        if (strcmp(token, redirect_symbol_str) != 0)
            redir = strchr(token, redirect_symbol_char);
        //possibility that there is no space for redirection symbol
        if (redir != NULL && !parallel) {
            int pos = redir - token;
            tokens[token_count] = Malloc(sizeof(char) * (pos + 1));
            strncpy(tokens[token_count], token, pos);
            token_count ++;
            if (token_count >= max_tokens){
                max_tokens += 3;
                tokens = (char**)realloc(tokens, sizeof(char*) * max_tokens);
            }
            tokens[token_count] = (char*)Malloc(sizeof(char) * 2);
            strcpy(tokens[token_count ++], redirect_symbol_str);
            tokens[token_count] = (char*)Malloc(sizeof(char) * strlen(redir));
            strncpy(tokens[token_count ++], redir + 1, strlen(redir));
        }else {
            if (token_count >= max_tokens){
                max_tokens *= 2;
                tokens = (char**)realloc(tokens, sizeof(char*) * max_tokens);
            }
            // Allocate memory for each token and copy it
            tokens[token_count] = (char*)Malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(tokens[token_count], token);
            token_count++;
        }

        token = strtok(NULL, delimiter);
    }
    if (parallel) return tokens;
    //filter all ""
    int count = 0, max_count = token_count;
    char** t = tokens;
    char** p = (char**)Malloc(sizeof(char*) * max_count);
    while (*t != NULL)
    {
        if (strcmp(*t, "") != 0){
            p[count] = (char*)Malloc(sizeof(char) * (strlen(*t) + 1));
            strcpy(p[count], *t);
            count ++;
        }
        t ++;
    }
    for (int i = 0; i < token_count; i ++) free(tokens[i]);
    tokens = realloc(tokens, sizeof(char*) * (token_count + 1));
    memcpy(tokens, p, sizeof(char*) * token_count);
    max_tokens = token_count;
    token_count = count;
    free(p);
    return tokens;
}

//0: no redirect
int handle_redirect(char** tokens){
    char**p = tokens;
    int flag = 0;
    while (*p != NULL){
        if (flag == 1){
            //multiple redirect symbols || multiple files
            if (strcmp(*p, redirect_symbol_str) == 0 || *(p + 1) != NULL){
                PERR2(handle_redirect);
                exit(0);
            }
        }else if(strcmp(*p, redirect_symbol_str) == 0) {
            //no file
            if (*(p + 1) == NULL) {PERR2(handle_redirect); exit(1);}
            flag = 1;
        }
        p ++;
    }
    if (flag){
        int fd = open(*(p - 1), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1) {PERR2(handle_redirect); exit(1);}
        *(p - 1) = NULL;
        *(p - 2) = NULL;
        return fd;
    }
    return 0;
}

void run(char** tokens){
    char* full_path = NULL;
    //sleep(15);
    size_t full_path_len;
    int valid_path = 0;
    for (int i = 0; i < paths_len; i ++){
        full_path_len = sizeof(char) * (strlen(paths[i]) + strlen(*tokens) + 2);
        full_path = realloc(full_path, full_path_len);
        snprintf(full_path, full_path_len, "%s/%s", paths[i], *tokens);
        if (access(full_path, X_OK) != -1){
            valid_path = 1;
            break;
        }
    }
    if (!valid_path) {PERR2(run); exit(1);}
    int fd = handle_redirect(tokens);
    int stdoutcp, stderrcp;
    if (fd) {
        stdoutcp = dup(1);
        stderrcp = dup(2);
        if (dup2(fd, 1) == -1) {PERR2(run); exit(1);}
        if (dup2(fd, 2) == -1) {PERR2(run); exit(1);}
    }
    execvp(full_path, tokens);
    if (fd) {
        if (dup2(1, stdoutcp) == -1) {PERR2(run); exit(1);}
        if (dup2(2, stderrcp) == -1) {PERR2(run); exit(1);}
        close(stdoutcp);
        close(stderrcp);
        close(fd);
    }
    free(full_path);
}

int handle_builtin(char **tokens){
    if (strcmp(*tokens, "exit") == 0) {
        //have args, error
        if (*(tokens + 1) != NULL){
            PERR2(handle_builtin);
            return 0;
        } else
            exit(0);
    } else if (strcmp(*tokens, "cd") == 0) {
        //0 or >1 args should be signaled as an error
        if (*(tokens + 2) != NULL || *(tokens + 1) == NULL)
            PERR2(handle_builtin);
        else {
            if (chdir(tokens[1]) == -1)
                PERR2(handle_builtin);
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


void handle_parallel(char* line){
    char** cmds = split(line, parallel_symbol_str, 1);
    char** p = cmds;
    int pid;
    while (*p != NULL) {
        tokens = split(*p, command_delimiter, 0);
        if (*tokens == NULL) return;
        int res = handle_builtin(tokens);
        if (res) {
            if (paths_len) {
                if ((pid = fork()) == -1) PERR2(handle_parallel);
                if (pid == 0) run(tokens);
            }
            else PERR2(handle_parallel);
        }
        p ++;
    }
    //wait for all processes to finish
    while ((pid = wait(NULL)) > 0) ;
}


void handlecmd(char* line){
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';
    if (strchr(line, parallel_symbol_char) != NULL) handle_parallel(line);
    else {
        tokens = split(line, command_delimiter, 0);
        if (*tokens == NULL) return;
        int res = handle_builtin(tokens);
        if (res) {
            if (paths_len) {
                int pid;
                if ((pid = fork()) == -1) PERR2(handlecmd);
                if (pid == 0) run(tokens);
                wait(NULL);
            }
            else PERR2(handlecmd);
        }
    }
}

int main(int argc, char* argv[]){
    if (argc > 2){
        PERR2(main);
        exit(1);
    }
    //set search path
    paths = Malloc(sizeof(char*));
    paths[0] = Malloc(sizeof(char) * (strlen("/bin") + 1));
    strcpy(paths[0], "/bin");
    paths_len = 1;

    char* line;
    size_t len = 0;
    if (argc == 1) { //interactive mode
        while (1){
            fputs("wish> ", stdout);
            if ((getline(&line, &len, stdin)) == -1)
                PERR2(main);
            handlecmd(line);
        }
        free(line);
    } else if (argc == 2) { //batch mode
        FILE *fp = fopen(argv[1], "r");
        if (fp == NULL) {PERR2(main); return 1;}
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
