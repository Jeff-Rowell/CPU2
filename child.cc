#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#define assertsyscall(x, y) if(!((x) y)){int err = errno; \
    fprintf(stderr, "In file %s at line %d: ", __FILE__, __LINE__); \
        perror(#x); exit(err);}

#define READ 0
#define WRITE 1

int main(int argc, char* argv[])
{
    pid_t ppid = getppid(); // always successful

    char buffer1[240000];
    int len1;
    assertsyscall(write(3, "1", sizeof("1")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len1 = read(4, buffer1, sizeof(buffer1))), != -1);
    buffer1[len1] = 0;
    printf("%s\n", buffer1);

    char buffer2[240000];
    int len2;
    assertsyscall(write(3, "2", sizeof("2")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len2 = read(4, buffer2, sizeof(buffer2))), != -1);
    buffer2[len2] = 0;
    printf("%s\n", buffer2);


    char buffer3[240000];
    int len3;
    assertsyscall(write(3, "3", sizeof("3")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len3 = read(4, buffer3, sizeof(buffer3))), != -1);
    buffer3[len3] = 0;
    printf("%s\n", buffer3);

    char buffer4[240000];
    int len4;
    assertsyscall(write(3, "4HELLO PARENT", sizeof("4HELLO PARENT")), != -1);
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall((len4 = read(4, buffer4, sizeof(buffer4))), != -1);
    buffer4[len4] = 0;
    printf("%s\n", buffer4);

    return 0;
}
