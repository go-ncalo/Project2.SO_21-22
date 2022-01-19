#include "tecnicofs_client_api.h"

// função para receber replies
// variável com o session_id
// maybe uma struct ?? com o session_id e as pipes

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    char *pipename = client_pipe_path;
    mkfifo(pipename);
    // abrir os pipes
    int fwr = open(server_pipe_path,O_WRONLY);
    int frd = open(pipename, O_RDONLY):
    // por as cenas num buffer, pq é só um write
    write(fwr,TFS_OP_CODE_MOUNT,strlen(TFS_OP_CODE_MOUNT)+1);
    write(fwr,client_pipe_path,strlen(client_pipe_path)+1);
    //ler 

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
