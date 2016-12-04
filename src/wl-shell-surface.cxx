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

#include "renderable_floating_outer_gradien.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"
#include "wl-shell-client.hxx"

#include "view-toplevel.hxx"

namespace page {

using namespace std;

void wl_shell_surface_t::delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xs = wl_shell_surface_t::get(resource);
	xs->destroy_all_views();
	xs->destroy.signal(xs);
	delete xs;
}

void wl_shell_surface_t::wl_shell_surface_delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xs = wl_shell_surface_t::get(resource);
	xs->destroy_all_views();
	xs->destroy.signal(xs);
	delete xs;
}

wl_shell_surface_t::wl_shell_surface_t(
		page_context_t * ctx,
		wl_client * client,
		weston_surface * surface,
		uint32_t id) :
	xdg_surface_base_t{ctx, client, surface, id},
	_pending{},
	_ack_serial{0}
{
	weston_log("ALLOC wl_shell_surface_t %p\n", this);

	rect pos{0,0,surface->width, surface->height};

	weston_log("window default position = %s\n", pos.to_string().c_str());

	/* tell weston how to use this data */
	if (weston_surface_set_role(surface, "xdg_toplevel",
			_resource, XDG_SHELL_ERROR_ROLE) < 0)
		throw "TODO";

	_resource = wl_resource_create(_client,
			reinterpret_cast<wl_interface const *>(&wl_shell_surface_interface), 1, _id);
	wl_shell_surface_vtable::set_implementation(_resource);

}

//void wl_shell_surface_t::set_xdg_surface_implementation() {
//	_resource = wl_resource_create(_client,
//			reinterpret_cast<wl_interface const *>(&xdg_surface_interface), 1,
//			_id);
//
//	xdg_surface_vtable::set_implementation(_resource);
//
//	_surface_send_configure = &wl_shell_surface_t::xdg_surface_send_configure;
//
//}
//
//void wl_shell_surface_t::set_wl_shell_surface_implementation() {
//	_resource = wl_resource_create(_client,
//			reinterpret_cast<wl_interface const *>(&wl_shell_surface_interface), 1,
//			_id);
//
//	wl_resource_set_implementation(_resource,
//			&_wl_shell_surface_implementation,
//			this, &wl_shell_surface_t::delete_resource);
//
//	_surface_send_configure = &wl_shell_surface_t::wl_surface_send_configure;
//
//}

wl_shell_surface_t::~wl_shell_surface_t() {
	weston_log("DELETE wl_shell_surface_t %p\n", this);
	/* should not be usefull, delete must be call on resource destroy */
//	if(_resource) {
//		wl_resource_set_user_data(_resource, nullptr);
//	}

}

void wl_shell_surface_t::weston_destroy() {
	destroy_all_views();

	if(_surface) {
		wl_list_remove(&_surface_destroy.link);
		_surface->committed_private = nullptr;
		_surface->committed = nullptr;
		_surface = nullptr;
	}

}

void wl_shell_surface_t::destroy_all_views() {
	if(not _master_view.expired()) {
		auto master_view = _master_view.lock();
		master_view->signal_destroy();
	}
}

void wl_shell_surface_t::weston_configure(struct weston_surface * es,
		int32_t sx, int32_t sy)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if(_master_view.expired()) {
		_ctx->manage_client(create_view());
	}

	/* configuration is invalid */
	if(_ack_serial != 0)
		return;

	if(_pending.maximized != _current.maximized) {
		if(_pending.maximized) {
			/* on maximize */
		} else {
			/* on unmaximize */
		}
	}

	if(_pending.minimized != _current.minimized) {
		if(_pending.minimized) {
			minimize();
			_pending.minimized = false;
		} else {
			/* on unminimize */
		}
	}

	if(_pending.transient_for != _current.transient_for) {

	}

	if(_pending.title != _current.title) {
		_current.title = _pending.title;
		if(not _master_view.expired()) {
			_master_view.lock()->signal_title_change();
		}
	}

	if(not _master_view.expired()) {
		_master_view.lock()->update_view();
	}

	_current = _pending;

}

auto wl_shell_surface_t::get(wl_resource * r) -> wl_shell_surface_t * {
	return reinterpret_cast<wl_shell_surface_t*>(wl_resource_get_user_data(r));
}

auto wl_shell_surface_t::resource() const -> wl_resource * {
	return _resource;
}

auto wl_shell_surface_t::create_view() -> view_toplevel_p {
	auto view = make_shared<view_toplevel_t>(_ctx, this);
	_master_view = view;
	return view;
}

auto wl_shell_surface_t::master_view() -> view_toplevel_w {
	return _master_view;
}

void wl_shell_surface_t::minimize() {
	if(_master_view.expired())
		return;
	auto master_view = _master_view.lock();

	if(not master_view->is(MANAGED_NOTEBOOK)) {
		_ctx->bind_window(master_view, true);
	}

}

wl_shell_surface_t * wl_shell_surface_t::get(weston_surface * surface) {
	return dynamic_cast<wl_shell_surface_t*>(
			xdg_surface_base_t::get(surface));
}

view_toplevel_p wl_shell_surface_t::base_master_view() {
	return dynamic_pointer_cast<view_toplevel_t>(_master_view.lock());
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

	if(master_view().expired())
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
}

void wl_shell_surface_t::wl_shell_surface_resize(wl_client *client,
	       wl_resource *resource,
	       wl_resource *seat_resource,
	       uint32_t serial,
	       uint32_t edges) {

	if(master_view().expired())
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

}

void wl_shell_surface_t::wl_shell_surface_set_toplevel(wl_client *client,
		     wl_resource *resource) {

}

void wl_shell_surface_t::wl_shell_surface_set_transient(wl_client *client,
		      wl_resource *resource,
		      wl_resource *parent,
		      int32_t x,
		      int32_t y,
		      uint32_t flags) {

}

void wl_shell_surface_t::wl_shell_surface_set_fullscreen(wl_client *client,
		       wl_resource *resource,
		       uint32_t method,
		       uint32_t framerate,
		       wl_resource *output) {
	_pending.minimized = false;
	_pending.fullscreen = true;
	_pending.maximized = false;
}

void wl_shell_surface_t::wl_shell_surface_set_popup(wl_client *client,
		  wl_resource *resource,
		  wl_resource *seat,
		  uint32_t serial,
		  wl_resource *parent,
		  int32_t x,
		  int32_t y,
		  uint32_t flags) {

}

void wl_shell_surface_t::wl_shell_surface_set_maximized(wl_client *client,
		      wl_resource *resource,
		      wl_resource *output) {
	_pending.minimized = false;
	_pending.fullscreen = false;
	_pending.maximized = true;
}

void wl_shell_surface_t::wl_shell_surface_set_title(wl_client *client,
		  wl_resource *resource,
		  const char *title) {
	_pending.title = title;

}

void wl_shell_surface_t::wl_shell_surface_set_class(wl_client *client,
		  wl_resource *resource,
		  const char *class_) {
	/* TODO */
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
	::wl_shell_surface_send_configure(_resource,
			WL_SHELL_SURFACE_RESIZE_TOP_LEFT, width, height);
	wl_client_flush(_client);
}

void wl_shell_surface_t::send_close() {
	xdg_surface_send_close(_resource);
}


}

