/*
 * xdg-toplevel-v6.hxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_SHELL_V6_TOPLEVEL_HXX_
#define SRC_XDG_SHELL_V6_TOPLEVEL_HXX_

#include "xdg-shell-unstable-v6-interface.hxx"
#include "view.hxx"
#include "xdg-shell-v6-surface.hxx"

namespace page {

using namespace std;
using namespace wcxx;

struct xdg_toplevel_v6_t : public connectable_t, public zxdg_toplevel_v6_vtable, public surface_t {
	xdg_surface_v6_t *     _base;

	page_context_t *       _ctx;
	wl_client *            _client;
	uint32_t               _id;
	struct wl_resource *   self_resource;

	struct _state {
		std::string title;
		bool fullscreen;
		bool maximized;
		bool minimized;
		surface_t * transient_for;
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

	static map<uint32_t, edge_e> const _edge_map;

	listener_t<struct wl_resource> on_resource_destroyed;

	signal_t<xdg_toplevel_v6_t *> destroy;

	xdg_toplevel_v6_t(xdg_toplevel_v6_t const &) = delete;
	xdg_toplevel_v6_t & operator=(xdg_toplevel_v6_t const &) = delete;

	xdg_toplevel_v6_t(
			page_context_t * ctx,
			wl_client * client,
			xdg_surface_v6_t * surface,
			uint32_t id);

	void surface_destroyed(xdg_surface_v6_t * s);
	void surface_first_commited(xdg_surface_v6_t * s);
	void surface_commited(xdg_surface_v6_t * s);

	edge_e edge_map(uint32_t edge);

	static auto get(struct wl_resource * r) -> xdg_toplevel_v6_t *;

	virtual ~xdg_toplevel_v6_t();

	/* zxdg_toplevel_v6_vtable */
	virtual void zxdg_toplevel_v6_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_toplevel_v6_set_parent(struct wl_client * client, struct wl_resource * resource, struct wl_resource * parent) override;
	virtual void zxdg_toplevel_v6_set_title(struct wl_client * client, struct wl_resource * resource, const char * title) override;
	virtual void zxdg_toplevel_v6_set_app_id(struct wl_client * client, struct wl_resource * resource, const char * app_id) override;
	virtual void zxdg_toplevel_v6_show_window_menu(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, int32_t x, int32_t y) override;
	virtual void zxdg_toplevel_v6_move(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial) override;
	virtual void zxdg_toplevel_v6_resize(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, uint32_t edges) override;
	virtual void zxdg_toplevel_v6_set_max_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height) override;
	virtual void zxdg_toplevel_v6_set_min_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height) override;
	virtual void zxdg_toplevel_v6_set_maximized(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_toplevel_v6_unset_maximized(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_toplevel_v6_set_fullscreen(struct wl_client * client, struct wl_resource * resource, struct wl_resource * output) override;
	virtual void zxdg_toplevel_v6_unset_fullscreen(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_toplevel_v6_set_minimized(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_toplevel_v6_delete_resource(struct wl_resource * resource) override;

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

#else

namespace page {
struct xdg_toplevel_v6_t;
}

#endif /* SRC_XDG_SHELL_V6_TOPLEVEL_HXX_ */
