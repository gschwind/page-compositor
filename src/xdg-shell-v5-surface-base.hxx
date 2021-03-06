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

#ifndef XDG_SURFACE_BASE_HXX_
#define XDG_SURFACE_BASE_HXX_

#include <cassert>
#include <memory>

#include "tree-types.hxx"
#include "utils.hxx"
#include "listener.hxx"
#include "region.hxx"

#include "exception.hxx"

#include "tree.hxx"
#include "page_context.hxx"
#include "surface.hxx"

namespace page {

using namespace std;

/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
struct xdg_surface_base_t {

	page_context_t *       _ctx;
	wl_client *            _client;
	weston_surface *       _surface;
	uint32_t               _id;
	struct wl_resource *   _resource;
	wl_listener            _surface_destroy;

	listener_t<struct weston_surface> on_surface_destroy;
	listener_t<struct weston_surface> on_surface_commit;

	xdg_surface_base_t(xdg_surface_base_t const &) = delete;
	xdg_surface_base_t & operator=(xdg_surface_base_t const &) = delete;

	xdg_surface_base_t(
			page_context_t * ctx,
			wl_client * client,
			weston_surface * surface,
			uint32_t id);

	virtual ~xdg_surface_base_t();

	virtual surface_t * page_surface() = 0;

};

}

#endif /* XDG_SURFACE_BASE_HXX_ */
