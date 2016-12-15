/*
 * xdg-toplevel-v6.cxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */


#include <linux/input.h>
#include <xdg-shell-v6-toplevel.hxx>

#include "xdg-shell-unstable-v6-server-protocol.h"
#include "grab_handlers.hxx"

namespace page {

map<uint32_t, edge_e> const xdg_toplevel_v6_t::_edge_map{
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_NONE, EDGE_NONE},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP, EDGE_TOP},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM, EDGE_BOTTOM},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_LEFT, EDGE_LEFT},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_LEFT, EDGE_TOP_LEFT},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_LEFT, EDGE_BOTTOM_LEFT},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_RIGHT, EDGE_RIGHT},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_RIGHT, EDGE_TOP_RIGHT},
	{ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_RIGHT, EDGE_BOTTOM_RIGHT}
};

xdg_toplevel_v6_t::xdg_toplevel_v6_t(
		page_context_t * ctx,
		wl_client * client,
		xdg_surface_v6_t * surface,
		uint32_t id) :
	self_resource{nullptr},
	_base{surface},
	_id{id},
	_client{client},
	_ctx{ctx}
{
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	self_resource = wl_resource_create(client, &zxdg_toplevel_v6_interface, 1, id);
	zxdg_toplevel_v6_vtable::set_implementation(self_resource);
	connect(_base->destroy, this, &xdg_toplevel_v6_t::surface_destroyed);
	connect(_base->commited, this, &xdg_toplevel_v6_t::surface_first_commited);
}

xdg_toplevel_v6_t::~xdg_toplevel_v6_t() {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
}

void xdg_toplevel_v6_t::surface_destroyed(xdg_surface_v6_t * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(self_resource);
}

void xdg_toplevel_v6_t::surface_first_commited(xdg_surface_v6_t * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	_transient_for = _pending.transient_for;

	/* tell weston how to use this data */
	if (weston_surface_set_role(_base->_surface, "xdg_toplevel_v6",
			self_resource, ZXDG_SHELL_V6_ERROR_ROLE) < 0)
		return;
	_ctx->manage_client(this);

	disconnect(_base->commited);
	connect(_base->commited, this, &xdg_toplevel_v6_t::surface_commited);

}

void xdg_toplevel_v6_t::surface_commited(xdg_surface_v6_t * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	/* configuration is invalid */
	if(_base->_ack_config != 0)
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
			//minimize();
			_pending.minimized = false;
		} else {
			/* on unminimize */
		}
	}

	if(_pending.transient_for != _transient_for) {
		_transient_for = _pending.transient_for;
		/* TODO */
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

edge_e xdg_toplevel_v6_t::edge_map(uint32_t edge) {
	auto x = _edge_map.find(edge);
	if(x == _edge_map.end()) {
		weston_log("warning unexpected edge found");
		return EDGE_NONE;
	}
	return x->second;
}

auto xdg_toplevel_v6_t::get(struct wl_resource * r) -> xdg_toplevel_v6_t * {
	return dynamic_cast<xdg_toplevel_v6_t*>(resource_get<zxdg_toplevel_v6_vtable>(r));
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(self_resource);
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_parent(struct wl_client * client, struct wl_resource * resource, struct wl_resource * parent)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	if(parent) {
		_pending.transient_for = xdg_toplevel_v6_t::get(parent);
	} else {
		_pending.transient_for = nullptr;
	}
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
	auto seat = resource_get<struct weston_seat>(seat_resource);
	_ctx->start_move(this, seat, serial);
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_resize(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat_resource, uint32_t serial, uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto seat = resource_get<struct weston_seat>(seat_resource);
	_ctx->start_resize(this, seat, serial, edge_map(edges));

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
	delete this;
}

weston_surface * xdg_toplevel_v6_t::surface() const {
	return _base->_surface;
}

weston_view * xdg_toplevel_v6_t::create_weston_view() {
	return weston_view_create(_base->_surface);
}

int32_t xdg_toplevel_v6_t::width() const {
	return _base->_surface->width;
}

int32_t xdg_toplevel_v6_t::height() const {
	return _base->_surface->height;
}

string const & xdg_toplevel_v6_t::title() const {
	return _current.title;
}

void xdg_toplevel_v6_t::send_configure(int32_t width, int32_t height, set<uint32_t> const & states) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

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

	weston_log("width=%d, height=%d\n", width, height);
	zxdg_toplevel_v6_send_configure(self_resource, width, height, &array);

	_base->_ack_config = wl_display_next_serial(_ctx->_dpy);
	zxdg_surface_v6_send_configure(_base->_resource, _base->_ack_config);
	wl_array_release(&array);
	wl_client_flush(_client);
}

void xdg_toplevel_v6_t::send_close() {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	zxdg_toplevel_v6_send_close(self_resource);
	wl_client_flush(_client);
}

void xdg_toplevel_v6_t::send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) {
	/* should not be called */
}

}

