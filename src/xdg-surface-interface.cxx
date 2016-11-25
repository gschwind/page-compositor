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

#include "xdg-surface-interface.hxx"
#include "xdg-shell-unstable-v5-server-protocol.h"

namespace wcxx {

namespace hidden {

inline xdg_surface_vtable * get(struct wl_resource * resource) {
	return reinterpret_cast<xdg_surface_vtable *>(
			wl_resource_get_user_data(resource));
}

static void xdg_surface_destroy(wl_client * client, wl_resource * resource)
{
	get(resource)->xdg_surface_destroy(client, resource);
}

static void xdg_surface_set_parent(wl_client * client, wl_resource * resource,
		wl_resource * parent_resource)
{
	get(resource)->xdg_surface_set_parent(client, resource, parent_resource);
}

static void xdg_surface_set_app_id(wl_client * client, wl_resource * resource,
		const char* app_id)
{
	get(resource)->xdg_surface_set_app_id(client, resource, app_id);
}

static void xdg_surface_show_window_menu(wl_client * client,
		wl_resource * surface_resource, wl_resource * seat_resource,
		uint32_t serial, int32_t x, int32_t y)
{
	get(surface_resource)->xdg_surface_show_window_menu(client, surface_resource, seat_resource,
			serial, x, y);
}

static void xdg_surface_set_title(wl_client * client, wl_resource * resource,
		char const * title)
{
	get(resource)->xdg_surface_set_title(client, resource, title);
}

static void xdg_surface_move(wl_client * client, wl_resource * resource,
		wl_resource * seat_resource, uint32_t serial)
{
	get(resource)->xdg_surface_move(client, resource, seat_resource, serial);
}

static void xdg_surface_resize(wl_client * client, wl_resource * resource,
		wl_resource * seat_resource, uint32_t serial, uint32_t edges)
{
	get(resource)->xdg_surface_resize(client, resource, seat_resource, serial, edges);
}

static void xdg_surface_ack_configure(wl_client * client,
		wl_resource * resource, uint32_t serial)
{
	get(resource)->xdg_surface_ack_configure(client, resource, serial);
}

static void xdg_surface_set_window_geometry(wl_client * client,
		wl_resource * resource, int32_t x, int32_t y, int32_t width,
		int32_t height)
{
	get(resource)->xdg_surface_set_window_geometry(client, resource, x, y, width, height);
}

static void xdg_surface_set_maximized(wl_client * client,
		wl_resource * resource)
{
	get(resource)->xdg_surface_set_maximized(client, resource);
}

static void xdg_surface_unset_maximized(wl_client * client,
		wl_resource * resource)
{
	get(resource)->xdg_surface_unset_maximized(client, resource);
}

static void xdg_surface_set_fullscreen(wl_client * client,
		wl_resource * resource, wl_resource * output_resource)
{
	get(resource)->xdg_surface_set_fullscreen(client, resource, output_resource);
}

static void xdg_surface_unset_fullscreen(wl_client * client,
		wl_resource * resource)
{
	get(resource)->xdg_surface_unset_fullscreen(client, resource);
}

static void xdg_surface_set_minimized(wl_client * client,
		wl_resource * resource)
{
	get(resource)->xdg_surface_set_minimized(client, resource);
}

static void xdg_surface_delete_resource(struct wl_resource * resource) {
	get(resource)->xdg_surface_delete_resource(resource);
}

static struct xdg_surface_interface const xdg_surface_implementation = {
	xdg_surface_destroy,
	xdg_surface_set_parent,
	xdg_surface_set_title,
	xdg_surface_set_app_id,
	xdg_surface_show_window_menu,
	xdg_surface_move,
	xdg_surface_resize,
	xdg_surface_ack_configure,
	xdg_surface_set_window_geometry,
	xdg_surface_set_maximized,
	xdg_surface_unset_maximized,
	xdg_surface_set_fullscreen,
	xdg_surface_unset_fullscreen,
	xdg_surface_set_minimized
};

}

void xdg_surface_vtable::set_implementation(
		struct wl_resource * resource) {
	wl_resource_set_implementation(resource,
			&hidden::xdg_surface_implementation,
			this, &hidden::xdg_surface_delete_resource);
}

}
