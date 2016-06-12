AC_INIT([page-compositor], [0.0.1])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/main.cxx])
AC_CONFIG_HEADERS([src/config.hxx])

CHECK_GNU_MAKE

dnl define custom macro to setup dir from datadir

dnl Check for required programs
AC_PROG_CXX
AC_PROG_LIBTOOL

# check for C++11
AX_CXX_COMPILE_STDCXX_11

PKG_CHECK_MODULES(CAIRO, cairo >= 1.0.0)
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_LIBS)

PKG_CHECK_MODULES(PANGO, pangocairo >= 0.28)
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)

PKG_CHECK_MODULES(WESTON, [libweston-0 >= 1.11.90])
AC_SUBST(WESTON_CFLAGS)
AC_SUBST(WESTON_LIBS)

PKG_CHECK_MODULES(WAYLAND, wayland-server >= 1.9.0)
AC_SUBST(WAYLAND_CFLAGS)
AC_SUBST(WAYLAND_LIBS)

PKG_CHECK_MODULES(PIXMAN, pixman-1 >= 0.32.8)
AC_SUBST(PIXMAN_CFLAGS)
AC_SUBST(PIXMAN_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.0.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AC_CHECK_LIB(rt, clock_gettime,
    [RT_LIBS="-lrt"
     AC_DEFINE(HAVE_CLOCK_GETTIME, 1,
	       [Define to 1 if you have the `clock_gettime` function.])])
AC_SUBST(RT_LIBS)

eval xdatadir=${datadir}
eval xdatadir=${xdatadir}
eval xdatadir=${xdatadir}
eval xdatadir=${xdatadir}
AC_DEFINE_UNQUOTED([DATA_DIR], ["${xdatadir}"], [Data directory (/usr/share)])

AC_PATH_PROG([has_wayland_scanner], [wayland-scanner], [yes], [no])
if test $has_wayland_scanner = yes; then
	wayland_scanner="wayland-scanner"
else
	dnl If wayland scanner is not found, try another way.
	PKG_CHECK_MODULES(WAYLAND_SCANNER, [wayland-scanner])
	wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
fi

AC_SUBST(wayland_scanner)

dnl Try LTO flags.
OLD_LDFLAGS="${LDFLAGS}"
LDFLAGS="${LDFLAGS} -flto"
AC_LINK_IFELSE([AC_LANG_SOURCE([int main () { return 0; }])],[
 AC_SUBST(LTO, [-flto])
])
LDFLAGS="${OLD_LDFLAGS}"

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  ])
AC_OUTPUT
