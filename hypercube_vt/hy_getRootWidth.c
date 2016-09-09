
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

#define DISPLAY ":0.0"

int main()
{
    Display* pdsp;
    Window wid;
    XWindowAttributes xwAttr;

    pdsp = XOpenDisplay( DISPLAY );
    if(!pdsp) {
        fprintf(stderr,
            "Failed to open display %s\n",
            DISPLAY);
        printf("0\n");
        return -1;
    }

    wid = DefaultRootWindow( pdsp );
    if(0 > wid) {
        fprintf(stderr, "Failed to obtain the root windows Id "
            "of the default screen of display %s\n",
            DISPLAY);
        printf("0\n");
        return -2;
    }
 
    bzero(&xwAttr, sizeof(xwAttr));
    XGetWindowAttributes(pdsp, wid, &xwAttr );
    printf ("%d\n", xwAttr.width);
    return 0;
}

