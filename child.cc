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

    char buffer1[200000];
    int len1;
    assertsyscall(write(3, "1", strlen("1")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len1 = read(4, buffer1, sizeof(buffer1))), != -1);
    buffer1[len1] = 0;

    char buffer2[200000];
    int len2;
    assertsyscall(write(3, "2", strlen("2")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len2 = read(4, buffer2, sizeof(buffer2))), != -1);
    buffer2[len2] = 0;

    char buffer3[200000];
    int len3;
    assertsyscall(write(3, "3", strlen("3")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len3 = read(4, buffer3, sizeof(buffer3))), != -1);
    buffer3[len3] = 0;

    char buffer4[20000];
    int len4;
    assertsyscall(write(3, "4HELLO PARENT", strlen("4HELLO PARENT")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len4 = read(4, buffer4, sizeof(buffer4))), != -1);
    buffer4[len4] = 0;

    return 0;
}
