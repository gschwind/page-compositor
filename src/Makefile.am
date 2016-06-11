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
	pointer-grab-handler.cxx \
	pointer-grab-handler.hxx \
	page.cxx \
	page.hxx \
	page-context.hxx \
	client_base.cxx \
	client_base.hxx \
	client_managed.cxx \
	client_managed.hxx \
	client_not_managed.cxx \
	client_not_managed.hxx \
	display-compositor.cxx \
	display-compositor.hxx \
	tree.cxx \
	tree.hxx \
	utils.cxx \
	pixmap.cxx \
	grab_handlers.cxx \
	notebook.cxx \
	split.cxx \
	viewport.cxx \
	workspace.cxx \
	page_root.cxx \
	display.cxx \
	simple2_theme.cxx \
	tiny_theme.cxx \
	config_handler.cxx \
	popup_alt_tab.cxx \
	main.cxx \
	blur_image_surface.cxx \
	leak_checker.cxx \
	blur_image_surface.hxx \
	notebook.hxx \
	client_proxy.hxx \
	config_handler.hxx \
	workspace.hxx \
	split.hxx \
	viewport.hxx \
	grab_handlers.hxx \
	display.hxx \
	mainloop.hxx \
	page.hxx \
	utils.hxx \
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

