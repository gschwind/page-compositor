/*
 *
 * copyright (2016) Benoit Gschwind
 *
 */


#include "xdg-shell-unstable-v6-server-protocol.h"

#include <compositor.h>

#include "utils.hxx"
#include "xdg-shell-v6-client.hxx"
#include "xdg-surface-v6.hxx"

namespace page {

using namespace std;

void xdg_shell_v6_client_t::destroy_surface(xdg_surface_v6_t * s) {
	xdg_surface_map.erase(s->_id);
}

void xdg_shell_v6_client_t::destroy_positionner(xdg_positioner_v6_t * p) {
	xdg_positioner_v6_map.erase(p->_id);
}

xdg_shell_v6_client_t::xdg_shell_v6_client_t(
		page_context_t * ctx,
		wl_client * client,
		uint32_t id) :
		_ctx{ctx},
		self_resource{nullptr},
		client{client}
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
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
	if(not xdg_positioner_v6_map.empty() or not xdg_surface_map.empty()) {
		wl_resource_post_error(self_resource, ZXDG_SHELL_V6_ERROR_DEFUNCT_SURFACES, "TODO");
		return;
	}
	destroy.signal(this);
	wl_resource_destroy(self_resource);
}

void xdg_shell_v6_client_t::zxdg_shell_v6_create_positioner(struct wl_client * client, struct wl_resource * resource, uint32_t id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xdg_positioner = new xdg_positioner_v6_t(_ctx, client, id);
	xdg_positioner_v6_map[id] = xdg_positioner;
	connect(xdg_positioner->destroy, this, &xdg_shell_v6_client_t::destroy_positionner);

}

void xdg_shell_v6_client_t::zxdg_shell_v6_get_xdg_surface(struct wl_client * client, struct wl_resource * resource, uint32_t id, struct wl_resource * surface)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto s = resource_get<weston_surface>(surface);
	/* disable shared_ptr, they are managed by wl_resource */
	auto xdg_surface = new xdg_surface_v6_t(_ctx, client, s, id);
	xdg_surface_map[id] = xdg_surface;
	connect(xdg_surface->destroy, this, &xdg_shell_v6_client_t::destroy_surface);

}

void xdg_shell_v6_client_t::zxdg_shell_v6_pong(struct wl_client * client, struct wl_resource * resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_shell_v6_client_t::zxdg_shell_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	delete this;
}

}



