bin_PROGRAMS = page-compositor 

AM_CXXFLAGS =  \
	-std=c++11 \
	-rdynamic \
	@LTO@ \
	@WLS_CFLAGS@ \
	@WLC_CFLAGS@ \
	@WLX_CFLAGS@ \
	@WESTON_CFLAGS@ \
	@PIXMAN_CFLAGS@ \
	@CAIRO_CFLAGS@ \
	@PANGO_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-fno-strict-aliasing

BUILT_SOURCES = \
	buffer-manager-protocol.c \
	buffer-manager-server-protocol.h \
	buffer-manager-client-protocol.h \
	xdg-shell-unstable-v5-protocol.c \
	xdg-shell-unstable-v5-server-protocol.h \
	xdg-shell-unstable-v5-interface.cxx \
	xdg-shell-unstable-v5-interface.hxx \
	xdg-shell-unstable-v6-protocol.c \
	xdg-shell-unstable-v6-server-protocol.h \
	xdg-shell-unstable-v6-interface.cxx \
	xdg-shell-unstable-v6-interface.hxx \
	wayland-interface.cxx \
	wayland-interface.hxx

page_compositor_SOURCES = \
	buffer-manager-protocol.c \
	xdg-shell-unstable-v5-protocol.c \
	xdg-shell-unstable-v5-interface.cxx \
	xdg-shell-unstable-v6-protocol.c \
	xdg-shell-unstable-v6-interface.cxx \
	wayland-interface.cxx \
	pointer-grab-handler.cxx \
	pointer-grab-handler.hxx \
	keyboard-grab-handler.cxx \
	keyboard-grab-handler.hxx \
	surface.cxx \
	surface.hxx \
	tree.cxx \
	tree.hxx \
	page.cxx \
	page.hxx \
	xdg-shell-v5-shell.cxx \
	xdg-shell-v5-shell.hxx \
	xdg-shell-v5-surface-base.cxx \
	xdg-shell-v5-surface-base.hxx \
	xdg-shell-v5-surface-toplevel.cxx \
	xdg-shell-v5-surface-toplevel.hxx \
	xdg-shell-v5-surface-popup.cxx \
	xdg-shell-v5-surface-popup.hxx \
	xdg-shell-v6-shell.cxx \
	xdg-shell-v6-shell.hxx \
	xdg-shell-v6-surface.cxx \
	xdg-shell-v6-surface.hxx \
	xdg-shell-v6-toplevel.cxx \
	xdg-shell-v6-toplevel.hxx \
	xdg-shell-v6-popup.cxx \
	xdg-shell-v6-popup.hxx \
	xdg-shell-v6-positioner.cxx \
	xdg-shell-v6-positioner.hxx \
	wl-shell-shell.cxx \
	wl-shell-shell.hxx \
	wl-shell-surface.cxx \
	wl-shell-surface.hxx \
	page-context.hxx \
	view.cxx \
	view.hxx \
	utils.cxx \
	pixmap.cxx \
	grab_handlers.cxx \
	notebook.cxx \
	split.cxx \
	viewport.cxx \
	workspace.cxx \
	page_root.cxx \
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
	mainloop.hxx \
	page.hxx \
	utils.hxx \
	buffer-manager.cxx \
	buffer-manager.hxx

page_compositor_LDADD = \
	@LTO@ \
	@PIXMAN_LIBS@ \
	@WLS_LIBS@ \
	@WLC_LIBS@ \
	@WLX_LIBS@ \
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

%-interface.hxx : $(top_srcdir)/protocol/%.xml
	$(top_srcdir)/tools/wayland-cxx-scanner.py header $< > $@

%-interface.cxx : $(top_srcdir)/protocol/%.xml %-interface.hxx
	$(top_srcdir)/tools/wayland-cxx-scanner.py code $< > $@

clean-local:
	rm -f xdg-shell-protocol.c
	rm -f xdg-shell-server-protocol.h
	rm -f buffer-manager-protocol.c
	rm -f buffer-manager-server-protocol.h
	rm -f buffer-manager-client-protocol.h
	rm -f xdg-shell-unstable-v5-protocol.c
	rm -f xdg-shell-unstable-v5-server-protocol.h
	rm -f xdg-shell-unstable-v5-interface.cxx
	rm -f xdg-shell-unstable-v5-interface.hxx
	rm -f wayland-interface.cxx
	rm -f wayland-interface.hxx

