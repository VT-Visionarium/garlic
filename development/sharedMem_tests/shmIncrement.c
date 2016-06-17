/*
 * If you do not have the makefile that came with this file you can
 * compile this file like so:

       gcc -g -Wall -Werror shm_test.c -o shm_test

*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>

static void _vassert(const char *x, const char *file, const char *func,
        int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("%s:%s:%d[%d]:ASSERTION(%s) FAILED: ",
            file, func, line, getpid(), x);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\nWill sleep now\n\n");
    while(1) usleep(100000);
}

#define VASSERT(x, fmt, ...) \
    do { if(!(x))\
        _vassert(#x, __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__);\
    } while(0)

#define ASSERT(x) \
    do { if(!(x))\
        _vassert(#x, __FILE__, __func__, __LINE__, "\n");\
    } while(0)



//volatile static long *shm = NULL;
static long *shm = NULL; // using "volatile long *shm" does not help
                         // make the call *shm += INC atomic.

#define TEST_RUNS    2000
#define NUM_CHILD      2 // 1 should pass every time, >1 should fail
                         // try NUM_CHILD 1 for sanity check
#define INC            64
#define INC_PER_CHILD  10000
#define INIT_VAL       0
#define EXPECTED_TOTAL (INIT_VAL + (NUM_CHILD*INC*INC_PER_CHILD))




static int child(void)
{
    //usleep(1000000);
    
    int count = INC_PER_CHILD;

    while(count--)
        // This is a very bad thing to do.
        // This is not an atomic operation across processes,
        // and we prove that by running this code.
        *shm += INC;

    //printf("child exiting\n");
    return 0;
}

static void createShm(void)
{
    int fd;
 
    fd = open("/dev/zero", O_RDWR );
    VASSERT(fd > -1, "open(\"/dev/zero\",) failed\n");
    shm = mmap(0, 64, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    VASSERT(shm, "mmap() failed\n");
    close(fd);
}

static int run(int num)
{
    int numForks = NUM_CHILD;

    pid_t pid;

    *shm = INIT_VAL;

    while(numForks--)
    {
        pid = fork();

        VASSERT(pid != -1, "fork() failed\n");

        if(pid == 0)
            exit(child());
    }

    int status;

    numForks = NUM_CHILD;
    while((pid = wait(&status)) != -1)
    {
        ASSERT(status == 0);
        numForks--;
    }

    printf("run %d/%d   EXPECTED_TOTAL=%ld\n",
        TEST_RUNS - num + 1, TEST_RUNS, (long int) EXPECTED_TOTAL);
    printf("NUN_FORKS=%d\n", NUM_CHILD);

    VASSERT(numForks == 0,
            "wait() did not succeed %d times for the %d children\n",
            NUM_CHILD, NUM_CHILD);

    VASSERT(*shm == EXPECTED_TOTAL, "expected count %ld != %ld\n",
            *shm,  EXPECTED_TOTAL);

    printf("parent finishing with SUCCESS\n\n");
    return 0;
}


int main(void)
{
    int run_count = TEST_RUNS;
    createShm();
    while(run_count-- && run(run_count+1) == 0);
    return 0;
}
