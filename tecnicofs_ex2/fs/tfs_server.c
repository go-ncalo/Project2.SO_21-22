#include "operations.h"
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <errno.h>


#define S 20
#define PIPENAME_SIZE 40

static int sessions=0;
static int freeSessions[S];
static char *client_pipes[S][PIPENAME_SIZE];

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];

    int fserver, fclient;
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    unlink(pipename);
    if (mkfifo (pipename, 0777) < 0)
		exit (1);


    for (int i=0; i<S; i++) {
        freeSessions[i]=FREE;
    }

    
    tfs_init();

    char r;

    /* TO DO */

    

    /*onde escrevemos para fifo do servidor o OPCODE  eos argumentos*/

    while (1) {
        fserver=open(pipename,O_RDONLY);
        read(fserver,r,sizeof(char)+1);
        close(fserver);
        switch (r)
        {
            case TFS_OP_CODE_MOUNT: {
                int session_id=addSession();
                fserver=open(pipename,O_RDONLY);
                read(pipename,client_pipes[session_id],PIPENAME_SIZE+1);
                close(fserver);
                if (session_id!=-1) {
                    unlink(client_pipes[session_id]);
                    if (mkfifo (client_pipes[session_id], 0777) < 0)
		                exit (1);
                }
                fclient=open(client_pipes[session_id],O_WRONLY);
                write(fclient,session_id,sizeof(int)+1);
                close(fclient);
                break;
            }
        }

    return 0;
    }
}

int addSession() {
    for (int i=0; i<S; i++) {
        if (freeSessions[i]==FREE) {
            freeSessions[i]=TAKEN;
            sessions++;
            return i;
        }
    }
    return -1;
}

int deleteSession(int id) {

    if (freeSessions[id]==FREE) {
        return -1;
    }

    freeSessions[id]==FREE;
    sessions--;
    client_pipes[id]=NULL;

}


