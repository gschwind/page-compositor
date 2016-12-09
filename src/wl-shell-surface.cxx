/*
 * Copyright (2010-2016) Benoit Gschwind
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

#include "wl-shell-surface.hxx"

#include <typeinfo>

#include <linux/input.h>
#include <compositor.h>
#include <surface.hxx>
#include "renderable_floating_outer_gradien.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"
#include "wl-shell-client.hxx"

#include "view.hxx"

namespace page {

using namespace std;

void wl_shell_surface_t::wl_shell_surface_delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	delete this;
}

wl_shell_surface_t::wl_shell_surface_t(
		page_context_t * ctx,
		wl_client * client,
		weston_surface * surface,
		uint32_t id) :
	_ctx{ctx},
	_id{id},
	_client{client},
	_surface{surface},
	_current{},
	_ack_serial{0}
{
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	_resource = wl_resource_create(_client, &wl_shell_surface_interface, 1, _id);
	wl_shell_surface_vtable::set_implementation(_resource);

	on_surface_destroy.connect(&_surface->destroy_signal, this, &wl_shell_surface_t::surface_destroyed);
	on_surface_commit.connect(&_surface->commit_signal, this, &wl_shell_surface_t::surface_commited);

}

wl_shell_surface_t::~wl_shell_surface_t() {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	if(_surface) {
		on_surface_destroy.disconnect();
		on_surface_commit.disconnect();
	}
}

auto wl_shell_surface_t::get(struct wl_resource * r) -> wl_shell_surface_t * {
	return dynamic_cast<wl_shell_surface_t*>(resource_get<wl_shell_surface_vtable>(r));
}

void wl_shell_surface_t::surface_commited(struct weston_surface * s) {
	if(_width != s->width or _heigth != s->height) {
		_width = s->width;
		_heigth = s->height;
		if(not _master_view.expired())
			_master_view.lock()->update_view();
	}
}

void wl_shell_surface_t::surface_destroyed(struct weston_surface * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(_resource);
}

void wl_shell_surface_t::wl_shell_surface_pong(wl_client *client,
	     wl_resource *resource,
	     uint32_t serial) {
	/* TODO */

}

void wl_shell_surface_t::wl_shell_surface_move(wl_client *client,
	     wl_resource *resource,
	     wl_resource *seat_resource,
	     uint32_t serial) {

	if(_master_view.expired())
		return;

	auto seat = reinterpret_cast<weston_seat*>(
			wl_resource_get_user_data(seat_resource));

	auto pointer = weston_seat_get_pointer(seat);
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	auto master_view = _master_view.lock();
	if(master_view->is(MANAGED_NOTEBOOK)) {
		_ctx->grab_start(pointer, new grab_bind_client_t{_ctx, master_view,
			BTN_LEFT, rect(x, y, 1, 1)});
	} else if(master_view->is(MANAGED_FLOATING)) {
		_ctx->grab_start(pointer, new grab_floating_move_t(_ctx, master_view,
			BTN_LEFT, x, y));
	}

	// start_move(ps, seat, serial)

}

void wl_shell_surface_t::wl_shell_surface_resize(wl_client *client,
	       wl_resource *resource,
	       wl_resource *seat_resource,
	       uint32_t serial,
	       uint32_t edges) {

	if(_master_view.expired())
		return;

	auto seat = reinterpret_cast<weston_seat*>(
			wl_resource_get_user_data(seat_resource));

	auto pointer = weston_seat_get_pointer(seat);
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	auto master_view = _master_view.lock();
	if(master_view->is(MANAGED_FLOATING)) {
		_ctx->grab_start(pointer, new grab_floating_resize_t(_ctx, master_view,
			BTN_LEFT, x, y, static_cast<xdg_surface_resize_edge>(edges))); // FIXME: map enum edges.
	}

	// start_resize(ps, seat, serial)

}

void wl_shell_surface_t::wl_shell_surface_set_toplevel(wl_client *client,
		     wl_resource *resource) {
	/* tell weston how to use this data */

	if(_master_view.expired()) {
		if (weston_surface_set_role(_surface, "wl_shell_surface_toplevel",
				_resource, WL_SHELL_ERROR_ROLE) < 0)
			return;
		_ctx->manage_client(this);
	}

}

void wl_shell_surface_t::wl_shell_surface_set_transient(wl_client *client,
		      wl_resource *resource,
		      wl_resource *parent,
		      int32_t x,
		      int32_t y,
		      uint32_t flags) {
	_current.transient_for = parent;

}

void wl_shell_surface_t::wl_shell_surface_set_fullscreen(wl_client *client,
		       wl_resource *resource,
		       uint32_t method,
		       uint32_t framerate,
		       wl_resource *output) {
	_current.minimized = false;
	_current.fullscreen = true;
	_current.maximized = false;
	/* TODO: switch to fullscreen */
}

void wl_shell_surface_t::wl_shell_surface_set_popup(wl_client *client,
		  wl_resource *resource,
		  wl_resource *seat,
		  uint32_t serial,
		  wl_resource *parent,
		  int32_t x,
		  int32_t y,
		  uint32_t flags) {

	if(not _master_view.expired())
		return;

	/* tell weston how to use this data */
	if (weston_surface_set_role(_surface, "wl_shell_surface_popup",
			_resource, WL_SHELL_ERROR_ROLE) < 0)
		return;

	_parent = wl_shell_surface_t::get(parent);
	x_offset = x;
	y_offset = y;

	_ctx->manage_popup(this);

	// start_popup(ps, ps, x, y)
	// start_grab_popup(ps, seat)

}

void wl_shell_surface_t::wl_shell_surface_set_maximized(wl_client *client,
		      wl_resource *resource,
		      wl_resource *output) {
	_current.minimized = false;
	_current.fullscreen = false;
	_current.maximized = true;
}

void wl_shell_surface_t::wl_shell_surface_set_title(wl_client *client,
		  wl_resource *resource,
		  const char *title) {
	_current.title = title;

	if(not _master_view.expired()) {
		_master_view.lock()->signal_title_change();
	}

	// update_title(ps)

}

void wl_shell_surface_t::wl_shell_surface_set_class(wl_client *client,
		  wl_resource *resource,
		  const char *class_) {
	str_class = class_;
}

weston_surface * wl_shell_surface_t::surface() const {
	return _surface;
}

weston_view * wl_shell_surface_t::create_weston_view() {
	return weston_view_create(_surface);
}

int32_t wl_shell_surface_t::width() const {
	return _surface->width;
}

int32_t wl_shell_surface_t::height() const {
	return _surface->height;
}

string const & wl_shell_surface_t::title() const {
	return _current.title;
}

void wl_shell_surface_t::send_configure(int32_t width, int32_t height, set<uint32_t> const & states) {
	_ack_serial = 0;
	wl_shell_surface_send_configure(_resource,
			WL_SHELL_SURFACE_RESIZE_NONE, width, height);
	wl_client_flush(_client);
}

void wl_shell_surface_t::send_close() {
	/* cannot send close */
}

void wl_shell_surface_t::send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) {
	/* should not be called */
}

}

