#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int usage(const char *argv0)
{
    printf(
        "  Usage %s [-h|--help]\n"
        "\n"
        "  Run \"dtk-server is900\", and other things.\n"
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
   
    system("dtk-server is900 &");

    char *disEnv;
    disEnv = getenv("DISPLAY");
    printf("DISPLAY=\"%s\"\n", disEnv);

    if(!disEnv)
        setenv("DISPLAY", "localhost:10.0", 1);

    system("dtk-floatSliders head"
        " -u"
        " -g 460x180+1327+44"
        " -N 6"
        " -s 0 -1.5 1.5 0"
        " -s 1 -1.5 1.5 0"
        " -s 2 -1.5 1.5 0"
        " -s 3 -190 190 0"
        " -s 4 -190 190 0"
        " -s 5 -190 190 0"
        " -l x y z H P R &");

    system("dtk-floatSliders wand"
        " -u"
        " -g 460x180+1324+246"
        " -N 6"
        " -s 0 -1.5 1.5 0"
        " -s 1 -1.5 1.5 0"
        " -s 2 -1.5 1.5 0"
        " -s 3 -190 190 0"
        " -s 4 -190 190 0"
        " -s 5 -190 190 0"
        " -l x y z H P R &");

    system("dtk-floatSliders joystick"
        " -u"
        " -g 460x80+1322+449"
        " -N 2"
        " -s 0 -1.1 1.1 0"
        " -s 1 -1.1 1.1 0"
        " -l x y &");

    system("dtk-buttons buttons"
        " -g 120x290-5+44 &");

    // Last one
    system("xfce4-terminal --geometry 148x7-0+522  -x dtk-readCAVEDevices &");

    // TODO: See if there was a process and check
    // for failure if there was.

    return 0; // success
}

