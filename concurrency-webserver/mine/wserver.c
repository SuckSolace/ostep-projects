#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include "common_threads.h"

#define MAXBUF (8196)

char default_root[] = ".";

sem_t empty, fill, mutex;
int use = 0, prod = 0, count = 0;
FILE* logfd;

typedef struct _pthread_args {
    fd_inf* buf;
    int tot;
    char* schedalg;
} pthread_args;

void put(fd_inf* val, fd_inf buf[], int tot){
    buf[prod] = *val;
    prod = (prod + 1) % tot;
}

void get(fd_inf buf[], int tot, fd_inf *ff){
    *ff = buf[use];
    use = (use + 1) % tot;
}

void sffget(fd_inf buf[], int tot, fd_inf *ff){
    off_t minsz = buf[0].file_sz;
    int minidx = 0;
    printf("prod: %d\n", prod);
    for (int i = 1; i < prod; i++){
        if (minsz >= buf[i].file_sz){
            minsz = buf[i].file_sz;
            minidx = i;
        }
    }
    ff->is_static = buf[minidx].is_static;
    ff->fd = buf[minidx].fd;
    ff->file_sz = buf[minidx].file_sz;
    ff->filename = malloc(sizeof(char) * MAXBUF);
    strcpy(ff->filename, buf[minidx].filename);
    ff->cgiargs = malloc(sizeof(char) * MAXBUF);
    strcpy(ff->cgiargs, buf[minidx].cgiargs);
    ff->sbuf = malloc(sizeof(struct stat));
    memcpy(ff->sbuf, buf[minidx].sbuf, sizeof(struct stat));
    printf("filename: %s fd: %d addr: %p\n", ff->filename, ff->fd, ff);
    for (int i = minidx + 1; i < prod; i ++ )
        memcpy(&buf[i - 1], &buf[i], sizeof(fd_inf));
    prod = (prod - 1 + tot) % tot;
}

void getfilesize(int fd, fd_inf * fif){
    int is_static;
    struct stat *sbuf = malloc(sizeof(struct stat));
    char* filename = malloc(sizeof(char) * MAXBUF);
    char* cgiargs = malloc(sizeof(char) * MAXBUF);
    char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];

    readline_or_die(fd, buf, MAXBUF);
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("method:%s uri:%s version: %s\n", method, uri, version);

    if (strcasecmp(method, "GET")){
        request_error(fd, method, "501", "Not Implemented", "server does not implement this method");
        return;
    }
    request_read_headers(fd);

    is_static = request_parse_uri(uri, filename, cgiargs);

    if (stat(filename, sbuf) < 0){
        request_error(fd, filename, "404", "Not found", "server could not find this file");
        return;
    }

    //get file size
    off_t sz = sbuf->st_size;
    fif->fd = fd;
    fif->file_sz = sz;
    fif->is_static = is_static;
    fif->sbuf = sbuf;
    fif->filename = filename;
    fif->cgiargs = cgiargs;
}

void free_fd_inf(fd_inf * fif){
    free(fif->filename);
    free(fif->sbuf);
    free(fif->cgiargs);
    free(fif);
}

void* consumer(void * args){
    pthread_args *inf = (pthread_args*) args;
    fd_inf* buf = inf->buf;
    char* schedalg = inf->schedalg;
    int tot = inf->tot;
    while (1){
        Sem_wait(&fill);
        Sem_wait(&mutex);
        fd_inf* tmp = malloc(sizeof(fd_inf));
        printf("p: %p\n", tmp);
        if (strcmp(schedalg, "FIFO") == 0)
            get(buf, tot, tmp);
        else if(strcmp(schedalg, "SFF") == 0){
            sffget(buf, tot, tmp);
        }
        //printf("filename: %s\n", tmp->filename);
        logfd = fopen("log", "a+");
        fprintf(logfd, "pid: %lu\n", pthread_self());
        fclose(logfd);
        Sem_post(&mutex);
        request_handle(tmp);
        close_or_die(tmp->fd);
        free_fd_inf(tmp);
        Sem_post(&empty);
    }
}


// ./wserver [-d basedir] [-p port] [-t threads] [-b buffers] [-s schedalg]
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
    int threads = 1;
    int buffers = 1;
    char *schedalg = malloc(sizeof(char) * 5);
    strcpy(schedalg, "FIFO");

    while ((c = getopt(argc, argv, "d:p:t:b:s:")) != -1)
        switch (c) {
            case 'd':
                root_dir = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                threads = atoi(optarg);
                break;
            case 'b':
                buffers = atoi(optarg);
                break;
            case 's':
                schedalg = optarg;
                break;
            default:
                fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffers] [-s schedalg]");
                exit(1);
        }
    // run out of this directory
    chdir_or_die(root_dir);

    //create connection descriptor buffers
    fd_inf buf[buffers];
    //master thread
    int listen_fd = open_listen_fd_or_die(port);
    //create a pool of worker threads
    pthread_t workers[threads];

    //consumer thread args
    pthread_args args;
    args.buf = buf;
    args.tot = buffers;
    args.schedalg = schedalg;


    for (int i = 0; i < threads; i ++)
        Pthread_create(&workers[i], NULL, &consumer, &args);

    logfd = fopen("log", "w");
    fclose(logfd);
    //init sems
    sem_init(&empty, 0, buffers);
    sem_init(&fill, 0, 0);
    sem_init(&mutex, 0, 1); //lock
    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);

        Sem_wait(&empty);
        Sem_wait(&mutex);
        fd_inf *fif = malloc(sizeof(fd_inf));
        getfilesize(conn_fd, fif);
        put(fif, buf, buffers);
        Sem_post(&mutex);
        Sem_post(&fill);
    }
    //free resources
    free(schedalg);
    sem_close(&empty);
    sem_close(&fill);
    sem_close(&mutex);
    return 0;
}
