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

#include "wl-shell-client.hxx"
#include "xdg-surface-toplevel.hxx"

namespace page {

static void _wl_shell_get_shell_surface(struct wl_client *client,
			  struct wl_resource *resource,
			  uint32_t id,
			  struct wl_resource *surface) {

	wl_shell_client_t::get(resource)->wl_shell_get_shell_surface(client, resource, id, surface);

}

static struct wl_shell_interface const _wl_shell_implementation = {
		page::_wl_shell_get_shell_surface
};

wl_shell_client_t * wl_shell_client_t::get(wl_resource * resource) {
	return reinterpret_cast<wl_shell_client_t *>(wl_resource_get_user_data(resource));
}

wl_shell_client_t::wl_shell_client_t(page_context_t * ctx, wl_client * client, uint32_t id) :
		_ctx{ctx},
		_client{client},
		_id{id}
{

	/* allocate a wayland resource for the provided 'id' */
	_wl_shell_resource = wl_resource_create(client,
			reinterpret_cast<wl_interface const *>(&wl_shell_interface), 1, id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	wl_resource_set_implementation(_wl_shell_resource,
			&_wl_shell_implementation, this, &wl_shell_delete);

}

void wl_shell_client_t::wl_shell_delete(struct wl_resource *resource) {

}

wl_shell_client_t::~wl_shell_client_t() { }

void wl_shell_client_t::wl_shell_get_shell_surface(struct wl_client *client,
				  struct wl_resource *resource,
				  uint32_t id,
				  struct wl_resource *surface_resource) {

	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto surface = resource_get<weston_surface>(surface_resource);

	/* disable shared_ptr, they are managed by wl_resource */
	auto xdg_surface = new xdg_surface_toplevel_t(_ctx, client, surface, id);
	xdg_surface->set_wl_shell_surface_implementation();

	/* TODO */
	//connect(xdg_surface->destroy, this, &xdg_shell_client_t::destroy_toplevel);

}

}
