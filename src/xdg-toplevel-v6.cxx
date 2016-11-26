/*
 * xdg-toplevel-v6.cxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */


#include "xdg-toplevel-v6.hxx"
#include "xdg-shell-unstable-v6-server-protocol.h"

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

	self_resource = wl_resource_create(client,
			reinterpret_cast<wl_interface const *>(&zxdg_toplevel_v6_interface), 1, id);
	zxdg_toplevel_v6_vtable::set_implementation(self_resource);

}

void xdg_toplevel_v6_t::surface_commited(weston_surface * es, int32_t sx, int32_t sy) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

//	if(_master_view.expired()) {
//		_ctx->manage_client(create_view());
//	}

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

auto xdg_toplevel_v6_t::create_view() -> xdg_surface_toplevel_view_p {
	auto view = make_shared<xdg_surface_toplevel_view_t>(_ctx, this);
	_master_view = view;
	return view;
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_parent(struct wl_client * client, struct wl_resource * resource, struct wl_resource * parent)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_title(struct wl_client * client, struct wl_resource * resource, const char * title)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
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

void xdg_toplevel_v6_t::zxdg_toplevel_v6_move(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_resize(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
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
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_unset_maximized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_fullscreen(struct wl_client * client, struct wl_resource * resource, struct wl_resource * output)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_unset_fullscreen(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_toplevel_v6_t::zxdg_toplevel_v6_set_minimized(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
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

	zxdg_toplevel_v6_send_configure(self_resource, width, height, &array);
	zxdg_surface_v6_send_configure(_surface->_resource, _ack_serial);
	wl_array_release(&array);
	wl_client_flush(_client);
}

void xdg_toplevel_v6_t::send_close() {
	zxdg_toplevel_v6_send_close(self_resource);
	wl_client_flush(_client);
}

}

