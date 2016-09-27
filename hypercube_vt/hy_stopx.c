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
        "  shutdown, like logout.  This runs until it sees that the\n"
        "  x session has stopped, so that when this returns you can be\n"
        "  assured that the X session no longer exists.\n"
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
    
    ret = system("/usr/bin/killall -ew xfce4-session");

    if(ret == 0)
    {
        // success we signaled the xfce4-session (zero or more)
        // so X should have been running.

        // We do the following so that we free-up the DISPLAY numbers
        // that the X server uses, before we start another X server
        // using DISPLAY numbers that are not in our scripts.

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

    printf("%s finished\n", argv[0]);

    return 0; // success
}



