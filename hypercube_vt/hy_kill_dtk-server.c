#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int usage(const char *argv0)
{
    printf(
        "  Usage %s [-h|--help]\n"
        "\n"
        "    Stop all dtk-server programs. Specifically this signals\n"
        "  the dtk-server so it will exit.\n"
        "\n", argv0);
    return 1;
}

int main(int argc, char **argv)
{
    if(argc > 1)
        return usage(argv[0]);

    // killall options used:
    //   -e  Require  an exact match for very long names.
    //   -w  wait for all killed processes to die. 
    
    system("/usr/bin/killall -ew dtk-server");

    // TODO: See if there was a process and check
    // for failure if there was.
    
    printf("returned from the calling of /usr/bin/killall -ew dtk-server\n");

    return 0; // success
}

