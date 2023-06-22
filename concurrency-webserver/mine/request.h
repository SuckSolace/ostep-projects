#ifndef __REQUEST_H__

typedef struct _fd_inf {
    int fd;
    off_t file_sz;
    int is_static;
    struct stat *sbuf;
    char * filename;
    char * cgiargs;
} fd_inf;

void request_handle(fd_inf *fif);
void request_error(int fd, char* cause, char *errnum, char *shortmsg, char *longmsg);
void request_read_headers(int fd);
int request_parse_uri(char*uri, char*filename, char*cgiargs);

#endif // __REQUEST_H__
