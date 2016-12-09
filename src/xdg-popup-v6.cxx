/*
 * xdg-toplevel-v6.cxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */


#include "xdg-popup-v6.hxx"
#include "xdg-positioner-v6.hxx"
#include "xdg-shell-unstable-v6-server-protocol.h"
#include "xdg-shell-unstable-v5-server-protocol.h"

#include "view.hxx"

namespace page {

xdg_popup_v6_t::xdg_popup_v6_t(
		page_context_t * ctx,
		wl_client * client,
		xdg_surface_v6_t * surface,
		uint32_t id,
		struct wl_resource * parent,
		struct wl_resource * positioner
) :
	self_resource{nullptr},
	_base{surface},
	_id{id},
	_client{client},
	_ctx{ctx},
	_is_configured{false}
{
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	self_resource = wl_resource_create(client, &zxdg_popup_v6_interface, 1, id);
	zxdg_popup_v6_vtable::set_implementation(self_resource);

	connect(_base->destroy, this, &xdg_popup_v6_t::surface_destroyed);
	connect(_base->commited, this, &xdg_popup_v6_t::surface_commited);

	_parent = xdg_surface_v6_t::get(parent)->_role;
	assert(_parent != nullptr);

	auto pos = xdg_positioner_v6_t::get(positioner);
	x_offset = pos->x_offset;
	y_offset = pos->y_offset;

}

xdg_popup_v6_t::~xdg_popup_v6_t() {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
}

void xdg_popup_v6_t::surface_destroyed(xdg_surface_v6_t * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(self_resource);
}

void xdg_popup_v6_t::surface_commited(xdg_surface_v6_t * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if (not _is_configured) {
		/* ask page to configure the popup */
		_ctx->configure_popup(this);
		_is_configured = true;
		return;
	}

	if(_base->_ack_config != 0)
		return;

	if(not _master_view.expired())
		return;

	if (weston_surface_set_role(_base->_surface, "xdg_popup_v6",
			self_resource, ZXDG_SHELL_V6_ERROR_ROLE) < 0)
		return;

	_ctx->manage_popup(this);

}

void xdg_popup_v6_t::zxdg_popup_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(self_resource);
}

void xdg_popup_v6_t::zxdg_popup_v6_grab(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_popup_v6_t::zxdg_popup_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	delete this;
}

weston_surface * xdg_popup_v6_t::surface() const {
	return _base->_surface;
}

weston_view * xdg_popup_v6_t::create_weston_view() {
	return weston_view_create(_base->_surface);
}

int32_t xdg_popup_v6_t::width() const {
	return _base->_surface->width;
}

int32_t xdg_popup_v6_t::height() const {
	return _base->_surface->height;
}

string const & xdg_popup_v6_t::title() const {
	static string const s{"noname"};
	return s;
}

void xdg_popup_v6_t::send_configure(int32_t width, int32_t height, set<uint32_t> const & states) {
	/* disable */
}

void xdg_popup_v6_t::send_close() {
	/* disable */
}

void xdg_popup_v6_t::send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	zxdg_popup_v6_send_configure(self_resource, x, y, width, height);
	_base->_ack_config = wl_display_next_serial(_ctx->_dpy);
	zxdg_surface_v6_send_configure(_base->_resource, _base->_ack_config);
	wl_client_flush(_client);
}

}

