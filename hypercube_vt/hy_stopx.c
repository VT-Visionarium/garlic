#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int usage(const char *argv0)
{
    printf(
        "  Usage %s [-h|--help]\n"
        "\n"
        "    Stop all user X sessions. Specifically this signals\n"
        "  the x-session-manager which then does a proper X session\n"
        "  shutdown, like logout.\n"
        "\n", argv0);
    return 1;
}

int main(int argc, char **argv)
{
    int ret;

    if(argc > 1)
        return usage(argv[0]);

    // killall options used:
    //   -e  Require  an exact match for very long names.
    //   -w  wait for all killed processes to die. 
    
    ret = system("/usr/bin/killall -ew x-session-manager");

    if(ret == 0)
    {
        // success we signaled the x-session-manager (or more)
        // so X should have been running.

        // We do the following so that we free-up the DISPLAY numbers
        // that the X server uses, before we start another X server
        // using DISPLAY numbers that are not in our scripts.

        // TODO: make smarter hy_* scripts that find the X display
        // numbers automatically, instead of hard coding them into the
        // scripts.

        // Now wait for X to no longer be running via signal
        // CONT.   This could hang forever if something went wrong.
        system("/usr/bin/killall -ew -s CONT /usr/bin/X");
        // and wait for xinit to exit ?
        system("/usr/bin/killall -ew -s CONT xinit");

        // TODO: check if that failed and why.
        // At this point there not much we'd do about it anyway.
    }

    // If there was no process that's okay.

    // TODO: See if there was a process and check
    // for failure if there was.

    return 0; // success
}



