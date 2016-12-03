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
	_id{id},
	_ack_config{0xffffffff}
{
	
	/* allocate a wayland resource for the provided 'id' */
	_resource = wl_resource_create(client, &zxdg_surface_v6_interface, 1, id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	zxdg_surface_v6_vtable::set_implementation(_resource);
	
	on_surface_destroy.connect(&_surface->destroy_signal, this,
			&xdg_surface_v6_t::surface_destroyed);
	on_surface_commit.connect(&_surface->commit_signal, this,
			&xdg_surface_v6_t::surface_commited);

}

void xdg_surface_v6_t::surface_commited(weston_surface * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	for(auto & x: xdg_toplevel_v6_map) {
		x.second->surface_commited(s);
	}
	for(auto & x: xdg_popup_v6_map) {
		x.second->surface_commited(s);
	}
}

void xdg_surface_v6_t::surface_destroyed(weston_surface * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	destroy_all_views();
	if(_surface) {
		on_surface_destroy.disconnect();
		on_surface_commit.disconnect();
	}
}

auto xdg_surface_v6_t::get(struct wl_resource * r) -> xdg_surface_v6_t * {
	return dynamic_cast<xdg_surface_v6_t*>(resource_get<zxdg_surface_v6_vtable>(r));
}


void xdg_surface_v6_t::destroy_all_views() {
	for(auto &x: xdg_toplevel_v6_map) {
		x.second->destroy_all_views();
	}

	for(auto &x: xdg_popup_v6_map) {
		x.second->destroy_all_views();
	}
}

xdg_surface_v6_t::~xdg_surface_v6_t() {
	if(_surface) {
		on_surface_destroy.disconnect();
		on_surface_commit.disconnect();
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

	auto xdg_surface = new xdg_popup_v6_t(_ctx, client, this, id, parent, positioner);
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
	if(_ack_config == serial)
		_ack_config = 0;
}

void xdg_surface_v6_t::zxdg_surface_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

}


