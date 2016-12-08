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
		uint32_t id) :
	self_resource{nullptr},
	_id{id},
	_client{client},
	_ctx{ctx},
	x_offset{0},
	y_offset{0}
{

	self_resource = wl_resource_create(client, &zxdg_positioner_v6_interface, 1, id);
	zxdg_positioner_v6_vtable::set_implementation(self_resource);

}

auto xdg_positioner_v6_t::get(struct wl_resource * r) -> xdg_positioner_v6_t * {
	return dynamic_cast<xdg_positioner_v6_t *>(resource_get<zxdg_positioner_v6_vtable>(r));
}

void xdg_positioner_v6_t::zxdg_positioner_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	destroy.signal(this);
	wl_resource_destroy(self_resource);
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_positioner_v6_t::zxdg_positioner_v6_set_anchor_rect(struct wl_client * client, struct wl_resource * resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	x_offset = x;
	y_offset = y;
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
	x_offset = x;
	y_offset = y;
}

void xdg_positioner_v6_t::zxdg_positioner_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	delete this;
}

}

