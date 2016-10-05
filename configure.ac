>-*- mode: m4 -*-
dnl We require Automake 1.7.2, which requires Autoconf 2.54.
dnl (It needs _AC_AM_CONFIG_HEADER_HOOK, for example.)
AC_PREREQ(2.54)

m4_define([yplot_version_epoch], [0])
m4_define([yplot_version_major], [0])
m4_define([yplot_version_minor], [0])
m4_define([yplot_version_extra], [])
m4_define([yplot_full_version],
	  [yplot_version_epoch.yplot_version_major.yplot_version_minor[]yplot_version_extra])

AC_INIT([yplot], [yplot_full_version],
	[])

AC_CONFIG_SRCDIR(src/y-element-view.h)
AM_INIT_AUTOMAKE([dist-bzip2])

dnl Version info for libraries = CURRENT:REVISION:AGE
dnl
dnl Within each x.y.*, ABI is maintained backward and _forward_ compatible.
dnl (As a consequence, no exported function may be added.)
dnl So it's enough to have one interface number per each x.y.* branch.
dnl
dnl OTOH, we are not able to keep ABI strictly backward compatible throughout
dnl the whole x.*.*.
dnl The easiest way is to declare no ABI compatibility, ie. AGE is always 0.
dnl
m4_define([version_iface],
	  m4_eval(100 * yplot_version_epoch + yplot_version_major))

AC_SUBST([VERSION_INFO], [version_iface:yplot_version_minor:0])
AC_SUBST([VERSION_IFACE], [version_iface])

if test `expr yplot_version_major % 2` -eq 1; then
    AC_MSG_NOTICE([NOTE: This is a development release])
    yplot_devel=yes
else
    yplot_devel=no
fi

if test `expr yplot_version_major % 2` -eq 1; then
    AC_MSG_NOTICE([NOTE: This is a development release])
    yplot_devel=yes
    yplot_api_ver=yplot_version_epoch.`expr yplot_version_major + 1`
    dnl An explicit version number should be used in place of the above
    dnl if the epoch is to change.
    dnl yplot_api_ver=1.0
else
    yplot_devel=no
    yplot_api_ver=yplot_version_epoch.yplot_version_major
fi
yplot_api_ver_=`echo $yplot_api_ver | sed -e 's/\./_/g'`

dnl Almost like epoch.major but development versions look forward to the next
dnl stable release.
AC_SUBST([YPLOT_API_VER], [$yplot_api_ver])
AC_SUBST([YPLOT_API_VER_], [$yplot_api_ver_])

dnl This one is created by autoheader, ...
AC_CONFIG_HEADERS(src/y-config.h)

AM_MAINTAINER_MODE([enable])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl We use $host_os:
AC_CANONICAL_HOST

AC_ISC_POSIX
AC_PROG_CC_C99
AC_PROG_LN_S
AC_HEADER_STDC
AC_SYS_LARGEFILE

LT_PREREQ([2.2.6])
LT_INIT([])

dnl Propagate Gnome-specific variable ACLOCAL_FLAGS to Makefile.
AC_SUBST(ACLOCAL_AMFLAGS, $ACLOCAL_FLAGS)

