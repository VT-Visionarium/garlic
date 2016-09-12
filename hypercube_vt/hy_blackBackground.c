#include <stdio.h>
#include <stdbool.h>
#include <gtk/gtk.h>


static int usage(const char *argv0)
{
    printf("  Usage: %s [YMIN [YMAX [TEXT]]]\n", argv0);

    printf("  Draws a black window the width of the screen.\n"
        "\n"
        "In X11 drawing coordinates Y increases downward.  If\n"
        "YMIN is less than zero than the window will be drawn from\n"
        "from the top of the screen to -YMIN and YMAX will be ignored.\n"
            );
    return 1;
}

static
bool draw(GtkWidget *da, cairo_t *cr, void *rec)
{
    //printf("%s()\n", __func__);
#if 0
    guint width, height;
    width = gtk_widget_get_allocated_width(da);
    height = gtk_widget_get_allocated_height(da);
#endif
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);
    //cairo_destroy(cr);
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

static inline runGtk(int ymin, int ymax)
{
    GtkWidget *window;
    GtkWidget *da;
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), getRootWidth(), ymax - ymin);
    gtk_window_move(GTK_WINDOW(window), 0, ymin);
    //gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_window_set_has_resize_grip(GTK_WINDOW(window), FALSE);
    g_signal_connect(window, "destroy", gtk_main_quit, NULL);
    da = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), da);
    g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(draw), window);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_widget_show_all(window);
    gtk_main();
}


int main(int argc, char **argv)
{
    int ymin = 0, ymax;
    int rootHeight;
    
    gtk_init(NULL, NULL);
    rootHeight = getRootHeight();
    ymax = rootHeight;

    if(argc > 1 &&
        (!strncmp(argv[1], "--", 2) || !strncmp(argv[1], "-h", 2))
        )
        return usage(argv[0]);

    if(argc > 1)
        ymin = strtol(argv[1], NULL, 10);
    if(argc > 2)
        ymax = strtol(argv[2], NULL, 10);

    if(ymax < 1)
        ymax = 1;
    else if(ymax > rootHeight)
        ymax = rootHeight;

    if(ymin < 0)
    {
        ymax = - ymin;
        ymin = 0;
    }
    else if(ymin > ymax)
        ymin = ymax - 1;

    runGtk(ymin, ymax);
    return 0;
}
