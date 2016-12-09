/*
 * managed_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef XDG_SURFACE_TOPLEVEL_HXX_
#define XDG_SURFACE_TOPLEVEL_HXX_

#include <string>
#include <vector>
#include <typeinfo>
#include <compositor.h>

#include "tree-types.hxx"

#include "xdg-shell-unstable-v5-interface.hxx"
#include "icon_handler.hxx"
#include "theme.hxx"

#include "floating_event.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_pixmap.hxx"

#include "xdg-surface-base.hxx"
#include "xdg-surface-popup.hxx"
#include "page-surface-interface.hxx"

namespace page {

class page_t;

using namespace wcxx;
using namespace std;

struct xdg_surface_toplevel_t :
	public xdg_surface_base_t,
	public xdg_surface_vtable,
	public page_surface_interface {

	friend class page::page_t;

	struct _state {
		std::string title;
		bool fullscreen;
		bool maximized;
		bool minimized;
		wl_resource * transient_for;
		rect geometry;

		_state() {
			fullscreen = false;
			maximized = false;
			minimized = false;
			title = "";
			transient_for = nullptr;
			geometry = rect{0,0,0,0};
		}

	} _pending, _current;

	/* 0 if ack by client, otherwise the last serial sent */
	uint32_t _ack_serial;

	signal_t<xdg_surface_toplevel_t *> destroy;

	/* private to avoid copy */
	xdg_surface_toplevel_t(xdg_surface_toplevel_t const &) = delete;
	xdg_surface_toplevel_t & operator=(xdg_surface_toplevel_t const &) = delete;

	static void delete_resource(wl_resource *resource);

	/** called on surface commit */
	virtual void surface_commited(weston_surface * es) override;
	virtual void surface_destroyed(weston_surface * es) override;

	static auto get(wl_resource * r) -> xdg_surface_toplevel_t *;

	xdg_surface_toplevel_t(
			page_context_t * ctx,
			wl_client * client,
			weston_surface * surface,
			uint32_t id);
	virtual ~xdg_surface_toplevel_t();

	/* read only attributes */
	auto resource() const -> wl_resource *;

	void set_title(char const *);
	void set_transient_for(xdg_surface_toplevel_t * s);
	auto transient_for() const -> xdg_surface_toplevel_t *;

	auto create_view() -> view_p;
	auto master_view() -> view_w;

	virtual page_surface_interface * page_surface();

	void minimize();

	static xdg_surface_toplevel_t * get(weston_surface * surface);

	/**
	 * xdg-surface-interface (request)
	 **/
	virtual void xdg_surface_destroy(wl_client * client, wl_resource * resource)
			override;
	virtual void xdg_surface_set_parent(wl_client * client,
			wl_resource * resource, wl_resource * parent_resource) override;
	virtual void xdg_surface_set_app_id(wl_client * client,
			wl_resource * resource, const char * app_id) override;
	virtual void xdg_surface_show_window_menu(wl_client * client,
			wl_resource * surface_resource, wl_resource * seat_resource,
			uint32_t serial, int32_t x, int32_t y) override;
	virtual void xdg_surface_set_title(wl_client * client,
			wl_resource * resource, const char * title) override;
	virtual void xdg_surface_move(wl_client * client, wl_resource * resource,
			wl_resource* seat_resource, uint32_t serial) override;
	virtual void xdg_surface_resize(wl_client* client, wl_resource * resource,
			wl_resource * seat_resource, uint32_t serial, uint32_t edges)
					override;
	virtual void xdg_surface_ack_configure(wl_client * client,
			wl_resource * resource, uint32_t serial) override;
	virtual void xdg_surface_set_window_geometry(wl_client * client,
			wl_resource * resource, int32_t x, int32_t y, int32_t width,
			int32_t height) override;
	virtual void xdg_surface_set_maximized(wl_client * client,
			wl_resource * resource) override;
	virtual void xdg_surface_unset_maximized(wl_client* client,
			wl_resource* resource) override;
	virtual void xdg_surface_set_fullscreen(wl_client * client,
			wl_resource * resource, wl_resource * output_resource) override;
	virtual void xdg_surface_unset_fullscreen(wl_client * client,
			wl_resource * resource) override;
	virtual void xdg_surface_set_minimized(wl_client * client,
			wl_resource * resource) override;

	virtual void xdg_surface_delete_resource(wl_resource *resource) override;

	/* page_surface_interface */
	virtual weston_surface * surface() const override;
	virtual weston_view * create_weston_view() override;
	virtual int32_t width() const override;
	virtual int32_t height() const override;
	virtual string const & title() const override;
	virtual void send_configure(int32_t width, int32_t height, set<uint32_t> const & states) override;
	virtual void send_close() override;
	virtual void send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) override;

};


}


#endif /* XDG_SURFACE_TOPLEVEL_HXX_ */
