#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <gtk/gtk.h>


static inline
int usage(const char *argv0)
{
    printf("  Usage: %s [--sync][-h|--help]\n\n", argv0);

    printf("  A simple GTK+ program to text that the X11 window manager is\n"
        "running.  This makes all X requests synchronously so as we can tell\n"
        "when X11 is running and ready.\n"
        "\n"
        "                  OPTIONS\n"
        "\n"
        "   --sync    exit returning 0 (success) after the first draw event\n"
        "\n"
            );
    return 1; // fail
}

static bool
exitOnDrawFlag = false;

#if 0 // focus event came before draw
// so we do not use it.
static
bool focus(GtkWidget *w, GtkDirectionType direction,
               gpointer user_data)
{
    printf("%s(%p, %d, %p)\n", __func__,
            w, direction, user_data);
    return FALSE;
}
#endif

static
bool draw(GtkWidget *da, cairo_t *cr, void *rec)
{
    printf("%s()\n", __func__);
#if 0
    guint width, height;
    width = gtk_widget_get_allocated_width(da);
    height = gtk_widget_get_allocated_height(da);
#endif
    cairo_set_source_rgb(cr, 0.1, 0.9, 0.9);
    cairo_paint(cr);
    //cairo_destroy(cr);
    if(exitOnDrawFlag)
        // success
        exit(0);
    return FALSE;
}
    
static inline
int getRootWidth(void)
{
    return gdk_window_get_width(gdk_get_default_root_window());
}

static inline
int getRootHeight(void)
{
     return gdk_window_get_height(gdk_get_default_root_window());
}

static inline void runGtk(void)
{
    GtkWidget *window;
    GtkWidget *da;
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window),
            8*getRootWidth()/9, 7*getRootHeight()/8);
    gtk_window_move(GTK_WINDOW(window), 0, 0);
    //gtk_window_fullscreen(GTK_WINDOW(window));
    //gtk_window_set_has_resize_grip(GTK_WINDOW(window), FALSE);
    g_signal_connect(window, "destroy", gtk_main_quit, NULL);
    //g_signal_connect(window, "focus",  G_CALLBACK(focus), NULL);
    da = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), da);
    g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(draw), window);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_widget_show_all(window);
    gtk_main();
}


int main(int argc, char **argv)
{
    if(argc > 2 || (argc > 1 && strcmp(argv[1], "--sync")))
        return usage(argv[0]);
    if(argc == 2 && strcmp(argv[1],"--sync") == 0)
        exitOnDrawFlag = true;

    gtk_init(&argc, &argv);
    runGtk();
    return 1; // The draw() exit we define as the successful test.
}
