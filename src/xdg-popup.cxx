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

#include "compositor.hxx"
#include "xdg-popup.hxx"
#include "xdg-shell.hxx"

using namespace std;

xdg_popup_t * xdg_popup_t::get(wl_resource * resource) {
	return reinterpret_cast<xdg_popup_t*>(wl_resource_get_user_data(resource));
}

static void
xdg_popup_destroy(struct wl_client *client,
		  struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct xdg_popup_interface xdg_popup_implementation = {
	xdg_popup_destroy
};

static void
shell_popup_configure(struct weston_surface * s, int32_t x , int32_t y) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	weston_log("s = %p, x = %d, y = %d\n", s, x, y);

	auto sh = reinterpret_cast<xdg_popup_t*>(s->configure_private);

	if(s->output != nullptr)
		weston_output_schedule_repaint(s->output);
}

static void
xdg_popup_delete(struct wl_resource *resource)
{
	delete xdg_popup_t::get(resource);
}

xdg_popup_t::xdg_popup_t(wl_client *client, uint32_t id,
		weston_surface * surface, int32_t x, int32_t y) :
		client{client},
		id{id},
		surface{surface},
		ox{x},
		oy{y}
{

	resource = wl_resource_create(client, &xdg_surface_interface, 1, id);
	wl_resource_set_implementation(resource, &xdg_popup_implementation, this, &xdg_popup_delete);

	/** tell weston how to use this data **/
	if (weston_surface_set_role(surface, "xdg_popup",
				    resource, XDG_SHELL_ERROR_ROLE) < 0)
		throw "TODO";

	/* the first output */
	weston_output* output = wl_container_of(cmp->ec->output_list.next,
			output, link);

	surface->configure = &shell_popup_configure;
	surface->configure_private = this;

	surface->output = output;

	view = weston_view_create(surface);
	weston_view_set_position(view, ox, oy);
	surface->timeline.force_refresh = 1;
	weston_layer_entry_insert(&cmp->default_layer.view_list, &view->layer_link);

	//view->output = output;

	weston_view_geometry_dirty(view);

}