dnl The first call to PKG_CHECK_MODULES is inside an `if.'
dnl We have to call PKG_PROG_PKG_CONFIG explicitly; otherwise it would
dnl get expanded just before the first occurence of PKG_CHECK_MODULES
dnl and might escape execution.
dnl
dnl While we're at it, we might require a version that supports private libraries
dnl	PKG_PROG_PKG_CONFIG([0.18])
dnl but there are two problems:
dnl 1) we cannot rule out something as new as SuSE 9.3
dnl 2) current versions of PKG_PROG_PKG_CONFIG choke on old versions and woe a
dnl	"syntax error" instead of "your pkgconfig/pkg-config is too old".
dnl Let it be, for now.
PKG_PROG_PKG_CONFIG

dnl *****************************
yplot_reqs="
	glib-2.0		>= 2.44.0
	gobject-2.0		>= 2.44.0
	gtk+-3.0    >= 3.18.0
	y-data-0.0   >= 0.0
"

EXTRA_DEPS=
EXTRA_LIBS=
EXTRA_INCLUDES=

dnl ****************************
dnl now that we have selected out libraries the whole collection in one
dnl shot so that we can have a nice neat compile/link line
dnl ****************************

PKG_CHECK_MODULES(YPLOT, $yplot_reqs)

AC_ARG_VAR(GLIB_GENMARSHAL, [The glib-genmarshal executable.])
AC_CHECK_PROG(GLIB_GENMARSHAL, glib-genmarshal, glib-genmarshal)

AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])

## this should come after `AC_PROG_CC'
ifdef([GNOME_COMPILE_WARNINGS],
	[GNOME_COMPILE_WARNINGS] CFLAGS="$CFLAGS $WARN_CFLAGS",
	[]
)
set_more_warnings=yes
if test "$GCC" = yes -a "x$set_more_warnings" != xno; then
	dnl Clang needs this option, or else it will appear to support any
	dnl warning option, only to spew warnings about them later.
	uwoption="-Werror=unknown-warning-option"
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $uwoption"
	AC_MSG_CHECKING([whether gcc understands $uwoption])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([], [])],
		[has_unknown_warning_option=yes],
		[has_unknown_warning_option=no; uwoption=""])
	AC_MSG_RESULT($has_unknown_warning_option)
	CFLAGS="$SAVE_CFLAGS"

	warning_options="-Wsign-compare -Wpointer-arith \
			 -Wchar-subscripts -Wwrite-strings \
			 -Wnested-externs \
			 -Wmissing-noreturn \
			 -Werror=missing-prototypes -Werror=nested-externs \
			 -Werror=implicit-function-declaration \
			 -Wmissing-declarations -Wno-pointer-sign \
			 -Werror=format-security -Wbitwise -Wcast-to-as \
			 -Wdefault-bitfield-sign -Wdo-while -Wparen-string \
			 -Wptr-subtraction-blows -Wreturn-void -Wtypesign \
			 -Wstrict-prototypes -Wno-error=format-nonliteral "
	for option in $warning_options ; do
		SAVE_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $uwoption $option"
		AC_MSG_CHECKING([whether gcc understands $option])
		dnl Include a system header so we ignore Werror=... flags
		dnl that cause trouble.
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM([[#include <stdio.h>]], [])],
			[has_option=yes],
			[has_option=no])
		CFLAGS="$SAVE_CFLAGS"
		if test $has_option = yes; then
			CFLAGS="$CFLAGS $option"
		fi
		AC_MSG_RESULT($has_option)
		unset has_option
		unset SAVE_CFLAGS
	done
	unset option
fi
AC_SUBST(WARN_CFLAGS)

CFLAGS="$CFLAGS -D_ISOC99_SOURCE -std=c99"
AC_MSG_NOTICE([adding -D_ISOC99_SOURCE to CFLAGS])

# GObject Introspection
GIR_REQ=1.0.0
AC_ARG_ENABLE(introspection,
          AS_HELP_STRING([--enable-introspection[=@<:@no/auto/yes@:>@]],
                         [Enable introspection for this build]),,
                         [enable_introspection=no])

AC_MSG_CHECKING([for gobject-introspection])

dnl presence/version checking
AS_CASE([$enable_introspection],
[no], [
    found_introspection="no (disabled, use --enable-introspection to enable)"
],
[yes],[
    PKG_CHECK_EXISTS([gobject-introspection-1.0],,
                     AC_MSG_ERROR([gobject-introspection-1.0 is not installed]))
    PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $GIR_REQ libgsf-1 >= 1.14.23],
                     found_introspection=yes,
                     AC_MSG_ERROR([You need to have gobject-introspection >= $GIR_REQ and libgsf >= 1.14.23 installed to build AC_PACKAGE_NAME]))
],
[auto],[
    PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $GIR_REQ libgsf-1 >= 1.14.23],
                     found_introspection=yes, found_introspection=no)
dnl Canonicalize enable_introspection
enable_introspection=$found_introspection
],
[	
    AC_MSG_ERROR([invalid argument passed to --enable-introspection, should be one of @<:@no/auto/yes@:>@])
])

AC_MSG_RESULT([$found_introspection])

