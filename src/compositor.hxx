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

#ifndef SRC_COMPOSITOR_HXX_
#define SRC_COMPOSITOR_HXX_

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <libweston-0/compositor.h>
#include <libweston-0/compositor-x11.h>

#include "listener.hxx"
#include "xdg-shell-server-protocol.h"
#include "xdg-shell.hxx"

#include <list>

namespace page {

using namespace std;

class page_root_t;

struct compositor_t {
	wl_display* dpy;
	weston_compositor* ec;
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

	compositor_t();
	void connect_all();


	void on_output_created(weston_output * output);

	void run();

};

extern compositor_t* cmp;

}


#endif /* SRC_COMPOSITOR_HXX_ */
