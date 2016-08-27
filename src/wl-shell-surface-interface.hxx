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

#ifndef WL_SHELL_SURFACE_INTERFACE_HXX_
#define WL_SHELL_SURFACE_INTERFACE_HXX_

#include <typeinfo>
#include <libweston-0/compositor.h>

namespace page {

struct wl_shell_surface_vtable {

	virtual ~wl_shell_surface_vtable() = default;

	virtual void wl_shell_surface_pong(wl_client *client,
		     wl_resource *resource,
		     uint32_t serial) = 0;

	virtual void wl_shell_surface_move(wl_client *client,
		     wl_resource *resource,
		     wl_resource *seat,
		     uint32_t serial) = 0;

	virtual void wl_shell_surface_resize(wl_client *client,
		       wl_resource *resource,
		       wl_resource *seat,
		       uint32_t serial,
		       uint32_t edges) = 0;

	virtual void wl_shell_surface_set_toplevel(wl_client *client,
			     wl_resource *resource) = 0;

	virtual void wl_shell_surface_set_transient(wl_client *client,
			      wl_resource *resource,
			      wl_resource *parent,
			      int32_t x,
			      int32_t y,
			      uint32_t flags) = 0;

	virtual void wl_shell_surface_set_fullscreen(wl_client *client,
			       wl_resource *resource,
			       uint32_t method,
			       uint32_t framerate,
			       wl_resource *output) = 0;

	virtual void wl_shell_surface_set_popup(wl_client *client,
			  wl_resource *resource,
			  wl_resource *seat,
			  uint32_t serial,
			  wl_resource *parent,
			  int32_t x,
			  int32_t y,
			  uint32_t flags) = 0;

	virtual void wl_shell_surface_set_maximized(wl_client *client,
			      wl_resource *resource,
			      wl_resource *output) = 0;

	virtual void wl_shell_surface_set_title(wl_client *client,
			  wl_resource *resource,
			  const char *title) = 0;

	virtual void wl_shell_surface_set_class(wl_client *client,
			  wl_resource *resource,
			  const char *class_) = 0;

};

}


#endif /* WL_SHELL_SURFACE_INTERFACE_HXX_ */
