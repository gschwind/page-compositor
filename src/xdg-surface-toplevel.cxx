/*
 * managed_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "leak_checker.hxx"

#include <typeinfo>

#include <linux/input.h>
#include <compositor.h>

#include <xdg-surface-toplevel.hxx>

#include "renderable_floating_outer_gradien.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"
#include "xdg-shell-client.hxx"
#include "wl-shell-client.hxx"

#include "view-base.hxx"
#include "xdg-surface-toplevel-view.hxx"

namespace page {

using namespace std;

void xdg_surface_toplevel_t::delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xs = xdg_surface_toplevel_t::get(resource);
	xs->destroy_all_views();
	xs->destroy.signal(xs);
	delete xs;
}

void xdg_surface_toplevel_t::xdg_surface_delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xs = xdg_surface_toplevel_t::get(resource);
	xs->destroy_all_views();
	xs->destroy.signal(xs);
	delete xs;
}

xdg_surface_toplevel_t::xdg_surface_toplevel_t(
		page_context_t * ctx,
		wl_client * client,
		weston_surface * surface,
		uint32_t id) :
	xdg_surface_base_t{ctx, client, surface, id},
	_pending{},
	_ack_serial{0}
{
	weston_log("ALLOC xdg_surface_toplevel_t %p\n", this);

	rect pos{0,0,surface->width, surface->height};

	weston_log("window default position = %s\n", pos.to_string().c_str());

	/* tell weston how to use this data */
	if (weston_surface_set_role(surface, "xdg_toplevel",
			_resource, XDG_SHELL_ERROR_ROLE) < 0)
		throw "TODO";

	_resource = wl_resource_create(_client,
			reinterpret_cast<wl_interface const *>(&xdg_surface_interface), 1,
			_id);

	xdg_surface_vtable::set_implementation(_resource);

}

//void xdg_surface_toplevel_t::set_xdg_surface_implementation() {
//	_resource = wl_resource_create(_client,
//			reinterpret_cast<wl_interface const *>(&xdg_surface_interface), 1,
//			_id);
//
//	xdg_surface_vtable::set_implementation(_resource);
//
//	_surface_send_configure = &xdg_surface_toplevel_t::xdg_surface_send_configure;
//
//}

//void xdg_surface_toplevel_t::set_wl_shell_surface_implementation() {
//	_resource = wl_resource_create(_client,
//			reinterpret_cast<wl_interface const *>(&wl_shell_surface_interface), 1,
//			_id);
//
//	wl_resource_set_implementation(_resource,
//			&_wl_shell_surface_implementation,
//			this, &xdg_surface_toplevel_t::delete_resource);
//
//	_surface_send_configure = &xdg_surface_toplevel_t::wl_surface_send_configure;
//
//}

xdg_surface_toplevel_t::~xdg_surface_toplevel_t() {
	weston_log("DELETE xdg_surface_toplevel_t %p\n", this);
	/* should not be usefull, delete must be call on resource destroy */
//	if(_resource) {
//		wl_resource_set_user_data(_resource, nullptr);
//	}

}

void xdg_surface_toplevel_t::weston_destroy() {
	destroy_all_views();

	if(_surface) {
		wl_list_remove(&_surface_destroy.link);
		_surface->committed_private = nullptr;
		_surface->committed = nullptr;
		_surface = nullptr;
	}
}

void xdg_surface_toplevel_t::destroy_all_views() {
	if(not _master_view.expired()) {
		auto master_view = _master_view.lock();
		master_view->signal_destroy();
	}
}

void xdg_surface_toplevel_t::weston_configure(struct weston_surface * es,
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

auto xdg_surface_toplevel_t::get(wl_resource * r) -> xdg_surface_toplevel_t * {
	return reinterpret_cast<xdg_surface_toplevel_t*>(wl_resource_get_user_data(r));
}

auto xdg_surface_toplevel_t::resource() const -> wl_resource * {
	return _resource;
}

void xdg_surface_toplevel_t::xdg_surface_destroy(struct wl_client *client,
		struct wl_resource *resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	wl_resource_destroy(resource);
}

void xdg_surface_toplevel_t::xdg_surface_set_parent(wl_client * client,
		wl_resource * resource, wl_resource * parent_resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->_pending.transient_for = parent_resource;
}

void xdg_surface_toplevel_t::xdg_surface_set_app_id(struct wl_client *client,
		       struct wl_resource *resource,
		       const char *app_id)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);

}

