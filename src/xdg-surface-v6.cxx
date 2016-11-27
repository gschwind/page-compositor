/*
 * client_base.cxx
 *
 *  Created on: 5 ao������t 2015
 *      Author: gschwind
 */

#include "xdg-surface-v6.hxx"

#include "xdg-shell-unstable-v6-server-protocol.h"

namespace page {

using namespace std;

xdg_surface_v6_t::xdg_surface_v6_t(
		page_context_t * ctx,
		wl_client * client,
		weston_surface * surface,
		uint32_t id) :
	_ctx{ctx},
	_resource{nullptr},
	_client{client},
	_surface{surface},
	_id{id}
{
	
	/* allocate a wayland resource for the provided 'id' */
	_resource = wl_resource_create(client,
			reinterpret_cast<wl_interface const *>(&zxdg_shell_v6_interface), 1, id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	zxdg_surface_v6_vtable::set_implementation(_resource);
	
	/**
	 * weston_surface are used for wl_surface, but those surfaces can have
	 * several role, configure_private may hold xdg_surface_toplevel or
	 * xdg_surface_popup_t. To avoid mistake configure_private always store
	 * xdg_surface_base, allowing dynamic_cast.
	 **/
	_surface->committed_private = this;
	_surface->committed = &xdg_surface_v6_t::surface_commited_callback;

	_surface_destroy.notify = [] (wl_listener *l, void *data) {
		auto surface = reinterpret_cast<weston_surface*>(data);
		auto ths = reinterpret_cast<xdg_surface_v6_t*>(surface->committed_private);
		ths->surface_destroyed();
	};

	wl_signal_add(&surface->destroy_signal, &_surface_destroy);

}

void xdg_surface_v6_t::surface_commited_callback(weston_surface * es, int32_t sx, int32_t sy) {
	reinterpret_cast<xdg_surface_v6_t *>(es->committed_private)->surface_commited(es, sx, sy);
}

void xdg_surface_v6_t::surface_commited(weston_surface * es, int32_t sx, int32_t sy) {
	for(auto & x: xdg_toplevel_v6_map) {
		x.second->surface_commited(es, sx, sy);
	}
}

void xdg_surface_v6_t::surface_destroyed() {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

xdg_surface_v6_t::~xdg_surface_v6_t() {

	if(_surface) {
		wl_list_remove(&_surface_destroy.link);
		_surface->committed_private = nullptr;
		_surface->committed = nullptr;
		_surface = nullptr;
	}

}

void xdg_surface_v6_t::zxdg_surface_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_surface_v6_t::zxdg_surface_v6_get_toplevel(struct wl_client * client, struct wl_resource * resource, uint32_t id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto xdg_surface = new xdg_toplevel_v6_t(_ctx, client, this, id);
	xdg_toplevel_v6_map[id] = xdg_surface;

}

void xdg_surface_v6_t::zxdg_surface_v6_get_popup(struct wl_client * client, struct wl_resource * resource, uint32_t id, struct wl_resource * parent, struct wl_resource * positioner)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto xdg_surface = new xdg_popup_v6_t(_ctx, client, this, id);
	xdg_popup_v6_map[id] = xdg_surface;

}

void xdg_surface_v6_t::zxdg_surface_v6_set_window_geometry(struct wl_client * client, struct wl_resource * resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_surface_v6_t::zxdg_surface_v6_ack_configure(struct wl_client * client, struct wl_resource * resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_surface_v6_t::zxdg_surface_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

}


