bin_PROGRAMS = page-compositor 

AM_CXXFLAGS =  \
	-std=c++11 \
	-rdynamic \
	@LTO@ \
	@PIXMAN_CFLAGS@ \
	@WESTON_CFLAGS@ \
	@WLS_CFLAGS@ \
	@WLC_CFLAGS@ \
	-fno-strict-aliasing

BUILT_SOURCES = \
	xdg-shell-server-protocol.h

page_compositor_SOURCES = \
	page-compositor.cxx \
	compositor.cxx \
	xdg-shell.cxx \
	xdg-surface.cxx \
	xdg-popup.cxx \
	xdg-shell-protocol.c \
	xdg-shell-server-protocol.h

page_compositor_LDADD = \
	@LTO@ \
	@PIXMAN_LIBS@ \
	@WESTON_LIBS@ \
	@WLS_LIBS@ \
	@WLC_LIBS@ \
	@RT_LIBS@ 

%-protocol.c : $(top_srcdir)/protocol/%.xml
	@wayland_scanner@ code < $< > $@

%-server-protocol.h : $(top_srcdir)/protocol/%.xml
	@wayland_scanner@ server-header < $< > $@

%-client-protocol.h : $(top_srcdir)/protocol/%.xml
	@wayland_scanner@ client-header < $< > $@

