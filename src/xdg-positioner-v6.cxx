/*
 * xdg-toplevel-v6.cxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */


#include "xdg-positioner-v6.hxx"
#include "xdg-shell-unstable-v6-server-protocol.h"

namespace page {

xdg_positioner_v6_t::xdg_positioner_v6_t(
		page_context_t * ctx,
		wl_client * client,
		xdg_surface_v6_t * surface,
		uint32_t id) :
	self_resource{nullptr},
	_surface{surface},
	_id{id},
	_client{client},
	_ctx{ctx}
{

	self_resource = wl_resource_create(client,
			reinterpret_cast<wl_interface const *>(&zxdg_positioner_v6_interface), 1, id);
	zxdg_positioner_v6_vtable::set_implementation(self_resource);

}

void xdg_positioner_v6_t::zxdg_positioner_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_anchor_rect(struct wl_client * client, struct wl_resource * resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_anchor(struct wl_client * client, struct wl_resource * resource, uint32_t anchor)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_gravity(struct wl_client * client, struct wl_resource * resource, uint32_t gravity)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_constraint_adjustment(struct wl_client * client, struct wl_resource * resource, uint32_t constraint_adjustment)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_offset(struct wl_client * client, struct wl_resource * resource, int32_t x, int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

}

