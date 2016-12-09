/*
 * unmanaged_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "xdg-surface-popup.hxx"

#include "xdg-shell-client.hxx"
#include "xdg-surface-popup.hxx"
#include "xdg-surface-toplevel.hxx"

#include "view.hxx"

#include "xdg-shell-unstable-v5-server-protocol.h"

namespace page {

using namespace std;

auto xdg_surface_popup_t::get(wl_resource * r) -> xdg_surface_popup_t * {
	return dynamic_cast<xdg_surface_popup_t*>(resource_get<xdg_popup_vtable>(r));
}

void xdg_surface_popup_t::surface_first_commited(weston_surface * es)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if (weston_surface_set_role(_surface, "xdg_popup",
			_resource, XDG_SHELL_ERROR_ROLE) < 0)
		return;

	_ctx->manage_popup(this);

	on_surface_commit.disconnect();
	on_surface_commit.connect(&_surface->commit_signal, this, &xdg_surface_popup_t::surface_commited);

}

void xdg_surface_popup_t::surface_commited(weston_surface * es)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

}

void xdg_surface_popup_t::xdg_popup_delete_resource(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	delete this;
}

xdg_surface_popup_t::xdg_surface_popup_t(
		  page_context_t * ctx,
		  wl_client * client,
		  wl_resource * resource,
		  uint32_t id,
		  weston_surface * surface,
		  xdg_surface_base_t * parent,
		  weston_seat * seat,
		  uint32_t serial,
		  int32_t x, int32_t y) :
		xdg_surface_base_t{ctx, client, surface, id},
		id{id},
		_surface{surface},
		seat{seat},
		serial{serial}
{
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	_resource = wl_resource_create(client, &xdg_popup_interface, 1, _id);
	xdg_popup_vtable::set_implementation(_resource);

	on_surface_commit.connect(&_surface->commit_signal, this, &xdg_surface_popup_t::surface_first_commited);
	on_surface_destroy.connect(&_surface->destroy_signal, this, &xdg_surface_popup_t::surface_destroyed);

	_parent = parent->page_surface();
	x_offset = x;
	y_offset = y;

	_ctx->configure_popup(this);

}

xdg_surface_popup_t::~xdg_surface_popup_t() {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	on_surface_commit.disconnect();
	on_surface_destroy.disconnect();
}

void xdg_surface_popup_t::xdg_popup_destroy(wl_client * client, wl_resource * resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(resource);
}

auto xdg_surface_popup_t::create_view() -> view_p {
	auto view = make_shared<view_t>(_ctx, this);
	_master_view = view;
	return view;
}

auto xdg_surface_popup_t::master_view() -> view_w {
	return _master_view;
}

void xdg_surface_popup_t::surface_destroyed(struct weston_surface * s) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->destroy_surface(this);
	destroy.signal(this);
	wl_resource_destroy(_resource);
}

surface_t * xdg_surface_popup_t::page_surface() {
	return this;
}


/* page_surface_interface */
weston_surface * xdg_surface_popup_t::surface() const {
	return _surface;
}

weston_view * xdg_surface_popup_t::create_weston_view() {
	return weston_view_create(_surface);
}

int32_t xdg_surface_popup_t::width() const {
	return _surface->width;
}

int32_t xdg_surface_popup_t::height() const {
	return _surface->height;
}

string const & xdg_surface_popup_t::title() const {
	static string const s{"noname"};
	return s;
}

void xdg_surface_popup_t::send_configure(int32_t width, int32_t height, set<uint32_t> const & states) {
	/* disabled */
}

void xdg_surface_popup_t::send_close() {
	/* disabled */
}

void xdg_surface_popup_t::send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) {
	/* disabled */
}

}
