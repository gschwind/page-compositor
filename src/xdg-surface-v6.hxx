/*
 * client.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 * client_base store/cache all client windows properties define in ICCCM or EWMH.
 *
 * most of them are store with pointer which is null if the properties is not set on client.
 *
 */

#ifndef XDG_SURFACE_V6_HXX_
#define XDG_SURFACE_V6_HXX_

#include "xdg-shell-unstable-v6-interface.hxx"

#include <cassert>
#include <memory>
#include <map>

#include "listener.hxx"
#include "tree-types.hxx"

#include "utils.hxx"
#include "region.hxx"

#include "exception.hxx"

#include "tree.hxx"
#include "page_context.hxx"

#include "xdg-toplevel-v6.hxx"
#include "xdg-popup-v6.hxx"

namespace page {

using namespace std;
using namespace wcxx;

/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
struct xdg_surface_v6_t : public connectable_t, public zxdg_surface_v6_vtable {

	page_context_t *       _ctx;

	struct wl_client *            _client;
	struct weston_surface *       _surface;
	uint32_t               _id;
	struct wl_resource *   _resource;

	listener_t<weston_surface> on_surface_destroy;
	listener_t<weston_surface> on_surface_commit;

	uint32_t _ack_config;

	signal_t<xdg_surface_v6_t *> destroy;
	signal_t<xdg_surface_v6_t *> commited;

	surface_t * _role;

	void surface_commited(weston_surface * s);
	void surface_destroyed(weston_surface * s);

	auto create_view() -> view_p;

	void toplevel_destroyed(xdg_toplevel_v6_t * s);
	void popup_destroyed(xdg_popup_v6_t * s);

	static auto get(struct wl_resource * r) -> xdg_surface_v6_t *;

	xdg_surface_v6_t(xdg_surface_v6_t const &) = delete;
	xdg_surface_v6_t & operator=(xdg_surface_v6_t const &) = delete;

	xdg_surface_v6_t(
			page_context_t * ctx,
			wl_client * client,
			weston_surface * surface,
			uint32_t id);

	virtual ~xdg_surface_v6_t();

	/* zxdg_surface_v6_vtable */
	virtual void zxdg_surface_v6_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_surface_v6_get_toplevel(struct wl_client * client, struct wl_resource * resource, uint32_t id) override;
	virtual void zxdg_surface_v6_get_popup(struct wl_client * client, struct wl_resource * resource, uint32_t id, struct wl_resource * parent, struct wl_resource * positioner) override;
	virtual void zxdg_surface_v6_set_window_geometry(struct wl_client * client, struct wl_resource * resource, int32_t x, int32_t y, int32_t width, int32_t height) override;
	virtual void zxdg_surface_v6_ack_configure(struct wl_client * client, struct wl_resource * resource, uint32_t serial) override;
	virtual void zxdg_surface_v6_delete_resource(struct wl_resource * resource) override;

};

}

#else

namespace page {

struct xdg_surface_v6_t;

}

#endif /* XDG_SURFACE_V6_HXX_ */
