/*

 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_NOT_MANAGED_HXX_
#define CLIENT_NOT_MANAGED_HXX_

#include <memory>
#include <libweston-0/compositor.h>
#include "xdg-shell-server-protocol.h"

#include "renderable.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"
#include "renderable_pixmap.hxx"
#include "xdg-surface-base.hxx"

namespace page {

using namespace std;

struct xdg_surface_popup_t : public xdg_surface_base_t {

	wl_client * client;
	wl_resource * resource;
	uint32_t id;
	weston_surface * surface;
	weston_surface * parent;
	weston_seat * seat;
	uint32_t serial;
	int32_t x;
	int32_t y;

	list<xdg_surface_popup_t *> _popup_childdren;

	xdg_surface_popup_view_w _master_view;

	auto create_view() -> xdg_surface_popup_view_p;
	auto master_view() -> xdg_surface_popup_view_w;

	/* avoid copy */
	xdg_surface_popup_t(xdg_surface_popup_t const &) = delete;
	xdg_surface_popup_t & operator=(xdg_surface_popup_t const &) = delete;

	static void _weston_configure(weston_surface * es, int32_t sx, int32_t sy);

	void xdg_popup_destroy(wl_client * client, wl_resource * resource);

	/** called on surface commit */
	void weston_configure(weston_surface * es, int32_t sx, int32_t sy);

	xdg_surface_popup_t(page_context_t * ctx,
			  xdg_shell_client_t * xdg_shell_client,
			  wl_client * client,
			  wl_resource * resource,
			  uint32_t id,
			  weston_surface * surface,
			  weston_surface * parent,
			  weston_seat * seat,
			  uint32_t serial,
			  int32_t x, int32_t y);
	~xdg_surface_popup_t();

	static void xdg_popup_delete(struct wl_resource *resource);
	static auto get(wl_resource * r) -> xdg_surface_popup_t *;

};

}

#endif /* CLIENT_NOT_MANAGED_HXX_ */
