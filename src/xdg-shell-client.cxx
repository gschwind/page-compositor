/*
 * client.cxx
 *
 *  Created on: 10 juin 2016
 *      Author: gschwind
 */

#include "xdg-shell-client.hxx"

#include "xdg-shell-unstable-v5-server-protocol.h"

namespace page {

using namespace std;

void xdg_shell_client_t::xdg_shell_destroy(wl_client * client,
		wl_resource * resource)
{
	if(not xdg_surface_toplevel_map.empty() or not xdg_surface_popup_map.empty()) {
		wl_resource_post_error(xdg_shell_resource, XDG_SHELL_ERROR_DEFUNCT_SURFACES, "TODO");
		return;
	}

	destroy.signal(this);
	wl_resource_destroy(xdg_shell_resource);
}

void xdg_shell_client_t::xdg_shell_use_unstable_version(wl_client * client,
		wl_resource * resource, int32_t version)
{
	/* TODO */
}

void xdg_shell_client_t::xdg_shell_get_xdg_surface(wl_client * client,
		wl_resource * resource, uint32_t id, wl_resource * surface_resource)
{

	auto surface = resource_get<weston_surface>(surface_resource);

	/* disable shared_ptr, they are managed by wl_resource */
	auto xdg_surface = new xdg_surface_toplevel_t(_ctx, client, surface, id);

	surfaces_map[surface] = xdg_surface;
	xdg_surface_toplevel_map[id] = xdg_surface;
	connect(xdg_surface->destroy, this, &xdg_shell_client_t::destroy_toplevel);

	weston_log("exit %s\n", __PRETTY_FUNCTION__);

}

void xdg_shell_client_t::xdg_shell_get_xdg_popup(wl_client * client,
		  wl_resource * resource,
		  uint32_t id,
		  wl_resource * surface_resource,
		  wl_resource * parent_resource,
		  wl_resource * seat_resource,
		  uint32_t serial,
		  int32_t x, int32_t y) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* In our case nullptr */
	auto surface = resource_get<weston_surface>(surface_resource);
	auto parent = resource_get<weston_surface>(parent_resource);
	auto seat = resource_get<weston_seat>(parent_resource);

	weston_log("p=%p, x=%d, y=%d\n", surface, x, y);

	/* disable shared_ptr for now, the resource is managed by wl_resource */
	auto xdg_popup = new xdg_surface_popup_t(_ctx, client, resource,
			id, surface, surfaces_map[parent], seat, serial, x, y);

	surfaces_map[surface] = xdg_popup;
	xdg_surface_popup_map[id] = xdg_popup;
	connect(xdg_popup->destroy, this, &xdg_shell_client_t::destroy_popup);

}

void xdg_shell_client_t::xdg_shell_pong(struct wl_client *client,
	 struct wl_resource *resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

auto xdg_shell_client_t::get(wl_resource * resource) -> xdg_shell_client_t * {
	return dynamic_cast<xdg_shell_client_t *>(resource_get<xdg_shell_vtable>(resource));
}

void xdg_shell_client_t::xdg_shell_delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	delete this;
}

xdg_shell_client_t::xdg_shell_client_t(
		page_context_t * ctx,
		wl_client * client,
		uint32_t id) :
		_ctx{ctx},
		xdg_shell_resource{nullptr},
		client{client}
{

	/* allocate a wayland resource for the provided 'id' */
	xdg_shell_resource = wl_resource_create(client, &xdg_shell_interface, 1, id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	xdg_shell_vtable::set_implementation(xdg_shell_resource);

}

xdg_surface_base_t * xdg_shell_client_t::find_xdg_surface_from_weston_surface(struct weston_surface * s) {
	return surfaces_map[s];
}

void xdg_shell_client_t::destroy_toplevel(xdg_surface_toplevel_t * s) {
	disconnect(s->destroy);
	xdg_surface_toplevel_map.erase(s->_id);
	surfaces_map.erase(s->surface());
}

void xdg_shell_client_t::destroy_popup(xdg_surface_popup_t * s) {
	disconnect(s->destroy);
	xdg_surface_popup_map.erase(s->_id);
	surfaces_map.erase(s->surface());
}

}



