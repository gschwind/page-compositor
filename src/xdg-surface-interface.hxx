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

#ifndef SRC_XDG_SURFACE_INTERFACE_HXX_
#define SRC_XDG_SURFACE_INTERFACE_HXX_

#include <libweston-0/compositor.h>

namespace wcxx {

struct xdg_surface_vtable {

	virtual ~xdg_surface_vtable() = default;

	virtual void xdg_surface_destroy(wl_client * client,
			wl_resource * resource) = 0;
	virtual void xdg_surface_set_parent(wl_client * client,
			wl_resource * resource, wl_resource * parent_resource) = 0;
	virtual void xdg_surface_set_app_id(wl_client * client,
			wl_resource * resource, const char * app_id) = 0;
	virtual void xdg_surface_show_window_menu(wl_client * client,
			wl_resource * surface_resource, wl_resource * seat_resource,
			uint32_t serial, int32_t x, int32_t y) = 0;
	virtual void xdg_surface_set_title(wl_client * client,
			wl_resource * resource, const char * title) = 0;
	virtual void xdg_surface_move(wl_client * client, wl_resource * resource,
			wl_resource* seat_resource, uint32_t serial) = 0;
	virtual void xdg_surface_resize(wl_client* client, wl_resource * resource,
			wl_resource * seat_resource, uint32_t serial, uint32_t edges) = 0;
	virtual void xdg_surface_ack_configure(wl_client * client,
			wl_resource * resource, uint32_t serial) = 0;
	virtual void xdg_surface_set_window_geometry(wl_client * client,
			wl_resource * resource, int32_t x, int32_t y, int32_t width,
			int32_t height) = 0;
	virtual void xdg_surface_set_maximized(wl_client * client,
			wl_resource * resource) = 0;
	virtual void xdg_surface_unset_maximized(wl_client* client,
			wl_resource* resource) = 0;
	virtual void xdg_surface_set_fullscreen(wl_client * client,
			wl_resource * resource, wl_resource * output_resource) = 0;
	virtual void xdg_surface_unset_fullscreen(wl_client * client,
			wl_resource * resource) = 0;
	virtual void xdg_surface_set_minimized(wl_client * client,
			wl_resource * resource) = 0;

	virtual void xdg_surface_delete_resource(struct wl_resource * resource) = 0;

	void set_implementation(struct wl_resource * resource);

};

}

#endif /* SRC_XDG_SURFACE_INTERFACE_HXX_ */
