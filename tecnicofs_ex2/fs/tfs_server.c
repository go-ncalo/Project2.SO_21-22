#include "operations.h"
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <errno.h>
#include <string.h>
#include <pthread.h>

int addSession();
int deleteSession(int id);
int mount_operation(int fserver);
int unmount_operation(int fserver);
int open_operation(int fserver);
int close_operation(int fserver);
int write_operation(int fserver);
int read_operation(int fserver);
int shutdown_after_all_closed_operation(int fserver);

#define S 20
#define PIPENAME_SIZE 40
#define FILE_NAME_SIZE 40


typedef struct {
    char opcode[1];
    int session_id;
    char filename[40];
    int flags;
    int fhandle;
    size_t len;
    char* buffer;
} args_struct;


static int sessions=0;
static int freeSessions[S];
static int file_descriptors[S];
static char client_pipes[S][PIPENAME_SIZE];
static pthread_t threads[S];
static pthread_cond_t pthread_cond[S];
static pthread_mutex_t mutexs[S];
static args_struct args[S];
static int active_tasks[S];

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char const *pipename = argv[1];

    int fserver;
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    unlink(pipename);
    if (mkfifo (pipename, 0640) < 0)
		exit (1);


    for (int i=0; i<S; i++) {
        freeSessions[i]=FREE;
        file_descriptors[i] = -1;

        active_tasks[i]=0;
        args[i].session_id=i;

        if (pthread_create(&threads[i],NULL, &thread_handle, &args[i]) == -1)
            return -1;

        if (pthread_cond_init(&pthread_cond[i], 0) != 0)
            return -1;

        if (pthread_mutex_init(&mutexs[i], 0) != 0)
            return -1;
    }

    
    tfs_init();

    int i = 1;
    char r[1];

    fserver = open(pipename,O_RDONLY);
    if (fserver == -1) {
        return -1;
    }

    while (i == 1) {
        if (read(fserver, r, sizeof(char)) == 0) {
            close(fserver);
            fserver = open(pipename,O_RDONLY);
            continue;
        }
        int session_id;
        switch (r[0]) {
            case TFS_OP_CODE_MOUNT:
                mount_operation(fserver);
                break;
            
            case TFS_OP_CODE_UNMOUNT:
                unmount_operation(fserver);
                break;

            case TFS_OP_CODE_OPEN:
                open_operation(fserver);
                break;

            case TFS_OP_CODE_CLOSE:
                close_operation(fserver);  
                break;

            case TFS_OP_CODE_WRITE:
                write_operation(fserver);
                break;

            case TFS_OP_CODE_READ:
                read_operation(fserver);
                break;
            
            case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED:
                shutdown_after_all_closed_operation(fserver);
                i = 0;
                break;

            default:
                break;
        }
        if (r[0]!=TFS_OP_CODE_MOUNT) {
            pthread_mutex_lock(&mutexs[session_id]);
            active_tasks[session_id]=1;
            pthread_cond_signal(&pthread_cond[session_id]);
            pthread_mutex_unlock(&mutexs[session_id]);
        }
    }

    for (int j=0; j<S; j++) {
        freeSessions[j] = FREE;
        if (file_descriptors[j] != -1) {
            close(file_descriptors[j]);
            file_descriptors[j] = -1;
        }
    }

    close(fserver);
    unlink(pipename);
    return 0;
}

int addSession() {
    for (int i=0; i < S; i++) {
        if (freeSessions[i] == FREE) {
            freeSessions[i] = TAKEN;
            sessions++;
            return i;
        }
    }
    return -1;
}

int deleteSession(int id) {

    if (freeSessions[id] == FREE) {
        return -1;
    }

    freeSessions[id] = FREE;
    sessions--;
    

    return 0;
}

int mount_operation() {
    int session_id = addSession();

    if (fserver == -1) {
        return -1;
    }

    if (read(fserver, client_pipes[session_id], PIPENAME_SIZE) == -1) {
        return -1;
    }

    int fclient = open(client_pipes[session_id], O_WRONLY);
    file_descriptors[session_id] = fclient;
    if (write(fclient, &session_id, sizeof(int)) == -1) {
        return -1;
    }
    return 0;
}

int open_operation() {
    char file[FILE_NAME_SIZE];
    int flag;
    int session_id;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }
    if (read(fserver, file, FILE_NAME_SIZE) == -1) {
        return -1;
    }
    if (read(fserver, &flag, sizeof(int)) == -1) {
        return -1;
    }
    
    int return_value = tfs_open(file, flag);

    int fclient = file_descriptors[session_id];

    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }

    return 0;
}

int unmount_operation() {
    int session_id;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    int return_value = deleteSession(session_id);

    int fclient = file_descriptors[session_id];
    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }
    if (close(fclient) == -1) {
        return -1;
    }
    file_descriptors[session_id] = -1;
    strcpy(client_pipes[session_id], "");

    return 0;
}

int close_operation() {
    int session_id, fhandle;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &fhandle, sizeof(int)) == -1) {
        return -1;
    }

    int return_value = tfs_close(fhandle);

    int fclient = file_descriptors[session_id];
    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }
    
    return 0;
}

int write_operation() {
    int session_id, fhandle;
    size_t len;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &fhandle, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &len, sizeof(size_t)) == -1) {
        return -1;
    }

    char buffer[len];

    if (read(fserver, buffer, len) == -1) {
        return -1;
    }

    int return_value = (int) tfs_write(fhandle, buffer, len);

    int fclient = file_descriptors[session_id];
    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }

    return 0;
}

int read_operation() {
    int session_id, fhandle;
    size_t len;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &fhandle, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &len, sizeof(size_t)) == -1) {
        return -1;
    }

    char buffer[len];
    char return_buffer[len + sizeof(int)];

    int return_value = (int) tfs_read(fhandle, buffer, len);

    memcpy(return_buffer, &return_value, sizeof(int));
    strncpy(&return_buffer[sizeof(int)], buffer, len);

    int fclient = file_descriptors[session_id];
    if (write(fclient, return_buffer, len + sizeof(int)) == -1) {
        return -1;
    }
    return 0;
}

int shutdown_after_all_closed_operation() {
    int session_id, fhandle;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &fhandle, sizeof(int)) == -1) {
        return -1;
    }

    int return_value = tfs_destroy_after_all_closed();

    if (return_value == -1) {
        return -1;
    }

    int fclient = file_descriptors[session_id];
    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }

    return 0;
}


int thread_handle(void* args) {

    while (1) {
        pthread_mutex_lock(&mutexs[((args_struct*)args)->session_id]);
        while (active_tasks[((args_struct*)args)->session_id] != 0) {
            pthread_cond_wait(&pthread_cond[((args_struct*)args)->session_id], &mutexs[((args_struct*)args)->session_id]);
        }
        pthread_mutex_unlock(&mutexs[((args_struct*)args)->session_id]);

    }
    switch (((args_struct*)args)->opcode) {
            case TFS_OP_CODE_UNMOUNT:
                unmount_operation();
                break;

            case TFS_OP_CODE_OPEN:
                open_operation();
                break;

            case TFS_OP_CODE_CLOSE:
                close_operation();  
                break;

            case TFS_OP_CODE_WRITE:
                write_operation();
                break;

            case TFS_OP_CODE_READ:
                read_operation();
                break;
            
            case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED:
                shutdown_after_all_closed_operation();
                i = 0;
                break;

            default:
                break;
        }

}

int read_unmount_operation(int fserver) {
    int session_id;
    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }
    return session_id;

}