typedef struct
{
  GtkApplication parent_instance;
} Demo;

typedef GtkApplicationClass DemoClass;

GType demo_get_type (void);
G_DEFINE_TYPE (Demo, demo, GTK_TYPE_APPLICATION)

static void
demo_init (Demo *app)
{
}

static void
demo_class_init (DemoClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);

  application_class->activate = demo_activate;
}

static Demo *
demo_new (void)
{
  Demo *demo;

  g_set_application_name ("Demo");

  demo = g_object_new (demo_get_type (),
                            "application-id", "org.gtk.demo",
                            "register-session", TRUE,
                            NULL);

  return demo;
}

int
main (int argc, char *argv[])
{
  Demo *demo = demo_new();

  int status = g_application_run(G_APPLICATION (demo),argc,argv);

  return status;
}
