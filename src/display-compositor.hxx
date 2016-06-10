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

#ifndef SRC_DISPLAY_COMPOSITOR_HXX_
#define SRC_DISPLAY_COMPOSITOR_HXX_

#include <deque>

#include <libweston-0/compositor.h>
#include "xdg-shell-server-protocol.h"

#include "region.hxx"
#include "listener.hxx"

namespace page {

using namespace std;

/**
 * This class is a wrapper to wl_display + weston_compositor
 **/
class display_compositor_t {


protected:
	static struct xdg_shell_interface xdg_shell_implementation;
	static struct xdg_surface_interface xdg_surface_implementation;
	static struct xdg_popup_interface xdg_popup_implementation;

public:

	wl_display * _dpy;
	weston_compositor * ec;
	weston_layer default_layer;

	wl_listener destroy;

	/* surface signals */
	wl_listener create_surface;
	wl_listener activate;
	wl_listener transform;

	wl_listener kill;
	wl_listener idle;
	wl_listener wake;

	wl_listener show_input_panel;
	wl_listener hide_input_panel;
	wl_listener update_input_panel;

	wl_listener seat_created;
	listener_t<weston_output> output_created;
	wl_listener output_destroyed;
	wl_listener output_moved;

	wl_listener session;

	display_compositor_t();

	virtual ~display_compositor_t();

	void connect_all();

	void on_output_created(weston_output * output);

	void run();

	double get_fps();
	deque<double> const & get_direct_area_history();
	deque<double> const & get_damaged_area_history();

	bool show_damaged();
	bool show_opac();
	void set_show_damaged(bool b);
	void set_show_opac(bool b);
	void add_damaged(region const & r);

	uint32_t get_composite_overlay();

	void update_layout();


	/**
	 * To make the port essayer I chossed to flatten the waylant API. This
	 * trick convert wayland protocol to a queue event similar to the X11 queue.
	 **/

	/**
	 * the xdg-shell
	 **/
	virtual void xdg_shell_destroy(wl_client * client, wl_resource * resource);

	virtual void xdg_shell_use_unstable_version(wl_client * client,
			wl_resource * resource, int32_t version);

	virtual void xdg_shell_get_xdg_surface(wl_client * client,
			wl_resource * resource, uint32_t id, wl_resource * surface_resource);

	virtual void xdg_shell_get_xdg_popup(wl_client * client,
			wl_resource * resource, uint32_t id, wl_resource * surface,
			wl_resource * parent, wl_resource* seat_resource, uint32_t serial,
			int32_t x, int32_t y);

	virtual void xdg_shell_pong(wl_client * client, wl_resource * resource,
			uint32_t serial);

	/**
	 * the xdg-surface
	 **/

	virtual void xdg_surface_destroy(wl_client * client, wl_resource * resource);

	virtual void xdg_surface_set_parent(wl_client * client,
			wl_resource * resource, wl_resource * parent_resource);

	virtual void xdg_surface_set_app_id(wl_client * client,
			wl_resource * resource, const char * app_id);

	virtual void xdg_surface_show_window_menu(wl_client * client,
			wl_resource * surface_resource, wl_resource * seat_resource,
			uint32_t serial, int32_t x, int32_t y);

	virtual void xdg_surface_set_title(wl_client * client, wl_resource * resource,
			const char * title);

	virtual void xdg_surface_move(wl_client * client, wl_resource * resource,
			wl_resource* seat_resource, uint32_t serial);

	virtual void xdg_surface_resize(wl_client* client, wl_resource * resource,
			wl_resource * seat_resource, uint32_t serial, uint32_t edges);

	virtual void xdg_surface_ack_configure(wl_client * client,
			wl_resource * resource, uint32_t serial);

	virtual void xdg_surface_set_window_geometry(wl_client * client,
			wl_resource * resource, int32_t x, int32_t y, int32_t width,
			int32_t height);

	virtual void xdg_surface_set_maximized(wl_client * client,
			wl_resource * resource);

	virtual void xdg_surface_unset_maximized(wl_client* client,
			wl_resource* resource);

	virtual void xdg_surface_set_fullscreen(wl_client * client,
			wl_resource * resource, wl_resource * output_resource);

	virtual void xdg_surface_unset_fullscreen(wl_client * client,
			wl_resource * resource);

	virtual void xdg_surface_set_minimized(wl_client * client,
			wl_resource * resource);

	/**
	 * xdg-popup
	 **/

	virtual void xdg_popup_destroy(wl_client * client, wl_resource * resource);

};

extern display_compositor_t * dc;

}



#endif /* SRC_DISPLAY_COMPOSITOR_HXX_ */
