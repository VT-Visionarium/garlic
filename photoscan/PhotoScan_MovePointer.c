#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

int
main(int argc, char **argv)
{
    Display *dpy;
    int x = 100, y = 100;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
	fprintf(stderr, "%s: cannot connect to display\n", argv[0]);
	return 1;
    }

    XWarpPointer(dpy, None, RootWindow(dpy, DefaultScreen(dpy)),
		  0, 0, 0, 0, x, y);

    XCloseDisplay(dpy);

    return 0;
}
