
INCLUDES = -std=c99

yplotdir = $(y_include_dir)

lib_LTLIBRARIES = libyplot-@YPLOT_API_VER@.la

yplot_SOURCES = \
  y-view-interval.c \
  y-axis-markers.c \
  y-element-view.c \
  y-element-view-cartesian.c \
  y-axis-view.c \
  y-scatter-view.c \
  y-plot-widget.c \
  y-density-plot.c
  
yplot_HEADERS = \
  y-view-interval.h \
  y-axis-markers.h \
  y-element-view.h \
  y-element-view-cartesian.h \
  y-axis-view.h \
  y-scatter-view.h \
  y-plot-widget.h \
  y-density-plot.h
  
libyplot_@YPLOT_API_VER@_la_LIBADD = 			\
	$(YPLOT_LIBS)

libyplot_@YPLOT_API_VER@_la_LDFLAGS = -no-undefined -version-info $(VERSION_INFO)

libyplot_@YPLOT_API_VER@_la_SOURCES = \
        $(yplot_SOURCES)
  
AM_CPPFLAGS = \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/src \
	$(YPLOT_CFLAGS)
	
y_include_dir = $(includedir)/libyplot-@YPLOT_API_VER@

noinst_PROGRAMS = stringtest demo

demo_SOURCES = demo.c
demo_LDADD = \
	./libyplot-@YPLOT_API_VER@.la \
	$(YPLOT_LIBS) \
	-lm
	
demo_LDFLAGS = -static

stringtest_SOURCES = stringtest.c
stringtest_LDADD = \
	./libyplot-@YPLOT_API_VER@.la \
	$(YPLOT_LIBS) \
	-lm

stringtest_LDFLAGS = -static

MAINTAINERCLEANFILES = Makefile.in
