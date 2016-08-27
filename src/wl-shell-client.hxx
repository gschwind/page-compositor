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

#ifndef WL_SHELL_CLIENT_HXX_
#define WL_SHELL_CLIENT_HXX_

#include "wl-shell-interface.hxx"
#include "utils.hxx"
#include "page_context.hxx"

namespace page {

struct wl_shell_client_t : public connectable_t, public wl_shell_vtable {
	page_context_t * _ctx;
	wl_client * _client;
	uint32_t _id;

	signal_t<wl_shell_client_t *> destroy;

	wl_resource * _wl_shell_resource;

	wl_shell_client_t(page_context_t * ctx, wl_client * client, uint32_t id);
	virtual ~wl_shell_client_t();

	virtual void wl_shell_get_shell_surface(struct wl_client *client,
				  struct wl_resource *resource,
				  uint32_t id,
				  struct wl_resource *surface) override;

	static void wl_shell_delete(struct wl_resource *resource);
	static wl_shell_client_t * get(wl_resource * resource);

};

}



#endif /* WL_SHELL_CLIENT_HXX_ */
