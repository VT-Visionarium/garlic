// This is just so I can debug Bill's code by inserting SPEW()
// and ASSERT() 


#if 1 // SPEW and ASSERT or not

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>

static void _spew(const char *file, int line, const char *func,
        int pid, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("%s:%d:%s()[%d]: ", file, line, func, pid);
    vprintf(fmt, ap);
    va_end(ap);
}

static int _assert(bool x, const char *xstr, const char *file, 
        int line, const char *func, int pid, const char *fmt, ...)
{
    if(!x)
    {
        va_list ap;
        va_start(ap, fmt);
        printf("%s:%d:%s()[%d]:ASSERTION(%s) FAILED: ", file, line, func, pid, xstr);
        vprintf(fmt, ap);
        va_end(ap);
        printf("\nWILL SLEEP NOW\n");
        while(1) usleep(1000);
    }
}


#  define SPEW(fmt, ...)        _spew(__FILE__, __LINE__, __func__, getpid(), fmt, ## __VA_ARGS__)
#  define ASSERT(x)             _assert(x, #x, __FILE__, __LINE__, __func__, getpid(), " ")
#  define VASSERT(x, fmt, ...)  _assert(x, #x, __FILE__,  __LINE__, __func__, getpid(), fmt, ## __VA_ARGS__)


#else

#  define SPEW(fmt, ...)         /* empty macro */
#  define ASSERT(x, fmt, ...)    /* empty macro */
#  define VASSERT(x, fmt, ...)   /* empty macro */

#endif
