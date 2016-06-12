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

#ifndef SRC_XDG_SHELL_INTERFACE_HXX_
#define SRC_XDG_SHELL_INTERFACE_HXX_

#include <libweston-0/compositor.h>

namespace page {

struct xdg_shell_vtable {

	virtual ~xdg_shell_vtable() = default;

	virtual void xdg_shell_destroy(wl_client * client,
			wl_resource * resource) = 0;
	virtual void xdg_shell_use_unstable_version(wl_client * client,
			wl_resource * resource, int32_t version) = 0;
	virtual void xdg_shell_get_xdg_surface(wl_client * client,
			wl_resource * resource, uint32_t id,
			wl_resource * surface_resource) = 0;
	virtual void xdg_shell_get_xdg_popup(wl_client * client,
			wl_resource * resource, uint32_t id, wl_resource * surface,
			wl_resource * parent, wl_resource* seat_resource, uint32_t serial,
			int32_t x, int32_t y) = 0;
	virtual void xdg_shell_pong(wl_client * client, wl_resource * resource,
			uint32_t serial) = 0;

};

}

#endif /* SRC_XDG_SHELL_INTERFACE_HXX_ */
