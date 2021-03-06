#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <list>
#include <iterator>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
using namespace std;

#define NUM_SECONDS 20
#define EVER ;;
#define READ 0
#define WRITE 1

#define assertsyscall(x, y) if(!((x) y)){int err = errno; \
fprintf(stderr, "In file %s at line %d: ", __FILE__, __LINE__); \
perror(#x); exit(err);}

#ifdef EBUG
# define dmess(a) cout << "in " << __FILE__ << \
" at " << __LINE__ << " " << a << endl;

# define dprint(a) cout << "in " << __FILE__ << \
" at " << __LINE__ << " " << (#a) << " = " << a << endl;

# define dprintt(a,b) cout << "in " << __FILE__ << \
" at " << __LINE__ << " " << a << " " << (#b) << " = " \
<< b << endl
#else
# define dmess(a)
# define dprint(a)
# define dprintt(a,b)
#endif

// http://man7.org/linux/man-pages/man7/signal-safety.7.html

#define WRITES(a) { const char *foo = a; write(1, foo, strlen(foo)); }
#define WRITEI(a) { char buf[10]; assert(eye2eh(a, buf, 10, 10) != -1); WRITES(buf); }

enum STATE { NEW, RUNNING, WAITING, READY, TERMINATED };

struct PCB
{
    STATE state;
    const char *name;       // name of the executable
    int pid;                // process id from fork();
    int ppid;               // parent process id
    int interrupts;         // number of times interrupted
    int switches;           // may be < interrupts
    int started;            // the time this process started
    int child2parent[2];    // pipe from child to parent
    int parent2child[2];    // pipe from parent to child
};

PCB *running;
PCB *idle;
PCB *temp;

// http://www.cplusplus.com/reference/list/list/
list<PCB *> new_list;
list<PCB *> processes;

int sys_time;

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

/*
** a signal handler for those signals delivered to this process, but
** not already handled.
*/
void grab(int signum) { WRITEI(signum); WRITES("\n"); }

// c++decl> declare ISV as array 32 of pointer to function(int) returning void
void(*ISV[32])(int) = {
/* 00 01 02 03 04 05 06 07 08 09 */
/* 0 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 10 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 20 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 30 */ grab, grab
};

/*
** stop the running process and index into the ISV to call the ISR
*/
void ISR(int signum)
{
    if(signum != SIGCHLD) //if it is SIGALRM
    {
        if(kill(running->pid, SIGSTOP) == -1)
        {
            WRITES("In ISR kill returned: ");
            WRITEI(errno);
            WRITES("\n");
            return;
        }

        WRITES("In ISR stopped: ");
        WRITEI(running->pid);
        WRITES("\n");
        running->state = READY;
    }

    ISV[signum](signum);
}

/*
** an overloaded output operator that prints a PCB
*/
ostream& operator <<(ostream &os, struct PCB *pcb)
{
    os << "state: " << pcb->state << endl;
    os << "name: " << pcb->name << endl;
    os << "pid: " << pcb->pid << endl;
    os << "ppid: " << pcb->ppid << endl;
    os << "interrupts: " << pcb->interrupts << endl;
    os << "switches: " << pcb->switches << endl;
    os << "started: " << pcb->started << endl;
    return(os);
}

/*
** an overloaded output operator that prints a list of PCBs
*/
ostream& operator <<(ostream &os, list<PCB *> which)
{
    list<PCB *>::iterator PCB_iter;
    for(PCB_iter = which.begin(); PCB_iter != which.end(); PCB_iter++)
    {
        os <<(*PCB_iter);
    }
    return(os);
}

/*
** send signal to process pid every interval for number of times.
*/
void send_signals(int signal, int pid, int interval, int number)
{
    dprintt("at beginning of send_signals", getpid());

    for(int i = 1; i <= number; i++)
    {
        assertsyscall(sleep(interval), == 0);
        dprintt("sending", signal);
        dprintt("to", pid);
        assertsyscall(kill(pid, signal), == 0)
    }

    dmess("at end of send_signals");
}

struct sigaction *create_handler(int signum, void(*handler)(int))
{
    struct sigaction *action = new(struct sigaction);

    action->sa_handler = handler;

/*
** SA_NOCLDSTOP
** If signum is SIGCHLD, do not receive notification when
** child processes stop(i.e., when child processes receive
** one of SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU).
*/
    if(signum == SIGCHLD)
    {
        action->sa_flags = SA_NOCLDSTOP | SA_RESTART;
    }
    else
    {
        action->sa_flags = SA_RESTART;
    }

    sigemptyset(&(action->sa_mask));
    assert(sigaction(signum, action, NULL) == 0);
    return(action);
}

void scheduler(int signum)
{
    WRITES("---- entering scheduler\n");
    assert(signum == SIGALRM);
    sys_time++;
    temp = running;
    int found_one = 0;

    if(running->state != TERMINATED)
    {
        running->interrupts++;
    }

    for(int i = 0; i < (int) processes.size(); i++)
    {
        PCB *front = processes.front();
        processes.pop_front();
        processes.push_back(front);

        if(front->state == NEW)
        {
            WRITES("starting: ");
            WRITES(front->name);
            front->state = RUNNING;
            front->ppid = getpid();
            front->switches = 0;
            front->interrupts = 0;
            front->started = sys_time;
            running = front;

            assertsyscall((front->pid = fork()), != -1);
            if(front->pid == 0)
            {
                // close the ends we should't use
                assertsyscall(close(front->child2parent[READ]), == 0);
                assertsyscall(close(front->parent2child[WRITE]), == 0);
                assertsyscall(dup2( (front->child2parent[WRITE]), 3), != -1);
                assertsyscall(dup2( (front->parent2child[READ]), 4), != -1);
                assertsyscall(execl(front->name, front->name, NULL), != -1);
            }
            // close the ends we should't use
            assertsyscall(close(front->child2parent[WRITE]), == 0);
            assertsyscall(close(front->parent2child[READ]), == 0);
            found_one = 1;
            break;
        }
        else if(front->state == READY)
        {
            WRITES("continuing ");
            WRITEI(front->pid);
            front->state = RUNNING;
            running = front;
            if(temp != running)
            {
                WRITES("\na switch has occurred");
                running->switches++;
            }

            if(kill(front->pid, SIGCONT) == -1)
            {
                WRITES("in scheduler kill error: ");
                WRITEI(errno);
                kill(0, SIGTERM);
            }
            found_one = 1;
            break;
        }
    }

    if(!found_one)
    {
        WRITES("continuing idle");
        idle->state = RUNNING;
        running = idle;
        if(kill(idle->pid, SIGCONT) == -1)
        {
            kill(0, SIGTERM);
        }
    }

    WRITES("\n---- leaving scheduler\n");
}

void process_done(int signum)
{
    WRITES("---- entering process_done\n");
    assert(signum == SIGCHLD);

    // might have multiple children done.
    for(int i = 0; i < (int) processes.size(); i++)
    {
        int status, cpid;

        // we know we received a SIGCHLD so don't wait.
        assertsyscall((cpid = waitpid(-1, &status, WNOHANG)), >= 0);

        if(cpid < 0)
        {
            WRITES("cpid < 0\n");
            assertsyscall(kill(0, SIGTERM), != 0);
        }
        else if(cpid == 0)
        {
            // no more children.
            break;
        }
        else
        {
            WRITES("process exited: \n");
            list<PCB *>::iterator PCB_iter;
            for(PCB_iter = processes.begin(); PCB_iter != processes.end(); PCB_iter++)
            {
                PCB* process = *PCB_iter;
                if(process->pid == cpid)
                {
                    process->state = TERMINATED;
                    cout << process;
                    cout << "process took " << sys_time - process->started << " second(s) to execute\n";
                }
            }
        }
    }

    /* restart idle process */
    WRITES("continuing idle\n");
    idle->state = RUNNING;
    running = idle;
    if(kill(running->pid, SIGCONT) == -1)
    {
        WRITES("in process_done kill error: ");
        WRITEI(errno);
        WRITES("\n");
    }

    WRITES("---- leaving process_done\n");
}

/*
* SIGTRAP handler
*/
void trap_handler(int signum)
{
    WRITES("---- entering trap_handler\n");
    assert(signum == SIGTRAP);

    int ret;
    char buffer[10240];
    list<PCB *>::iterator i;
    for(i = processes.begin(); i != processes.end(); i++)
    {
        PCB *process = *i;

        struct pollfd fds[1];
        fds[0].fd = process->child2parent[READ];
        fds[0].events = POLLIN | POLLOUT;
        ret = poll(fds, (nfds_t) 1, 0);
        if (ret > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                int n = read(process->child2parent[READ], buffer, sizeof(buffer));
                buffer[n] = '\0';
                char kernel_call = buffer[0];

                WRITES("received kernel call: ");
                if(kernel_call == '1')
                {
                    WRITES("1\n");
                    char buf[10];
                    char temp[8];
                    strncat(buf, "system_time: ", strlen("system_time") + 1);
                    assertsyscall( (eye2eh(sys_time, temp, sizeof(temp), 10)), != -1);
                    strncat(buf, temp, strlen("system_time") + 1);
                    assertsyscall((write(process->parent2child[WRITE], buf, strlen(buf) * sizeof(char))), != -1);
                    buf[0] = 0;
                    temp[0] = 0;
                }
                else if(kernel_call == '2')
                {
                    WRITES("2\n");
                    string calling_process = "calling process: \n";
                    string ret_str = calling_process + process->name;
                    char* buf = (char*)ret_str.c_str();
                    assertsyscall((write(process->parent2child[WRITE], buf, strlen(buf) * sizeof(char))), != -1);
                }
                else if(kernel_call == '3')
                {
                    WRITES("3\n");
                    string ret_str = "process_list: \n";
                    int counter = 0;

                    list<PCB *>::iterator k;
                    for(k = processes.begin(); k != processes.end(); k++)
                    {
                        if(counter % 5 == 0 && counter != 0)
                        {
                            ret_str += "\n";
                        }
                        PCB *process = *k;
                        ret_str = ret_str + process->name + "\t\t";
                        counter++;
                    }
                    char* buf = (char*)ret_str.c_str();
                    assertsyscall((write(process->parent2child[WRITE], buf, strlen(buf) * sizeof(char))), != -1);
                }
                else if(kernel_call == '4')
                {
                    WRITES("4\n");
                    char *temp = buffer;
                    temp++;
                    assertsyscall((write(process->parent2child[WRITE], temp, strlen(temp) * sizeof(char))), != -1);
                }
            }
        }
    }

    if( (processes.size() == 1) && (kill(running->pid, SIGCONT) == -1) )
    {
        WRITES("in trap_handler kill error: ");
        WRITEI(errno);
        WRITES("\n");
    }

    WRITES("---- leaving trap_handler\n");
}

/*
** set up the "hardware"
*/
void boot()
{
    sys_time = 0;

    ISV[SIGALRM] = scheduler;
    ISV[SIGCHLD] = process_done;
    ISV[SIGTRAP] = trap_handler;

    struct sigaction *alarm = create_handler(SIGALRM, ISR);
    struct sigaction *child = create_handler(SIGCHLD, ISR);
    struct sigaction *trap = create_handler(SIGTRAP, ISR);

    // start up clock interrupt
    int ret;
    if((ret = fork()) == 0)
    {
        send_signals(SIGALRM, getppid(), 1, NUM_SECONDS);

        // once that's done, cleanup and really kill everything...
        delete(alarm);
        delete(child);
        delete(trap);
        delete(idle);
        delete(running);
        delete(temp);
        kill(0, SIGTERM);
    }

    /* Luke Randazzo helped me here but I completely understand that we need to cleanup the parent process as well
    ** after the fork(), otherwise the parent process' memory for the sigaction structs would be leaked.
     */
    delete(alarm);
    delete(child);
    delete(trap);
    delete(idle);
    delete(running);
    delete(temp);

    if(ret < 0)
    {
        perror("fork");
    }
}

void create_idle()
{
    idle = new(PCB);
    idle->state = READY;
    idle->name = "IDLE";
    idle->ppid = getpid();
    idle->interrupts = 0;
    idle->switches = 0;
    idle->started = sys_time;
    assertsyscall((idle->pid = fork()), != -1);

    if(idle->pid == 0)
    {
        pause();
        perror("pause in create_idle");
    }
}

int main(int argc, char **argv)
{
    for(int i = 1; i < argc; i++)
    {
        int fl;
        PCB* process_i = new(PCB);
        process_i->state = NEW;
        process_i->name = argv[i];
        process_i->pid = 0;
        process_i->ppid = 0;
        process_i->interrupts = 0;
        process_i->switches = 0;
        process_i->started = 0;

        assertsyscall(pipe(process_i->child2parent), == 0);
        assertsyscall(pipe(process_i->parent2child), == 0);
        assertsyscall((fl = fcntl(process_i->child2parent[READ], F_GETFL)), != -1);
        assertsyscall(fcntl(process_i->child2parent[READ], F_SETFL, fl | O_NONBLOCK), == 0);

        processes.push_back(process_i);
    }

    boot();
    create_idle();
    running = idle;
    cout << running;

    // we keep this process around so that the children don't die and
    // to keep the IRQs in place.
    for(EVER)
    {
        // "Upon termination of a signal handler started during a
        // pause(), the pause() call will return."
        pause();
    }
}
