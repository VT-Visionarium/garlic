#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <gdk/gdkdisplay.h>
#include <gtk/gtk.h>

#define ASSERT(x) \
    myAssert((x), #x, __FILE__, __func__, __LINE__)

static void
myAssert(bool x, const char *valstr, const char *file,
        const char *func, int line)
{
    if(!x)
    {
        fprintf(stderr, "%s: %s:%d ASSERT(%s) FAILED: sleeping here\n",
                func, file, line, valstr);
        while(1)
            usleep(100);
    }
}

static
void makeWin(GdkDisplay *dsp)
{
    GtkWidget *colorWidget, *win;
  
    colorWidget = gtk_color_selection_new();
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_screen(GTK_WINDOW(win),
            gdk_display_get_default_screen(dsp));

    gtk_container_add(GTK_CONTAINER(win), colorWidget);

    g_signal_connect(GTK_OBJECT(win), "delete-event", 
                    G_CALLBACK (gtk_main_quit), NULL);

    gtk_widget_show_all(GTK_WIDGET(win));
}


int main(int argc, char **argv)
{
    GdkDisplay *dsp;

    /* Initialize the widget set */
    gtk_init(&argc, &argv);

    makeWin(gdk_display_get_default());

    ASSERT(dsp = gdk_display_open(":0.1"));
    makeWin(dsp);

    /* Enter the main event loop, and wait for user interaction */
    gtk_main();

    return 0;
}
