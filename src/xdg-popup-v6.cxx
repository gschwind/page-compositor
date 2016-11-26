/*
 * xdg-toplevel-v6.cxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */


#include "xdg-popup-v6.hxx"
#include "xdg-shell-unstable-v6-server-protocol.h"

namespace page {

xdg_popup_v6_t::xdg_popup_v6_t(
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
			reinterpret_cast<wl_interface const *>(&zxdg_popup_v6_interface), 1, id);
	zxdg_popup_v6_vtable::set_implementation(self_resource);

}

void xdg_popup_v6_t::zxdg_popup_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_popup_v6_t::zxdg_popup_v6_grab(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

void xdg_popup_v6_t::zxdg_popup_v6_delete_resource(struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

weston_surface * xdg_popup_v6_t::surface() const {
	return _surface->_surface;
}

weston_view * xdg_popup_v6_t::create_weston_view() {
	return weston_view_create(_surface->_surface);
}

int32_t xdg_popup_v6_t::width() const {
	return _surface->_surface->width;
}

int32_t xdg_popup_v6_t::height() const {
	return _surface->_surface->height;
}

string const & xdg_popup_v6_t::title() const {
	return string{"noname"};
}

void xdg_popup_v6_t::send_configure(int32_t width, int32_t height, set<uint32_t> const & states) {
	/* disable */
}

void xdg_popup_v6_t::send_close() {
	/* disable */
}

}

