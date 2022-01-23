#include "tecnicofs_client_api.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static char const *server_pipe;
static int session_id;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    char *pipename = client_pipe_path;
    mkfifo(pipename,0777);
    int fwr = open(server_pipe_path,O_WRONLY);
    write(fwr,TFS_OP_CODE_MOUNT,strlen(TFS_OP_CODE_MOUNT)+1);
    close(fwr);
    int fwr = open(server_pipe_path,O_WRONLY);
    write(fwr,client_pipe_path,strlen(client_pipe_path)+1);
    close(fwr);
    int frd = open(pipename, O_RDONLY);
    read(frd,session_id,strlen(session_id)+1);
    close(frd);

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