void xdg_surface_toplevel_t::xdg_surface_show_window_menu(wl_client *client,
			     wl_resource *surface_resource,
			     wl_resource *seat_resource,
			     uint32_t serial,
			     int32_t x,
			     int32_t y)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(surface_resource);

}

void xdg_surface_toplevel_t::xdg_surface_set_title(wl_client *client,
			wl_resource *resource, const char *title)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	_pending.title = title;
}

void xdg_surface_toplevel_t::xdg_surface_move(struct wl_client *client, struct wl_resource *resource,
		 struct wl_resource *seat_resource, uint32_t serial)
{
	//weston_log("call %s\n", __PRETTY_FUNCTION__);

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

void xdg_surface_toplevel_t::xdg_surface_resize(struct wl_client *client, struct wl_resource *resource,
		   struct wl_resource *seat_resource, uint32_t serial,
		   uint32_t edges)
{
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
			BTN_LEFT, x, y, static_cast<xdg_surface_resize_edge>(edges)));
	}

}

void xdg_surface_toplevel_t::xdg_surface_ack_configure(wl_client *client,
		wl_resource * resource,
		uint32_t serial)
{

	if(serial == _ack_serial)
		_ack_serial = 0;

}

void xdg_surface_toplevel_t::xdg_surface_set_window_geometry(struct wl_client *client,
				struct wl_resource *resource,
				int32_t x,
				int32_t y,
				int32_t width,
				int32_t height)
{
	_pending.geometry = rect(x, y, width, height);
}

void xdg_surface_toplevel_t::xdg_surface_set_maximized(struct wl_client *client,
			  struct wl_resource *resource)
{
	_pending.maximized = true;
}

void xdg_surface_toplevel_t::xdg_surface_unset_maximized(struct wl_client *client,
			    struct wl_resource *resource)
{
	_pending.maximized = false;
}

void xdg_surface_toplevel_t::xdg_surface_set_fullscreen(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *output_resource)
{
	_pending.fullscreen = true;
}

void xdg_surface_toplevel_t::xdg_surface_unset_fullscreen(struct wl_client *client,
			     struct wl_resource *resource)
{
	_pending.fullscreen = false;
}

void xdg_surface_toplevel_t::xdg_surface_set_minimized(struct wl_client *client,
			    struct wl_resource *resource)
{
	_pending.minimized = true;
}

auto xdg_surface_toplevel_t::create_view() -> view_toplevel_p {
	auto view = make_shared<view_toplevel_t>(_ctx, this);
	_master_view = view;
	return view;
}

auto xdg_surface_toplevel_t::master_view() -> view_toplevel_w {
	return _master_view;
}

void xdg_surface_toplevel_t::minimize() {
	if(_master_view.expired())
		return;
	auto master_view = _master_view.lock();

	if(not master_view->is(MANAGED_NOTEBOOK)) {
		_ctx->bind_window(master_view, true);
	}

}

xdg_surface_toplevel_t * xdg_surface_toplevel_t::get(weston_surface * surface) {
	return dynamic_cast<xdg_surface_toplevel_t*>(
			xdg_surface_base_t::get(surface));
}

view_base_p xdg_surface_toplevel_t::base_master_view() {
	return dynamic_pointer_cast<view_base_t>(_master_view.lock());
}

weston_surface * xdg_surface_toplevel_t::surface() const {
	return _surface;
}

weston_view * xdg_surface_toplevel_t::create_weston_view() {
	return weston_view_create(_surface);
}

int32_t xdg_surface_toplevel_t::width() const {
	return _surface->width;
}

int32_t xdg_surface_toplevel_t::height() const {
	return _surface->height;
}

string const & xdg_surface_toplevel_t::title() const {
	return _current.title;
}

void xdg_surface_toplevel_t::send_configure(int32_t width, int32_t height, set<uint32_t> const & states) {
	_ack_serial = wl_display_next_serial(_ctx->_dpy);

	wl_array array;
	wl_array_init(&array);
	wl_array_add(&array, sizeof(uint32_t)*states.size());

	{
		int i = 0;
		for(auto x: states) {
			((uint32_t*)array.data)[i] = x;
			++i;
		}
	}

	::xdg_surface_send_configure(_resource, width, height, &array, _ack_serial);
	wl_array_release(&array);
	wl_client_flush(_client);
}

void xdg_surface_toplevel_t::send_close() {
	xdg_surface_send_close(_resource);
}


}

