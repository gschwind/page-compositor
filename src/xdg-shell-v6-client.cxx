/*
 *
 * copyright (2016) Benoit Gschwind
 *
 */


#include "xdg-shell-v6-client.hxx"

#include "xdg-shell-unstable-v6-server-protocol.h"

namespace page {

using namespace std;

xdg_shell_v6_client_t::xdg_shell_v6_client_t(
		page_context_t * ctx,
		wl_client * client,
		uint32_t id) :
		_ctx{ctx},
		self_resource{nullptr},
		client{client}
{

	/* allocate a wayland resource for the provided 'id' */
	self_resource = wl_resource_create(client,
			reinterpret_cast<wl_interface const *>(&zxdg_shell_v6_interface), 1, id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	zxdg_shell_v6_vtable::set_implementation(self_resource);

}

void xdg_shell_v6_client_t::zxdg_shell_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_shell_v6_client_t::zxdg_shell_v6_create_positioner(struct wl_client * client, struct wl_resource * resource, uint32_t id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_shell_v6_client_t::zxdg_shell_v6_get_xdg_surface(struct wl_client * client, struct wl_resource * resource, uint32_t id, struct wl_resource * surface)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_shell_v6_client_t::zxdg_shell_v6_pong(struct wl_client * client, struct wl_resource * resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_shell_v6_client_t::zxdg_shell_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_shell_v6_client_t::remove_all_transient(xdg_surface_toplevel_t * s) {

}

void xdg_shell_v6_client_t::remove_all_popup(xdg_surface_popup_t * s) {

}

void xdg_shell_v6_client_t::destroy_toplevel(xdg_surface_toplevel_t * s) {
	disconnect(s->destroy);
	xdg_surface_toplevel_map.erase(s->_id);
}

void xdg_shell_v6_client_t::destroy_popup(xdg_surface_popup_t * s) {
	disconnect(s->destroy);
	xdg_surface_popup_map.erase(s->_id);
}

}



