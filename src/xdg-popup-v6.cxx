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
	_positioner{positioner},
	_parent{parent}
{
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	self_resource = wl_resource_create(client, &zxdg_popup_v6_interface, 1, id);
	zxdg_popup_v6_vtable::set_implementation(self_resource);

}

xdg_popup_v6_t::~xdg_popup_v6_t() {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
}

void xdg_popup_v6_t::surface_commited(weston_surface * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if(_base->_master_view.expired()) {
		/* tell weston how to use this data */
		if (weston_surface_set_role(_base->_surface, "xdg_popup_v6",
				self_resource, ZXDG_SHELL_V6_ERROR_ROLE) < 0)
			throw "TODO";

		auto xview = _base->create_view();
		auto base = xdg_surface_v6_t::get(_parent);
		auto positioner = xdg_positioner_v6_t::get(_positioner);
		weston_log("%p\n", base);
		auto parent_view = base->_master_view.lock();
		weston_log("%p\n", parent_view.get());

		weston_log("x=%d, y=%d\n", positioner->x_offset, positioner->y_offset);
		if(parent_view != nullptr) {
			weston_log("x=%d, y=%d\n", positioner->x_offset, positioner->y_offset);
			parent_view->add_popup_child(xview, positioner->x_offset, positioner->y_offset);

			/* the popup does not get shown if it is not configured */
			zxdg_popup_v6_send_configure(self_resource, positioner->x_offset, positioner->y_offset, surface()->width, surface()->height);
			_base->_ack_config = wl_display_next_serial(_ctx->_dpy);
			zxdg_surface_v6_send_configure(_base->_resource, _base->_ack_config);
			wl_client_flush(_client);
		}
	}

	_ctx->sync_tree_view();

}

void xdg_popup_v6_t::zxdg_popup_v6_destroy(struct wl_client * client, struct wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_base->destroy_all_views();
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

}

