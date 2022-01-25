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
static char const *client_pipe;
static int session_id;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {

    server_pipe=malloc(strlen(server_pipe_path)+1);
    memcpy(server_pipe,server_pipe_path,strlen(server_pipe_path)+1);
    client_pipe=malloc(strlen(client_pipe_path)+1);
    memcpy(client_pipe,client_pipe_path,strlen(client_pipe_path)+1);

    char *pipename = client_pipe_path;
    unlink(pipename);
    if (mkfifo (pipename, 0777) < 0)
		exit (1);
    
    if (write_on_server_pipe(TFS_OP_CODE_MOUNT,sizeof(char))==-1) {
        return -1;
    }
    printf("escreveu opcode no server\n");

    if (write_on_server_pipe(client_pipe_path,strlen(client_pipe_path)+1)==-1) {
        return -1;
    }
    printf("escreveu client path no server\n");


    //int session_id=read_int_client_pipe();

    int frd = open(client_pipe_path,O_RDONLY);
    if (frd==-1) {
        return -1;
    }
    if (read(frd,session_id,sizeof(int))==-1) {
        return -1;
    }
    if (close(frd)==-1) {
        return -1;
    }

    printf("Session id: %d\n", session_id);

    if (session_id!=-1) {
        return 0;
    }

    return -1;
}

int tfs_unmount() {
    free(client_pipe);
    free(server_pipe);
    return -1;
}

int tfs_open(char const *name, int flags) {

    /*int fwr = open(server_pipe,O_WRONLY);
    if (fwr==-1) {
        return -1;
    }
    char buffer = TFS_OP_CODE_OPEN;
    if (write(fwr, &buffer,sizeof(char))==-1) {
        return -1;
    }
    if (close(fwr)==-1) {
        return -1;
    }
    */
    
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


int write_on_server_pipe(const void* message,size_t bytes) {
    int fwr = open(server_pipe,O_WRONLY);
    if (fwr==-1) {
        return -1;
    }
    if (write(fwr, &message,bytes)==-1) {
        return -1;
    }
    if (close(fwr)==-1) {
        return -1;
    }
    return 0;
}

int read_int_client_pipe() {
    printf("entrou no read\n");
    int value;
    printf("here\n");
    int frd = open(client_pipe,O_RDONLY);
    printf("erro\n");
    if (frd==-1) {
        printf("erro1\n");
        return -1;
    }
    if (read(frd,value,sizeof(int))==-1) {
        printf("erro2\n");
        return -1;
    }
    if (close(frd)==-1) {
         printf("erro3\n");
        return -1;
    }
    printf("value: %d\n", value);
    return value;

}