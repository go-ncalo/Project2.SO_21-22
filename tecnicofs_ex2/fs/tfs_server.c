#include "operations.h"
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <errno.h>
#include <string.h>

int tfs_mount(int fserver);
int addSession();
int deleteSession(int id);

#define S 20
#define PIPENAME_SIZE 40

static int sessions=0;
static int freeSessions[S];
static char client_pipes[S][PIPENAME_SIZE];

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
    }

    
    tfs_init();

    char r[1];

    /* TO DO */

    /*onde escrevemos para fifo do servidor o OPCODE  eos argumentos*/

    fserver = open(pipename,O_RDONLY);
    if (fserver == -1) {
        printf("erro HERE\n");
    }

    while (1) {

        if (read(fserver,r,sizeof(char))==0) {
            close(fserver);
            fserver=open(pipename,O_RDONLY);
            continue;
        }

        switch (r[0]) {
            case TFS_OP_CODE_MOUNT:
                tfs_mount(fserver);
                break;

            default:
                break;
        }
    }
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
    strcpy(client_pipes[id], "");

    return 0;
}

int tfs_mount(int fserver) {
    int session_id = addSession();

    if (fserver == -1) {
        printf("error\n");
    }

    if (read(fserver, client_pipes[session_id], PIPENAME_SIZE) == -1) {
        printf("erro\n");
    }

    int fclient = open(client_pipes[session_id], O_WRONLY);
    
    if (write(fclient, &session_id, sizeof(int)) == -1) {
        return -1;
    }

    close(fclient);
    return 0;
}