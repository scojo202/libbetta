AM_CPPFLAGS = \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/src \
	$(YPLOT_CFLAGS)

yplot_include_dir = $(includedir)/libyplot-@YPLOT_API_VER@/yplot
