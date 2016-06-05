bin_PROGRAMS = page-compositor 

AM_CXXFLAGS =  \
	-std=c++11 \
	-rdynamic \
	@LTO@ \
	@PIXMAN_CFLAGS@ \
	@CAIRO_CFLAGS@ \
	@PANGO_CFLAGS@ \
	@WAYLAND_CFLAGS@ \
	@WESTON_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-fno-strict-aliasing

BUILT_SOURCES = \
	xdg-shell-server-protocol.h

page_compositor_SOURCES = \
	page-compositor.cxx \
	compositor.cxx \
	pixmap.cxx \
	utils.cxx \
	config-handler.cxx \
	config-handler.hxx \
	xdg-shell.cxx \
	xdg-surface.cxx \
	xdg-popup.cxx \
	simple2-theme.cxx \
	tiny-theme.cxx \
	tree.cxx \
	page-root.cxx \
	xdg-shell-protocol.c \
	xdg-shell-server-protocol.h

page_compositor_LDADD = \
	@LTO@ \
	@PIXMAN_LIBS@ \
	@WAYLAND_LIBS@ \
	@WESTON_LIBS@ \
	@CAIRO_LIBS@ \
	@PANGO_LIBS@ \
	@GLIB_LIBS@ \
	@RT_LIBS@ 

%-protocol.c : $(top_srcdir)/protocol/%.xml
	@wayland_scanner@ code < $< > $@

%-server-protocol.h : $(top_srcdir)/protocol/%.xml
	@wayland_scanner@ server-header < $< > $@

%-client-protocol.h : $(top_srcdir)/protocol/%.xml
	@wayland_scanner@ client-header < $< > $@

