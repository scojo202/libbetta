#include <gtk/gtk.h>
#include <math.h>
#include "y-element-view.h"

GtkWidget *window;

static void
init (gint argc, gchar *argv[])
{
  gtk_init(&argc, &argv);
}

static
gboolean
draw (GtkWidget *da,
                   cairo_t   *cr,
                   gpointer   data)
{
  int width=gtk_widget_get_allocated_width(da);
  int height=gtk_widget_get_allocated_height(da);
  Point p = {width/2,height/2};
  cairo_arc(cr,p.x,p.y,2,0,2*G_PI);
  cairo_close_path(cr);
  cairo_fill(cr);
  string_draw(cr,pango_font_description_from_string("Sans 10"),p,ANCHOR_BOTTOM,ROT_270,"Anchor bottom");
  return TRUE;
}

int
main (int argc, char *argv[])
{

  init (argc, argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  GtkWidget *area = gtk_drawing_area_new();

  g_signal_connect (area, "draw",
                        G_CALLBACK (draw), NULL);

  gtk_container_add(GTK_CONTAINER(window),area);

  gtk_widget_show_all(window);

  gtk_main ();

  return 0;
}
