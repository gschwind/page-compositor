/*
 * xdg-toplevel-v6.cxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */


#include "xdg-toplevel-v6.hxx"

#include <linux/input.h>

#include "xdg-shell-unstable-v6-server-protocol.h"
#include "grab_handlers.hxx"

namespace page {

xdg_toplevel_v6_t::xdg_toplevel_v6_t(
		page_context_t * ctx,
		wl_client * client,
		xdg_surface_v6_t * surface,
		uint32_t id) :
	self_resource{nullptr},
	_surface{surface},
	_id{id},
	_client{client},
	_ctx{ctx}
{

	/* tell weston how to use this data */
	if (weston_surface_set_role(surface->_surface, "xdg_toplevel_v6",
			surface->_resource, XDG_SHELL_ERROR_ROLE) < 0)
		throw "TODO";

	self_resource = wl_resource_create(client,
			reinterpret_cast<wl_interface const *>(&zxdg_toplevel_v6_interface), 1, id);
	zxdg_toplevel_v6_vtable::set_implementation(self_resource);

}

void xdg_toplevel_v6_t::destroy_all_views() {
	if(not _master_view.expired()) {
		auto master_view = _master_view.lock();
		master_view->signal_destroy();
	}
}

void xdg_toplevel_v6_t::surface_commited(weston_surface * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if(_master_view.expired()) {
		_ctx->manage_client(create_view());
	}

	/* configuration is invalid */
	if(_surface->_ack_config != 0)
		return;
	_surface->_ack_config = 0;

	if(_pending.maximized != _current.maximized) {
		if(_pending.maximized) {
			/* on maximize */
		} else {
			/* on unmaximize */
		}
	}

	if(_pending.minimized != _current.minimized) {
		if(_pending.minimized) {
			//minimize();
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

auto xdg_toplevel_v6_t::create_view() -> view_toplevel_p {
	auto view = make_shared<view_toplevel_t>(_ctx, this);
	_master_view = view;
	return view;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
	wl_resource_destroy(self_resource);
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_parent(struct wl_client * client, struct wl_resource * resource, struct wl_resource * parent)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_pending.transient_for = parent;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_title(struct wl_client * client, struct wl_resource * resource, const char * title)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_pending.title = title;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_app_id(struct wl_client * client, struct wl_resource * resource, const char * app_id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_show_window_menu(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, int32_t x, int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_move(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat_resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
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
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_resize(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat_resource, uint32_t serial, uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
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
			BTN_LEFT, x, y, static_cast<xdg_surface_resize_edge>(edges)));
	}
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_max_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_min_size(struct wl_client * client, struct wl_resource * resource, int32_t width, int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_maximized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_pending.maximized = true;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_unset_maximized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_pending.maximized = false;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_fullscreen(struct wl_client * client, struct wl_resource * resource, struct wl_resource * output)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_pending.fullscreen = true;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_unset_fullscreen(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_pending.fullscreen = false;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_minimized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_pending.minimized = true;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

weston_surface * xdg_toplevel_v6_t::surface() const {
	return _surface->_surface;
}

weston_view * xdg_toplevel_v6_t::create_weston_view() {
	return weston_view_create(_surface->_surface);
}

int32_t xdg_toplevel_v6_t::width() const {
	return _surface->_surface->width;
}

int32_t xdg_toplevel_v6_t::height() const {
	return _surface->_surface->height;
}

string const & xdg_toplevel_v6_t::title() const {
	return _current.title;
}

void xdg_toplevel_v6_t::send_configure(int32_t width, int32_t height, set<uint32_t> const & states) {
	_surface->_ack_config = wl_display_next_serial(_ctx->_dpy);

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

	zxdg_toplevel_v6_send_configure(self_resource, width, height, &array);
	zxdg_surface_v6_send_configure(_surface->_resource, _surface->_ack_config);
	wl_array_release(&array);
	wl_client_flush(_client);
}

void xdg_toplevel_v6_t::send_close() {
	zxdg_toplevel_v6_send_close(self_resource);
	wl_client_flush(_client);
}

}

