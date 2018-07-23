#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#define assertsyscall(x, y) if(!((x) y)){int err = errno; \
    fprintf(stderr, "In file %s at line %d: ", __FILE__, __LINE__); \
        perror(#x); exit(err);}

#define READ 0
#define WRITE 1

int main(int argc, char* argv[])
{
    pid_t ppid = getppid(); // always successful
    char buffer[20000];
    int len;
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall(write(3, "3", strlen("3")), != -1);

    assertsyscall((len = read(4, buffer, sizeof(buffer))), != -1);
    buffer[len] = 0;

    printf("child recieved:\n---------------\n%s\n", buffer);

    return 0;
}