if test "x$found_introspection" = "xyes"; then
   dnl You can override INTROSPECTION_GIRDIR and INTROSPECTION_TYPELIBDIR
   dnl if you wish.  If you override the latter, you might want to set
   dnl GI_TYPELIB_PATH to include the same directory.  For example
   dnl
   dnl GI_TYPELIB_PATH=$PREFIX/lib64/girepository-1.0
   dnl INTROSPECTION_TYPELIBDIR=$GI_TYPELIB_PATH
   dnl INTROSPECTION_GIRDIR=$PREFIX/share/gir-1.0
   dnl
   dnl Note, that unlike binaries produced with y-plot, nothing tells
   dnl python where to find y-plot, so you might also need to set
   dnl LD_LIBRARY_PATH.

   INTROSPECTION_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
   INTROSPECTION_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`
   INTROSPECTION_GENERATE=`$PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0`
   if test "x$INTROSPECTION_GIRDIR" = x; then
     INTROSPECTION_GIRDIR=`$PKG_CONFIG --variable=girdir gobject-introspection-1.0`
   fi
   if test "x$INTROSPECTION_TYPELIBDIR" = x; then
     INTROSPECTION_TYPELIBDIR="$($PKG_CONFIG --variable=typelibdir gobject-introspection-1.0)"
   fi
   INTROSPECTION_CFLAGS=`$PKG_CONFIG --cflags gobject-introspection-1.0`
   INTROSPECTION_LIBS=`$PKG_CONFIG --libs gobject-introspection-1.0`
   INTROSPECTION_MAKEFILE=`$PKG_CONFIG --variable=datadir gobject-introspection-1.0`/gobject-introspection-1.0/Makefile.introspection
else
   INTROSPECTION_SCANNER=
   INTROSPECTION_COMPILER=
   INTROSPECTION_GENERATE=
   INTROSPECTION_GIRDIR=
   INTROSPECTION_TYPELIBDIR=
   INTROSPECTION_CFLAGS=
   INTROSPECTION_LIBS=
   INTROSPECTION_MAKEFILE=
fi
AC_SUBST(INTROSPECTION_SCANNER)
AC_SUBST(INTROSPECTION_COMPILER)
AC_SUBST(INTROSPECTION_GENERATE)
AC_SUBST(INTROSPECTION_GIRDIR)
AC_SUBST(INTROSPECTION_TYPELIBDIR)
AC_SUBST(INTROSPECTION_CFLAGS)
AC_SUBST(INTROSPECTION_LIBS)
AC_SUBST(INTROSPECTION_MAKEFILE)

AM_CONDITIONAL(HAVE_INTROSPECTION, test "x$found_introspection" = "xyes")
dnl we need to change the install directories for distcheck
AC_ARG_WITH([gir-dir],
	AS_HELP_STRING(
		[--with-gir-dir],
		[
		 path to gir repository
		 (automatically detected via pkg-config)
		]
	),
	[GIRDIR=$withval],
	[GIRDIR=$INTROSPECTION_GIRDIR]
)
AC_SUBST(GIRDIR)
AC_ARG_WITH([typelib-dir],
	AS_HELP_STRING(
		[--with-typelib-dir],
		[
		 path to typelibs repository
		 (automatically detected via pkg-config)
		]
	),
	[TYPELIBDIR=$withval],
	[TYPELIBDIR=$INTROSPECTION_TYPELIBDIR]
)
AC_SUBST(TYPELIBDIR)

dnl ******************
dnl * Config defaults
dnl ******************
dnl
dnl These are changed in y-plot.c for WIN32 packages
AC_SUBST(yplot_datadir, '${datadir}/y-plot/${VERSION}')
AC_SUBST(yplot_libdir, '${libdir}/y-plot/${VERSION}')
AC_SUBST(yplot_icondir, '${datadir}/pixmaps/y-plot/${VERSION}')
AC_SUBST(yplot_localedir, '${prefix}/${DATADIRNAME}/locale')

dnl Export to y-config.h and y-features.h
AC_DEFINE(YPLOT_VERSION, "yplot_full_version",
	[The version number of this release, possibly with additional suffix])
AC_DEFINE(YPLOT_VERSION_EPOCH, yplot_version_epoch,
	[The Epoch of this release])
AC_DEFINE(YPLOT_VERSION_MAJOR, yplot_version_major,
	[The Major version number of this release])
AC_DEFINE(YPLOT_VERSION_MINOR, yplot_version_minor,
	[The Minor version number of this release])
AC_DEFINE(YPLOT_VERSION_EXTRA, "yplot_version_extra",
	[Extra, possibly empty tag for this release])

GTK_DOC_CHECK([1.12])

AC_CONFIG_FILES([
Makefile
y-plot.pc
src/Makefile
])
AC_OUTPUT

echo "

Configuration:

	Source code location:	${srcdir}
	Compiler:		${CC}
	Compiler flags:		${CFLAGS}
"
