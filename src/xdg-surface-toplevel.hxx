/*
 * managed_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_MANAGED_HXX_
#define CLIENT_MANAGED_HXX_

#include <string>
#include <vector>
#include <typeinfo>

#include "xdg-surface-interface.hxx"
#include "icon_handler.hxx"
#include "theme.hxx"

#include "floating_event.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_pixmap.hxx"

#include "xdg-surface-toplevel.hxx"
#include "xdg-surface-popup.hxx"


namespace page {

class page_t;

using namespace std;

struct xdg_surface_toplevel_t : public xdg_surface_vtable, public xdg_surface_base_t {

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

	xdg_surface_toplevel_view_w _master_view;

	/* 0 if ack by client, otherwise the last serial sent */
	uint32_t _ack_serial;

	list<xdg_surface_toplevel_t *> _transient_chiddren;
	list<xdg_surface_popup_t *> _popup_childdren;

	/* private to avoid copy */
	xdg_surface_toplevel_t(xdg_surface_toplevel_t const &) = delete;
	xdg_surface_toplevel_t & operator=(xdg_surface_toplevel_t const &) = delete;

	static void xdg_surface_delete(wl_resource *resource);

	/** called on surface commit */
	void weston_configure(weston_surface * es, int32_t sx, int32_t sy);

	static auto get(wl_resource * r) -> xdg_surface_toplevel_t *;

	xdg_surface_toplevel_t(
			page_context_t * ctx,
			xdg_shell_client_t * xdg_shell_client,
			wl_client * client,
			weston_surface * surface,
			uint32_t id);
	virtual ~xdg_surface_toplevel_t();

	/* read only attributes */
	auto resource() const -> wl_resource *;
	auto title() const -> string const &;

	void set_title(char const *);
	void set_transient_for(xdg_surface_toplevel_t * s);
	auto transient_for() const -> xdg_surface_toplevel_t *;

	auto create_view() -> xdg_surface_toplevel_view_p;
	auto master_view() -> xdg_surface_toplevel_view_w;

	/**
	 * xdg-surface-interface (event)
	 **/
	void send_close();

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

};


}


#endif /* MANAGED_WINDOW_HXX_ */
