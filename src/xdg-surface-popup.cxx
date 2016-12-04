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
	return reinterpret_cast<xdg_surface_popup_t *>(
			wl_resource_get_user_data(r));
}

void xdg_surface_popup_t::weston_configure(weston_surface * es, int32_t sx,
		int32_t sy)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if(_master_view.expired()) {
		auto xview = create_view();
		auto base = xdg_surface_base_t::get(parent);
		weston_log("%p\n", base);
		auto parent_view = base->base_master_view();
		weston_log("%p\n", parent_view.get());

		if(parent_view != nullptr) {
			parent_view->add_popup_child(xview, x, y);
		}

		_ctx->sync_tree_view();

	}

}

static void xdg_popup_destroy(wl_client * client, wl_resource * resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto p = xdg_surface_popup_t::get(resource);
	p->xdg_popup_destroy(client, resource);
}

static struct xdg_popup_interface const _xdg_popup_implementation = {
		xdg_popup_destroy
};

void xdg_surface_popup_t::xdg_popup_delete(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xs = xdg_surface_popup_t::get(resource);
	xs->destroy_all_views();
	xs->destroy.signal(xs);
	delete xs;
}

xdg_surface_popup_t::xdg_surface_popup_t(
		  page_context_t * ctx,
		  wl_client * client,
		  wl_resource * resource,
		  uint32_t id,
		  weston_surface * surface,
		  weston_surface * parent,
		  weston_seat * seat,
		  uint32_t serial,
		  int32_t x, int32_t y) :
		xdg_surface_base_t{ctx, client, surface, id},
		id{id},
		_surface{surface},
		parent{parent},
		seat{seat},
		serial{serial},
		x{x},
		y{y}
{
	weston_log("ALLOC xdg_surface_popup_t %p\n", this);

	_resource = wl_resource_create(client,
			reinterpret_cast<wl_interface const *>(&xdg_popup_interface), 1, id);

	wl_resource_set_implementation(_resource,
			&_xdg_popup_implementation,
			this,
			&xdg_surface_popup_t::xdg_popup_delete);

	/* tell weston how to use this data */
	if (weston_surface_set_role(surface, "xdg_popup",
			_resource, XDG_SHELL_ERROR_ROLE) < 0)
		throw "TODO";


}

xdg_surface_popup_t::~xdg_surface_popup_t() {
	weston_log("DELETE xdg_surface_popup_t %p\n", this);
	/* should not be usefull, delete must be call on resource destroy */
//	if(_resource) {
//		wl_resource_set_user_data(_resource, nullptr);
//	}

}

void xdg_surface_popup_t::xdg_popup_destroy(wl_client * client, wl_resource * resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	assert(resource == _resource);
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

void xdg_surface_popup_t::weston_destroy() {
	destroy_all_views();

	if(_surface) {
		wl_list_remove(&_surface_destroy.link);
		_surface->committed_private = nullptr;
		_surface->committed = nullptr;
		_surface = nullptr;
	}

}

void xdg_surface_popup_t::destroy_all_views() {
	if(not _master_view.expired()) {
		_master_view.lock()->signal_destroy();
	}
}

xdg_surface_popup_t * xdg_surface_popup_t::get(weston_surface * surface) {
	return dynamic_cast<xdg_surface_popup_t*>(
			xdg_surface_base_t::get(surface));
}

view_p xdg_surface_popup_t::base_master_view() {
	return dynamic_pointer_cast<view_t>(_master_view.lock());
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

}
