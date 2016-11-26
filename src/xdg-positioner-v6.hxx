/*
 * xdg-toplevel-v6.hxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_POSITIONER_V6_HXX_
#define SRC_XDG_POSITIONER_V6_HXX_

#include "xdg-shell-unstable-v6-interface.hxx"
#include "xdg-surface-v6.hxx"

namespace page {

using namespace std;
using namespace wcxx;

struct xdg_positioner_v6_t : public zxdg_positioner_v6_vtable {

	page_context_t *       _ctx;
	wl_client *            _client;
	xdg_surface_v6_t *     _surface;
	uint32_t               _id;
	struct wl_resource *   self_resource;

	xdg_positioner_v6_t(xdg_positioner_v6_t const &) = delete;
	xdg_positioner_v6_t & operator=(xdg_positioner_v6_t const &) = delete;

	xdg_positioner_v6_t(
			page_context_t * ctx,
			wl_client * client,
			xdg_surface_v6_t * surface,
			uint32_t id);

	virtual ~xdg_positioner_v6_t() = default;

	virtual void zxdg_positioner_v6_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_positioner_v6_set_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height) override;
	virtual void zxdg_positioner_v6_set_anchor_rect(struct wl_client * client, struct wl_resource * resource, int32_t x, int32_t y, int32_t width, int32_t height) override;
	virtual void zxdg_positioner_v6_set_anchor(struct wl_client * client, struct wl_resource * resource, uint32_t anchor) override;
	virtual void zxdg_positioner_v6_set_gravity(struct wl_client * client, struct wl_resource * resource, uint32_t gravity) override;
	virtual void zxdg_positioner_v6_set_constraint_adjustment(struct wl_client * client, struct wl_resource * resource, uint32_t constraint_adjustment) override;
	virtual void zxdg_positioner_v6_set_offset(struct wl_client * client, struct wl_resource * resource, int32_t x, int32_t y) override;
	virtual void zxdg_positioner_v6_delete_resource(struct wl_resource * resource) override;

};

}

#endif /* SRC_XDG_POSITIONER_V6_HXX_ */
