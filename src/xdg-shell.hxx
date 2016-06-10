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

#ifndef SRC_XDG_SHELL_HXX_
#define SRC_XDG_SHELL_HXX_

#include <list>

#include <libweston-0/compositor.h>

#include "xdg-shell-server-protocol.h"

namespace page {

using namespace std;

struct xdg_shell_t {
	wl_client * client;
	uint32_t id;

	wl_resource * resource;

	list<wl_resource *> xdg_shell_surfaces;

	xdg_shell_t(wl_client * client, uint32_t id);

	static xdg_shell_t * get(wl_resource * resource);

};

extern const struct xdg_shell_interface xdg_shell_implementation;

}


#endif /* SRC_XDG_SHELL_HXX_ */
