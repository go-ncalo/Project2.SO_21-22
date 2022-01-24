#include "tecnicofs_client_api.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


static char const *server_pipe;
static int session_id;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    printf("entrou no tfs_mount\n");
    char *pipename = client_pipe_path;
    unlink(pipename);
    if (mkfifo (pipename, 0777) < 0)
		exit (1);
    printf("criou pipe do client\n");
    int fwr = open(server_pipe_path,O_WRONLY);
    if (fwr==-1) {
        printf("erro na abertura da pipe do server \n");
    }
    printf("abriu pipe do server\n");
    char *buffer = '1';
    if (write(fwr, buffer,sizeof(char))==-1) {
        printf("erro na escrita no server %d\n", errno);
    }
    printf("escreveu na pipe do server\n");
    close(fwr);
    fwr = open(server_pipe_path,O_WRONLY);
    printf("abriu pipe do server\n");
    write(fwr,client_pipe_path,strlen(client_pipe_path));
    close(fwr);
    printf("escreveu na pipe do server\n");
    int frd = open(pipename, O_RDONLY);
    read(frd,session_id,strlen(session_id));
    close(frd);
    printf("leu da pipe do client\n");

    if (session_id!=-1) {
        return 0;
    }

    return -1;
}

int tfs_unmount() {

    
    return -1;
}

int tfs_open(char const *name, int flags) {
    /* TODO: Implement this */
    return -1;
}

int tfs_close(int fhandle) {
    /* TODO: Implement this */
    return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

int tfs_shutdown_after_all_closed() {
    /* TODO: Implement this */
    return -1;
}
