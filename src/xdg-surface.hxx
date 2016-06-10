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

#ifndef SRC_XDG_SURFACE_HXX_
#define SRC_XDG_SURFACE_HXX_

#include <list>

#include "xdg-shell-server-protocol.h"

namespace page {

using namespace std;

struct xdg_surface_t {
	wl_client *client;
	xdg_shell_t * shell;
	uint32_t id;
	weston_surface * surface;

	/* handle the wayland resource related to this xdg_surface */
	wl_resource * resource;
	weston_view * view;

	xdg_surface_t(wl_client *client, xdg_shell_t * shell, uint32_t id, weston_surface * surface);

	static xdg_surface_t * get(wl_resource * resource);

};

}


#endif /* SRC_XDG_SURFACE_HXX_ */
