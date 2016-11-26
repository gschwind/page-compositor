/*
 * xdg-toplevel-v6.cxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */


#include "xdg-toplevel-v6.hxx"
#include "xdg-shell-unstable-v6-server-protocol.h"

namespace page {

xdg_toplevel_v6_t::xdg_toplevel_v6_t(
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
			reinterpret_cast<wl_interface const *>(&zxdg_toplevel_v6_interface), 1, id);
	zxdg_toplevel_v6_vtable::set_implementation(self_resource);

}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_parent(struct wl_client * client, struct wl_resource * resource, struct wl_resource * parent)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_title(struct wl_client * client, struct wl_resource * resource, const char * title)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_app_id(struct wl_client * client, struct wl_resource * resource, const char * app_id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_show_window_menu(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, int32_t x, int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_move(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_resize(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_max_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_min_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_maximized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_unset_maximized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_fullscreen(struct wl_client * client, struct wl_resource * resource, struct wl_resource * output)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_unset_fullscreen(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_minimized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

}

