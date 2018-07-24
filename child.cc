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

#define WRITES(a) { const char *foo = a; write(1, foo, strlen(foo)); }
#define WRITEI(a) { char buf[10]; assert(eye2eh(a, buf, 10, 10) != -1); WRITES(buf); }

/*
** Async-safe integer to a string. i is assumed to be positive. The number
** of characters converted is returned; -1 will be returned if bufsize is
** less than one or if the string isn't long enough to hold the entire
** number. Numbers are right justified. The base must be between 2 and 16;
** otherwise the string is filled with spaces and -1 is returned.
*/
int eye2eh(int i, char *buf, int bufsize, int base)
{
    if(bufsize < 1) return(-1);
    buf[bufsize-1] = '\0';
    if(bufsize == 1) return(0);
    if(base < 2 || base > 16)
    {
        for(int j = bufsize-2; j >= 0; j--)
        {
            buf[j] = ' ';
        }
        return(-1);
    }

    int count = 0;
    const char *digits = "0123456789ABCDEF";
    for(int j = bufsize-2; j >= 0; j--)
    {
        if(i == 0)
        {
            buf[j] = ' ';
        }
        else
        {
            buf[j] = digits[i%base];
            i = i/base;
            count++;
        }
    }
    if(i != 0) return(-1);
    return(count);
}

int main(int argc, char* argv[])
{
    WRITES("---- entering child\n");
    pid_t ppid = getppid(); // always successful
    char buffer1[20000];
    int len1;
    assert( kill(ppid, SIGTRAP) != -1 );
    assertsyscall(write(3, "3", strlen("3")), != -1);

    assertsyscall((len1 = read(4, buffer1, sizeof(buffer1))), != -1);
    buffer1[len1] = 0;

    WRITES(buffer1);
    WRITES("\n");
    WRITES("---- leaving child\n");

//    char buffer2[20000];
//    int len2;
//    assert( kill(ppid, SIGTRAP) != -1 );
//    assertsyscall(write(3, "2", strlen("2")), != -1);
//
//    assertsyscall((len2 = read(4, buffer2, sizeof(buffer2))), != -1);
//    buffer2[len2] = 0;
//
//    WRITES("child recieved:\n---------------\n");
//    WRITES(buffer2);
//    WRITES("\n");
//
//    char buffer3[20000];
//    int len3;
//    assert( kill(ppid, SIGTRAP) != -1 );
//    assertsyscall(write(3, "3", strlen("3")), != -1);
//
//    assertsyscall((len3 = read(4, buffer3, sizeof(buffer3))), != -1);
//    buffer3[len3] = 0;
//
//    WRITES("child recieved:\n---------------\n");
//    WRITES(buffer3);
//    WRITES("\n");
//
//    char buffer4[20000];
//    int len4;
//    assert( kill(ppid, SIGTRAP) != -1 );
//    assertsyscall(write(3, "3", strlen("3")), != -1);
//
//    assertsyscall((len4 = read(4, buffer4, sizeof(buffer4))), != -1);
//    buffer4[len4] = 0;
//
//    WRITES("child recieved:\n---------------\n");
//    WRITES(buffer4);
//    WRITES("\n");

    return 0;
}
