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
int unmount_operation(int session_id);
int open_operation(int session_id, char const *name, int flags);
int close_operation(int session_id, int fhandle);
int write_operation(int session_id, int fhandle, void const *buffer, size_t len);
int read_operation(int session_id, int fhandle, size_t len);
int shutdown_after_all_closed_operation(int session_id);
int parse_unmount_operation(int fserver);
int parse_open_operation(int fserver);
int parse_close_operation(int fserver);
int parse_write_operation(int fserver);
int parse_read_operation(int fserver);
int parse_shutdown_after_all_closed_operation(int fserver);
void* thread_handle(void* arg);
int i = 1;

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

    for (int j=0; j<S; j++) {
        freeSessions[j]=FREE;
        file_descriptors[j] = -1;

        active_tasks[j]=0;
        args[j].session_id=j;

        if (pthread_create(&threads[j],NULL, &thread_handle, &args[j]) == -1)
            return -1;

        if (pthread_cond_init(&pthread_cond[j], 0) != 0)
            return -1;

        if (pthread_mutex_init(&mutexs[j], 0) != 0)
            return -1;
    }

    
    tfs_init();

    i = 1;
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
        int session_id=-1;
        switch (r[0]) {

            case TFS_OP_CODE_MOUNT:
                mount_operation(fserver);
                break;
            
            case TFS_OP_CODE_UNMOUNT:
                session_id=parse_unmount_operation(fserver);
                break;

            case TFS_OP_CODE_OPEN:
                session_id=parse_open_operation(fserver);
                break;

            case TFS_OP_CODE_CLOSE:
                session_id=parse_close_operation(fserver);  
                break;

            case TFS_OP_CODE_WRITE:
                session_id=parse_write_operation(fserver);
                break;

            case TFS_OP_CODE_READ:
                session_id=parse_read_operation(fserver);
                break;
            
            case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED:
                session_id=parse_shutdown_after_all_closed_operation(fserver);
                break;

            default:
                break;
        }
        if (r[0]!=TFS_OP_CODE_MOUNT && session_id!=-1) {
            pthread_mutex_lock(&mutexs[session_id]);
            args[session_id].opcode[0]=r[0];
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
    for (int j=0; j < S; j++) {
        if (freeSessions[j] == FREE) {
            freeSessions[j] = TAKEN;
            sessions++;
            return j;
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

int mount_operation(int fserver) {
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

int open_operation(int session_id, char const *name, int flags) {

    int return_value = tfs_open(name, flags);

    int fclient = file_descriptors[session_id];

    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }


    return 0;
}

int unmount_operation(int session_id) {
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

int close_operation(int session_id, int fhandle) {

    int return_value = tfs_close(fhandle);

    int fclient = file_descriptors[session_id];
    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }
    
    return 0;
}

int write_operation(int session_id, int fhandle, void const *buffer, size_t len) {
    
    int return_value = (int) tfs_write(fhandle, buffer, len);

    int fclient = file_descriptors[session_id];
    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }

    return 0;
}

int read_operation(int session_id, int fhandle, size_t len) {

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

int shutdown_after_all_closed_operation(int session_id) {

    int return_value = tfs_destroy_after_all_closed();

    if (return_value == -1) {
        return -1;
    }

    int fclient = file_descriptors[session_id];
    if (write(fclient, &return_value, sizeof(int)) == -1) {
        return -1;
    }

    i=0;

    return 0;
}


void* thread_handle(void* arg) {

    while (1) {
        pthread_mutex_lock(&mutexs[((args_struct*)arg)->session_id]);
        while (active_tasks[((args_struct*)arg)->session_id] !=1) {
            pthread_cond_wait(&pthread_cond[((args_struct*)arg)->session_id], &mutexs[((args_struct*)arg)->session_id]);
        }
        pthread_mutex_unlock(&mutexs[((args_struct*)arg)->session_id]);

        switch (((args_struct*)arg)->opcode[0]) {

            case TFS_OP_CODE_UNMOUNT:
                unmount_operation(((args_struct*)arg)->session_id);
                break;

            case TFS_OP_CODE_OPEN:
                open_operation(((args_struct*)arg)->session_id, ((args_struct*)arg)->filename,
                                ((args_struct*)arg)->flags);
                break;

            case TFS_OP_CODE_CLOSE:
                close_operation(((args_struct*)arg)->session_id,((args_struct*)arg)->fhandle);  
                break;

            case TFS_OP_CODE_WRITE:
                write_operation(((args_struct*)arg)->session_id,((args_struct*)arg)->fhandle,
                                ((args_struct*)arg)->buffer,((args_struct*)arg)->len);
                free(((args_struct*)args)->buffer);
                break;

            case TFS_OP_CODE_READ:
                read_operation(((args_struct*)arg)->session_id,((args_struct*)arg)->fhandle, 
                                ((args_struct*)arg)->len);
                break;
            
            case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED:
                shutdown_after_all_closed_operation(((args_struct*)arg)->session_id);
                break;

            default:
                break;
        }
    
    active_tasks[((args_struct*)arg)->session_id]=0;
    
    }

}

int parse_unmount_operation(int fserver) {
    int session_id;
    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }
    return session_id;
}

int parse_open_operation(int fserver) {

    int session_id;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }
    if (read(fserver, &args[session_id].filename, FILE_NAME_SIZE) == -1) {
        return -1;
    }
    if (read(fserver, &args[session_id].flags, sizeof(int)) == -1) {
        return -1;
    }


    return session_id;
}

int parse_close_operation(int fserver) {
    
    int session_id;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &args[session_id].fhandle, sizeof(int)) == -1) {
        return -1;
    }

    return session_id;
}

int parse_write_operation(int fserver) {
    
    int session_id;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &args[session_id].fhandle, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &args[session_id].len, sizeof(size_t)) == -1) {
        return -1;
    }
    
    
    args[session_id].buffer=malloc(args[session_id].len);

    if (read(fserver, args[session_id].buffer, args[session_id].len) == -1) {
        return -1;
    }

    return session_id;
}

int parse_read_operation(int fserver) {
    int session_id;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &args[session_id].fhandle, sizeof(int)) == -1) {
        return -1;
    }

    if (read(fserver, &args[session_id].len, sizeof(size_t)) == -1) {
        return -1;
    }
    return session_id;
}

int parse_shutdown_after_all_closed_operation(int fserver) {
    int session_id;

    if (read(fserver, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    return session_id;
}