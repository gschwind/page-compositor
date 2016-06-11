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

#include "display-compositor.hxx"

namespace page {

using namespace std;

display_compositor_t * dc{nullptr};

static void xdg_shell_destroy(wl_client * client, wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_shell_destroy(client, resource);
}

static void xdg_shell_use_unstable_version(wl_client * client,
		wl_resource * resource, int32_t version)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_shell_use_unstable_version(client, resource, version);
}

static void xdg_shell_get_xdg_surface(wl_client * client,
		wl_resource * resource, uint32_t id, wl_resource* surface_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_shell_get_xdg_surface(client, resource, id, surface_resource);
}

static void xdg_shell_get_xdg_popup(wl_client * client, wl_resource * resource,
		uint32_t id, wl_resource * surface_resource,
		wl_resource * parent_resource, wl_resource * seat_resource,
		uint32_t serial, int32_t x, int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_shell_get_xdg_popup(client, resource, id, surface_resource,
			parent_resource, seat_resource, serial, x, y);
}

static void xdg_shell_pong(wl_client * client, wl_resource * resource,
		uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_shell_pong(client, resource, serial);
}

struct xdg_shell_interface display_compositor_t::xdg_shell_implementation = {
	page::xdg_shell_destroy, page::xdg_shell_use_unstable_version,
	page::xdg_shell_get_xdg_surface, page::xdg_shell_get_xdg_popup,
	page::xdg_shell_pong
};

static void xdg_surface_destroy(wl_client * client, wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_destroy(client, resource);
}

static void xdg_surface_set_parent(wl_client * client, wl_resource * resource,
		wl_resource * parent_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_set_parent(client, resource, parent_resource);
}

static void xdg_surface_set_app_id(wl_client * client, wl_resource * resource,
		const char* app_id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_set_app_id(client, resource, app_id);
}

static void xdg_surface_show_window_menu(wl_client * client,
		wl_resource * surface_resource, wl_resource * seat_resource,
		uint32_t serial, int32_t x, int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_show_window_menu(client, surface_resource, seat_resource,
			serial, x, y);
}

static void xdg_surface_set_title(wl_client * client, wl_resource * resource,
		char const * title)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_set_title(client, resource, title);
}

static void xdg_surface_move(wl_client * client, wl_resource * resource,
		wl_resource * seat_resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_move(client, resource, seat_resource, serial);
}

static void xdg_surface_resize(wl_client * client, wl_resource * resource,
		wl_resource * seat_resource, uint32_t serial, uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_resize(client, resource, seat_resource, serial, edges);
}

static void xdg_surface_ack_configure(wl_client * client,
		wl_resource * resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_ack_configure(client, resource, serial);
}

static void xdg_surface_set_window_geometry(wl_client * client,
		wl_resource * resource, int32_t x, int32_t y, int32_t width,
		int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_set_window_geometry(client, resource, x, y, width, height);
}

static void xdg_surface_set_maximized(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_set_maximized(client, resource);
}

static void xdg_surface_unset_maximized(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_unset_maximized(client, resource);
}

static void xdg_surface_set_fullscreen(wl_client * client,
		wl_resource * resource, wl_resource * output_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_set_fullscreen(client, resource, output_resource);
}

static void xdg_surface_unset_fullscreen(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_unset_fullscreen(client, resource);
}

static void xdg_surface_set_minimized(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_surface_set_minimized(client, resource);
}

struct xdg_surface_interface display_compositor_t::xdg_surface_implementation = {
	page::xdg_surface_destroy, page::xdg_surface_set_parent,
	page::xdg_surface_set_title, page::xdg_surface_set_app_id,
	page::xdg_surface_show_window_menu, page::xdg_surface_move,
	page::xdg_surface_resize, page::xdg_surface_ack_configure,
	page::xdg_surface_set_window_geometry,
	page::xdg_surface_set_maximized,
	page::xdg_surface_unset_maximized,
	page::xdg_surface_set_fullscreen,
	page::xdg_surface_unset_fullscreen,
	page::xdg_surface_set_minimized
};

static void xdg_popup_destroy(wl_client * client, wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	dc->xdg_popup_destroy(client, resource);
}

struct xdg_popup_interface display_compositor_t::xdg_popup_implementation = {
	page::xdg_popup_destroy
};

}
