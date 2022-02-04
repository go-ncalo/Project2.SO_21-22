#include "tecnicofs_client_api.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define PIPENAME_SIZE 40
#define FILE_NAME_SIZE 40

//static char *server_pipe;
static char *client_pipe;
int session_id;
int fwr;
int frd;

int read_int_client_pipe();

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {

    client_pipe=malloc(strlen(client_pipe_path)+1);
    memcpy(client_pipe,client_pipe_path,strlen(client_pipe_path)+1);

    char buffer[PIPENAME_SIZE + 1];
    memset(buffer, '\0', sizeof(buffer));
    buffer[0] = TFS_OP_CODE_MOUNT;
    size_t bytes = strlen(client_pipe);
    strncpy(&buffer[1], client_pipe_path, bytes);

    unlink(client_pipe);
    if (mkfifo (client_pipe, 0777) < 0)
		exit (1);

    fwr = open(server_pipe_path,O_WRONLY);
    
    if (write_on_server_pipe(buffer,PIPENAME_SIZE+sizeof(char)) == -1) {
        return -1;
    }

    frd = open(client_pipe,O_RDONLY);
    if (frd == -1) {
        return -1;
    }

    session_id = read_int_client_pipe();
    if (session_id!=-1) {
        return 0;
    }

    return -1;
}

int tfs_unmount() {
    char buffer[1 + sizeof(int)];
    memset(buffer, '\0', sizeof(buffer));
    buffer[0] = TFS_OP_CODE_UNMOUNT;
    memcpy(buffer + 1, &session_id, sizeof(int));

    if (write_on_server_pipe(buffer, sizeof(buffer)) == -1) {
        return -1;
    }

    int return_value = read_int_client_pipe();
    if (return_value != -1) {
        if (unlink(client_pipe) == -1) {
            return -1;
        }
        close(fwr);
        if (close(frd)==-1) {
            return -1;
        }
        free(client_pipe);
        return return_value;
    }
    return -1;
}

int tfs_open(char const *name, int flags) {

    char buffer[FILE_NAME_SIZE + 1 + (2 * sizeof(int))];
    memset(buffer, '\0', sizeof(buffer));
    buffer[0] = TFS_OP_CODE_OPEN;
    memcpy(buffer + 1, &session_id, sizeof(int));
    strncpy(&buffer[1 + sizeof(int)], name, FILE_NAME_SIZE);
    memcpy(buffer + 1 + sizeof(int) + FILE_NAME_SIZE, &flags, sizeof(int));
    if (write_on_server_pipe(buffer, sizeof(buffer)) == -1) {
        return -1;
    }

    int return_value = read_int_client_pipe();
    if (return_value != -1) {
        return return_value;
    }
    
    return -1;
}

int tfs_close(int fhandle) {
    char buffer[1 + (2 * sizeof(int))];
    memset(buffer, '\0', sizeof(buffer));
    buffer[0] = TFS_OP_CODE_CLOSE;
    memcpy(buffer + 1, &session_id, sizeof(int));
    memcpy(buffer + 1 + sizeof(int), &fhandle, sizeof(int));
    if (write_on_server_pipe(buffer, sizeof(buffer)) == -1) {
        return -1;
    }

    int return_value = read_int_client_pipe();
    if (return_value != -1) {
        return return_value;
    }

    return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    char buffer_arguments[1 + (2 * sizeof(int)) + sizeof(size_t) + len];
    memset(buffer_arguments, '\0', sizeof(buffer_arguments));
    buffer_arguments[0] = TFS_OP_CODE_WRITE;
    memcpy(buffer_arguments + 1, &session_id, sizeof(int));
    memcpy(buffer_arguments + 1 + sizeof(int), &fhandle, sizeof(int));
    memcpy(buffer_arguments + 1 + (2 * sizeof(int)), &len, sizeof(size_t));
    strncpy(&buffer_arguments[1 + (2 * sizeof(int)) + sizeof(size_t)], buffer, len);

    if (write_on_server_pipe(buffer_arguments, sizeof(buffer_arguments)) == -1) {
        return -1;
    }

    int return_value = read_int_client_pipe();
    if (return_value != -1) {
        return return_value;
    }

    return -1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    char buffer_arguments[1 + (2 * sizeof(int)) + sizeof(size_t)];
    memset(buffer_arguments, '\0', sizeof(buffer_arguments));
    buffer_arguments[0] = TFS_OP_CODE_READ;
    memcpy(buffer_arguments + 1, &session_id, sizeof(int));
    memcpy(buffer_arguments + 1 + sizeof(int), &fhandle, sizeof(int));
    memcpy(buffer_arguments + 1 + (2 * sizeof(int)), &len, sizeof(size_t));

    if (write_on_server_pipe(buffer_arguments, sizeof(buffer_arguments)) == -1) {
        return -1;
    }

    char buffer_server[len + sizeof(int)];
    if (frd == -1) {
        return -1;
    }
    if (read(frd, buffer_server, len + sizeof(int))==-1) {
        return -1;
    }

    int return_value;
    memcpy(&return_value, buffer_server, sizeof(int));
    if (return_value != -1) {
        strncpy(buffer, &buffer_server[sizeof(int)], len);
        return return_value;
    }

    return -1;
}

int tfs_shutdown_after_all_closed() {
    char buffer[1 + sizeof(int)];
    memset(buffer, '\0', sizeof(buffer));
    buffer[0] = TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED;
    memcpy(buffer + 1, &session_id, sizeof(int));

    if (write_on_server_pipe(buffer, sizeof(buffer)) == -1) {
        return -1;
    }

    int return_value = read_int_client_pipe();
    if (return_value != -1) {
        return return_value;
    }

    return -1;
}


int write_on_server_pipe(const void* message,size_t bytes) {
    if (write(fwr, message,bytes)==-1) {
        return -1;
    }

    return 0;
}

int read_int_client_pipe() {
    int value;
    if (read(frd,&value,sizeof(int))==-1) {
        return -1;
    }

    return value;
}