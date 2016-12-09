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

#include "view.hxx"

namespace page {

using namespace std;

map<uint32_t, edge_e> const xdg_surface_toplevel_t::_edge_map{
	{XDG_SURFACE_RESIZE_EDGE_NONE, EDGE_NONE},
	{XDG_SURFACE_RESIZE_EDGE_TOP, EDGE_TOP},
	{XDG_SURFACE_RESIZE_EDGE_BOTTOM, EDGE_BOTTOM},
	{XDG_SURFACE_RESIZE_EDGE_LEFT, EDGE_LEFT},
	{XDG_SURFACE_RESIZE_EDGE_TOP_LEFT, EDGE_TOP_LEFT},
	{XDG_SURFACE_RESIZE_EDGE_BOTTOM_LEFT, EDGE_BOTTOM_LEFT},
	{XDG_SURFACE_RESIZE_EDGE_RIGHT, EDGE_RIGHT},
	{XDG_SURFACE_RESIZE_EDGE_TOP_RIGHT, EDGE_TOP_RIGHT},
	{XDG_SURFACE_RESIZE_EDGE_BOTTOM_RIGHT, EDGE_BOTTOM_RIGHT}
};

void xdg_surface_toplevel_t::xdg_surface_delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	delete this;
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
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	rect pos{0,0,surface->width, surface->height};

	weston_log("window default position = %s\n", pos.to_string().c_str());

	_resource = wl_resource_create(_client, &xdg_surface_interface, 1, _id);
	xdg_surface_vtable::set_implementation(_resource);

}

xdg_surface_toplevel_t::~xdg_surface_toplevel_t() {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
}

void xdg_surface_toplevel_t::surface_destroyed(struct weston_surface * s) {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(_resource);
}

void xdg_surface_toplevel_t::surface_commited(struct weston_surface * es)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if(_master_view.expired()) {
		/* tell weston how to use this data */
		if (weston_surface_set_role(_surface, "xdg_toplevel",
				_resource, XDG_SHELL_ERROR_ROLE) < 0)
			throw "TODO";

		_ctx->manage_client(this);
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

edge_e xdg_surface_toplevel_t::edge_map(uint32_t edge) {
	auto x = _edge_map.find(edge);
	if(x == _edge_map.end()) {
		weston_log("warning unexpected edge found");
		return EDGE_NONE;
	}
	return x->second;
}

auto xdg_surface_toplevel_t::get(wl_resource * r) -> xdg_surface_toplevel_t * {
	return dynamic_cast<xdg_surface_toplevel_t*>(resource_get<xdg_surface_vtable>(r));
}

auto xdg_surface_toplevel_t::resource() const -> wl_resource * {
	return _resource;
}

void xdg_surface_toplevel_t::xdg_surface_destroy(struct wl_client *client,
		struct wl_resource *resource)
{
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(_resource);
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
	auto seat = resource_get<struct weston_seat>(seat_resource);
	_ctx->start_move(this, seat, serial);
}

void xdg_surface_toplevel_t::xdg_surface_resize(struct wl_client *client, struct wl_resource *resource,
		   struct wl_resource *seat_resource, uint32_t serial,
		   uint32_t edges)
{
	auto seat = resource_get<struct weston_seat>(seat_resource);
	_ctx->start_resize(this, seat, serial, edge_map(edges));
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

auto xdg_surface_toplevel_t::create_view() -> view_p {
	auto view = make_shared<view_t>(_ctx, this);
	_master_view = view;
	return view;
}

auto xdg_surface_toplevel_t::master_view() -> view_w {
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

surface_t * xdg_surface_toplevel_t::page_surface() {
	return this;
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
	wl_client_flush(_client);
}

void xdg_surface_toplevel_t::send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) {
	/* should not be called */
}

}

