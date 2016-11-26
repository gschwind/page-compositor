/*
 * xdg-toplevel-v6.hxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_POPUP_V6_HXX_
#define SRC_XDG_POPUP_V6_HXX_

#include "xdg-shell-unstable-v6-interface.hxx"
#include "xdg-surface-v6.hxx"

namespace page {

using namespace std;
using namespace wcxx;

struct xdg_popup_v6_t : public zxdg_popup_v6_vtable {

	page_context_t *       _ctx;
	wl_client *            _client;
	xdg_surface_v6_t *     _surface;
	uint32_t               _id;
	struct wl_resource *   self_resource;

	xdg_popup_v6_t(xdg_popup_v6_t const &) = delete;
	xdg_popup_v6_t & operator=(xdg_popup_v6_t const &) = delete;

	xdg_popup_v6_t(
			page_context_t * ctx,
			wl_client * client,
			xdg_surface_v6_t * surface,
			uint32_t id);

	virtual ~xdg_popup_v6_t() = default;

	virtual void zxdg_popup_v6_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_popup_v6_grab(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial) override;
	virtual void zxdg_popup_v6_delete_resource(struct wl_resource * resource) override;


};

}

#else

namespace page {
struct xdg_popup_v6_t;
}

#endif /* SRC_XDG_POPUP_V6_HXX_ */
