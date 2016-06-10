/*
 * Copyright (2016) Benoit Gschwind
 *
 * This file is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xdg-shell.hxx"
#include "xdg-surface.hxx"
#include "xdg-popup.hxx"

namespace page {

using namespace std;

xdg_shell_t * xdg_shell_t::get(wl_resource * resource) {
	return reinterpret_cast<xdg_shell_t*>(wl_resource_get_user_data(resource));
}


static void
xdg_shell_delete(struct wl_resource *resource) {
	delete xdg_shell_t::get(resource);
}


xdg_shell_t::xdg_shell_t(wl_client * client, uint32_t id) :
		resource{nullptr},
		client{client},
		id{id}
{


	/* allocate a wayland resource for the provided 'id' */
	auto resource = wl_resource_create(client, &xdg_shell_interface, 1, id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	wl_resource_set_implementation(resource, &xdg_shell_implementation,
			this, &xdg_shell_delete);


}

static void
xdg_shell_destroy(struct wl_client *client,
		  struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_use_unstable_version(struct wl_client *client,
			 struct wl_resource *resource,
			 int32_t version) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_get_xdg_surface(struct wl_client *client,
		    struct wl_resource *resource,
		    uint32_t id,
		    struct wl_resource *surface_resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	/* In our case nullptr */
	auto surface =
		reinterpret_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));
	auto shell = xdg_shell_t::get(resource);

	auto xdg_surface = new xdg_surface_t(client, shell, id, surface);
	shell->xdg_shell_surfaces.push_back(xdg_surface->resource);

	wl_array xxx;
	wl_array_init(&xxx);
	wl_array_add(&xxx, 4);
	((int*)xxx.data)[0] = XDG_SURFACE_STATE_ACTIVATED;
	xdg_surface_send_configure(xdg_surface->resource, 800, 800, &xxx, 0);

	weston_log("exit %s\n", __PRETTY_FUNCTION__);

}

static void
xdg_get_xdg_popup(struct wl_client *client,
		  struct wl_resource *resource,
		  uint32_t id,
		  struct wl_resource *surface_resource,
		  struct wl_resource *parent_resource,
		  struct wl_resource *seat_resource,
		  uint32_t serial,
		  int32_t x, int32_t y) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* In our case nullptr */
	auto surface =
		reinterpret_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));
	auto shell = xdg_shell_t::get(resource);

	weston_log("p=%p, x=%d, y=%d\n", surface, x, y);

	auto xdg_popup = new xdg_popup_t(client, id, surface, x, y);

}

static void
xdg_pong(struct wl_client *client,
	 struct wl_resource *resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}



const struct xdg_shell_interface xdg_shell_implementation = {
	xdg_shell_destroy,
	xdg_use_unstable_version,
	xdg_get_xdg_surface,
	xdg_get_xdg_popup,
	xdg_pong
};

}

