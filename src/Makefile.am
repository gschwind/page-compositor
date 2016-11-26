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
	xdg-shell-unstable-v6-interface.hxx

page_compositor_SOURCES = \
	buffer-manager-protocol.c \
	xdg-shell-unstable-v5-protocol.c \
	xdg-shell-unstable-v5-interface.cxx \
	xdg-shell-unstable-v6-protocol.c \
	xdg-shell-unstable-v6-interface.cxx \
	pointer-grab-handler.cxx \
	pointer-grab-handler.hxx \
	keyboard-grab-handler.cxx \
	keyboard-grab-handler.hxx \
	tree.cxx \
	tree.hxx \
	page.cxx \
	page.hxx \
	xdg-shell-client.cxx \
	xdg-shell-client.hxx \
	xdg-shell-v6-client.cxx \
	xdg-shell-v6-client.hxx \
	xdg-surface-v6.cxx \
	xdg-surface-v6.hxx \
	xdg-toplevel-v6.cxx \
	xdg-toplevel-v6.hxx \
	wl-shell-client.cxx \
	wl-shell-client.hxx \
	page-context.hxx \
	xdg-surface-base.cxx \
	xdg-surface-base.hxx \
	xdg-surface-toplevel.cxx \
	xdg-surface-toplevel.hxx \
	xdg-surface-popup.cxx \
	xdg-surface-popup.hxx \
	xdg-surface-base-view.cxx \
	xdg-surface-base-view.hxx \
	xdg-surface-toplevel-view.cxx \
	xdg-surface-toplevel-view.hxx \
	xdg-surface-popup-view.cxx \
	xdg-surface-popup-view.hxx \
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

