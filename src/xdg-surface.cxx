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

#include <wayland-util.h>

#include <wayland-server-core.h>
#include <wayland-server.h>
#include "xdg-shell-server-protocol.h"
#include "compositor.hxx"
#include "xdg-shell.hxx"
#include "xdg-surface.hxx"

namespace page {

using namespace std;

extern const struct xdg_surface_interface xdg_surface_implementation;


struct weston_pointer_grab_move_t {
	struct weston_pointer_grab base;
	float origin_x;
	float origin_y;
	float origin_pointer_x;
	float origin_pointer_y;
};

static void
move_grab_focus(struct weston_pointer_grab* grab)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
move_grab_motion(weston_pointer_grab* _grab, uint32_t time,
	       weston_pointer_motion_event* event)
{

	auto grab = reinterpret_cast<weston_pointer_grab_move_t*>(_grab);

	auto pointer = grab->base.pointer;

	/** global position of the cursor when grab started **/
	double ox = wl_fixed_to_double(pointer->x);
	double oy = wl_fixed_to_double(pointer->y);

	/** relative move from the start of the grab **/
	double dx = event->x;
	double dy = event->y;

	/** update pointer position **/
	weston_pointer_move(pointer, event);

	/** move the surface from his invariant point **/
	weston_view_set_position(pointer->focus,
			ox + grab->origin_x,
			oy + grab->origin_y);
	weston_compositor_schedule_repaint(pointer->focus->surface->compositor);
}

static void
move_grab_button(struct weston_pointer_grab *grab,
		 uint32_t time, uint32_t button, uint32_t state_w)
{
	struct weston_pointer *pointer = grab->pointer;

	grab->pointer->focus;
	weston_log("move %f, %f\n",
			wl_fixed_to_double(grab->pointer->grab_x),
			wl_fixed_to_double(grab->pointer->grab_y));

	if (pointer->button_count == 0 &&
			state_w == WL_POINTER_BUTTON_STATE_RELEASED) {
		weston_pointer_end_grab(grab->pointer);
	}
}

static
void move_grab_axis(struct weston_pointer_grab* grab, uint32_t time,
	     struct weston_pointer_axis_event* event) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
move_grab_axis_source(struct weston_pointer_grab* grab, uint32_t source){
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
move_grab_frame(struct weston_pointer_grab* grab) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
move_grab_cancel(struct weston_pointer_grab *grab)
{
	weston_pointer_end_grab(grab->pointer);
}

static const struct weston_pointer_grab_interface move_grab_interface = {
	move_grab_focus,
	move_grab_motion,
	move_grab_button,
	move_grab_axis,
	move_grab_axis_source,
	move_grab_frame,
	move_grab_cancel
};

xdg_surface_t * xdg_surface_t::get(wl_resource * resource) {
	return reinterpret_cast<xdg_surface_t*>(wl_resource_get_user_data(resource));
}

static void
shell_surface_configure(struct weston_surface * s, int32_t x , int32_t y) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	weston_log("s = %p, x = %d, y = %d\n", s, x, y);

	auto sh = reinterpret_cast<xdg_surface_t*>(s->configure_private);

	weston_seat * seat;
	wl_list_for_each(seat, &cmp->ec->seat_list, link)
		weston_surface_activate(s, seat);

	if(s->output != nullptr)
		weston_output_schedule_repaint(s->output);
}

static void
xdg_surface_delete(struct wl_resource *resource)
{
	delete xdg_surface_t::get(resource);
}


xdg_surface_t::xdg_surface_t(wl_client *client, xdg_shell_t * shell, uint32_t id, weston_surface * surface) :
		client{client},
		shell{shell},
		id{id},
		surface{surface}
{

	resource = wl_resource_create(client, &xdg_surface_interface, 1, id);
	wl_resource_set_implementation(resource, &xdg_surface_implementation,
			this, xdg_surface_delete);

	/** tell weston how to use this data **/
	if (weston_surface_set_role(surface, "xdg_surface",
				    resource, XDG_SHELL_ERROR_ROLE) < 0)
		throw "TODO";

	/* the first output */
	weston_output* output = wl_container_of(cmp->ec->output_list.next,
		    output, link);

	surface->configure = &shell_surface_configure;
	surface->configure_private = this;

	surface->output = output;

	view = weston_view_create(surface);
	weston_view_set_position(view, 0, 0);
	surface->timeline.force_refresh = 1;

	wl_array array;
	wl_array_init(&array);
	wl_array_add(&array, sizeof(uint32_t)*2);
	((uint32_t*)array.data)[0] = XDG_SURFACE_STATE_MAXIMIZED;
	((uint32_t*)array.data)[1] = XDG_SURFACE_STATE_ACTIVATED;
	xdg_surface_send_configure(resource, 800, 800, &array, 10);
	wl_array_release(&array);

	weston_view_geometry_dirty(view);

}

static void
xdg_surface_destroy(struct wl_client *client,
		    struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
xdg_surface_set_parent(struct wl_client *client,
		       struct wl_resource *resource,
		       struct wl_resource *parent_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_set_app_id(struct wl_client *client,
		       struct wl_resource *resource,
		       const char *app_id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_show_window_menu(struct wl_client *client,
			     struct wl_resource *surface_resource,
			     struct wl_resource *seat_resource,
			     uint32_t serial,
			     int32_t x,
			     int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_set_title(struct wl_client *client,
			struct wl_resource *resource, const char *title)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_move(struct wl_client *client, struct wl_resource *resource,
		 struct wl_resource *seat_resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto seat = reinterpret_cast<weston_seat*>(wl_resource_get_user_data(seat_resource));
	auto xdg_surface = xdg_surface_t::get(resource);
	auto pointer = weston_seat_get_pointer(seat);

	weston_pointer_grab_move_t * grab_data =
			reinterpret_cast<weston_pointer_grab_move_t *>(malloc(sizeof *grab_data));
	/** TODO: memory error **/

	grab_data->base.interface = &move_grab_interface;
	grab_data->base.pointer = nullptr;

	/* relative client position from the cursor */
	grab_data->origin_x = xdg_surface->view->geometry.x - wl_fixed_to_double(pointer->grab_x);
	grab_data->origin_y = xdg_surface->view->geometry.y - wl_fixed_to_double(pointer->grab_y);

	wl_list_remove(&(xdg_surface->view->layer_link.link));
	wl_list_insert(&(cmp->default_layer.view_list.link),
			&(xdg_surface->view->layer_link.link));

	weston_pointer_start_grab(seat->pointer_state, &grab_data->base);

}

static void
xdg_surface_resize(struct wl_client *client, struct wl_resource *resource,
		   struct wl_resource *seat_resource, uint32_t serial,
		   uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_ack_configure(struct wl_client *client,
			  struct wl_resource *resource,
			  uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xdg_surface = xdg_surface_t::get(resource);
	weston_layer_entry_insert(&cmp->default_layer.view_list, &xdg_surface->view->layer_link);

}

static void
xdg_surface_set_window_geometry(struct wl_client *client,
				struct wl_resource *resource,
				int32_t x,
				int32_t y,
				int32_t width,
				int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_set_maximized(struct wl_client *client,
			  struct wl_resource *resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_unset_maximized(struct wl_client *client,
			    struct wl_resource *resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_set_fullscreen(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *output_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_unset_fullscreen(struct wl_client *client,
			     struct wl_resource *resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void
xdg_surface_set_minimized(struct wl_client *client,
			    struct wl_resource *resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

const struct xdg_surface_interface xdg_surface_implementation = {
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
	xdg_surface_set_minimized,
};

}
