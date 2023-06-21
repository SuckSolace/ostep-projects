#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include "common_threads.h"

char default_root[] = ".";

sem_t empty, fill, mutex;
int use = 0, prod = 0, count = 0;
FILE* logfd;

typedef struct _pthread_args {
    int* buf;
    int tot;
    char* schedalg;
} pthread_args;

void put(int val, int buf[], int tot){
    buf[prod] = val;
    prod = (prod + 1) % tot;
}


int get(int buf[], int tot){
    int val = buf[use];
    use = (use + 1) % tot;
    return val;
}


int sffget(int buf[], int tot){

}


void* consumer(void * args){
    pthread_args *inf = (pthread_args*) args;
    int* buf = inf->buf;
    int tot = inf->tot;
    char* schedalg = inf->schedalg;
    while (1){
        Sem_wait(&fill);
        Sem_wait(&mutex);
        int tmp;
        if (strcmp(schedalg, "FIFO") == 0)
            tmp = get(buf, tot);
        else if(strcmp(schedalg, "SSF") == 0)
            tmp = sffget(buf, tot);
        logfd = fopen("log", "a+");
        fprintf(logfd, "pid: %lu\n", pthread_self());
        fclose(logfd);
        Sem_post(&mutex);
        request_handle(tmp);
        close_or_die(tmp);
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

    while ((c = getopt(argc, argv, "d:p:t:b:s")) != -1)
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
    int buf[buffers];
    //master thread
    int listen_fd = open_listen_fd_or_die(port);
    //create a pool of worker threads
    pthread_t workers[threads];

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
        put(conn_fd, buf, buffers);
        Sem_post(&mutex);
        Sem_post(&fill);
    }
    free(schedalg);
    sem_close(&empty);
    sem_close(&fill);
    sem_close(&mutex);
    return 0;
}
