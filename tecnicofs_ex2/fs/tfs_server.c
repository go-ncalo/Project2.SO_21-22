#include "operations.h"

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    char r[1];
    char *buffer;

    /* TO DO */

    /*onde escrevemos para fifo do servidor o OPCODE  eos argumentos*/
    mkfifo(pipename);

    while (1) {
        read()
    }

    return 0;
}