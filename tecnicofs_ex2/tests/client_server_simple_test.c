#include "client/tecnicofs_client_api.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*  This test is similar to test1.c from the 1st exercise.
    The main difference is that this one explores the
    client-server architecture of the 2nd exercise. */

int main(int argc, char **argv) {

    char *str = "AAA!";
    char *path = "/f1";
    char buffer[40];

    int f;
    ssize_t r;

    if (argc < 3) {
        printf("You must provide the following arguments: 'client_pipe_path "
               "server_pipe_path'\n");
        return 1;
    }

    assert(tfs_mount(argv[1], argv[2]) == 0);
    printf("mount\n");

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    printf("open\n");

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));
    printf("write\n");

    assert(tfs_close(f) != -1);
    printf("close\n");

    f = tfs_open(path, 0);
    assert(f != -1);
    printf("open2\n");

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str));
    printf("read\n");

    buffer[r] = '\0';
    assert(strcmp(buffer, str) == 0);
    printf("read what was supposed\n");

    assert(tfs_close(f) != -1);
    printf("close2\n");

    assert(tfs_unmount() == 0);
    printf("unmount\n");

    printf("Successful test.\n");

    return 0;
}
