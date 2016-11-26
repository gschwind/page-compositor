/*
 * xdg-toplevel-v6.hxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_TOPLEVEL_V6_HXX_
#define SRC_XDG_TOPLEVEL_V6_HXX_

#include "xdg-shell-unstable-v6-interface.hxx"
#include "xdg-surface-v6.hxx"
#include "view-toplevel.hxx"

namespace page {

using namespace std;
using namespace wcxx;

struct xdg_toplevel_v6_t : public zxdg_toplevel_v6_vtable, public page_surface_interface {

	page_context_t *       _ctx;
	wl_client *            _client;
	xdg_surface_v6_t *     _surface;
	uint32_t               _id;
	struct wl_resource *   self_resource;

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

	view_toplevel_w _master_view;

	signal_t<xdg_toplevel_v6_t *> destroy;

	xdg_toplevel_v6_t(xdg_toplevel_v6_t const &) = delete;
	xdg_toplevel_v6_t & operator=(xdg_toplevel_v6_t const &) = delete;

	xdg_toplevel_v6_t(
			page_context_t * ctx,
			wl_client * client,
			xdg_surface_v6_t * surface,
			uint32_t id);

	void surface_commited(weston_surface * es, int32_t sx, int32_t sy);
	auto create_view() -> view_toplevel_p;

	virtual ~xdg_toplevel_v6_t() = default;

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

};

}

#else

namespace page {
struct xdg_toplevel_v6_t;
}

#endif /* SRC_XDG_TOPLEVEL_V6_HXX_ */
