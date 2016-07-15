/*
 * unmanaged_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "xdg-surface-popup.hxx"

#include "xdg-shell-server-protocol.h"

#include "xdg-shell-client.hxx"
#include "xdg-surface-popup-view.hxx"

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
}

static void xdg_popup_destroy(wl_client * client, wl_resource * resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto p = xdg_surface_popup_t::get(resource);
	p->xdg_popup_destroy(client, resource);
}

struct xdg_popup_interface xx_xdg_popup_interface = {
		xdg_popup_destroy
};

void xdg_surface_popup_t::xdg_popup_delete(struct wl_resource *resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	auto xs = xdg_surface_popup_t::get(resource);
	xs->_xdg_shell_client->destroy_popup(xs);
	delete xs;
}

xdg_surface_popup_t::xdg_surface_popup_t(
		  page_context_t * ctx,
		  xdg_shell_client_t * xdg_shell_client,
		  wl_client * client,
		  wl_resource * resource,
		  uint32_t id,
		  weston_surface * surface,
		  weston_surface * parent,
		  weston_seat * seat,
		  uint32_t serial,
		  int32_t x, int32_t y) :
		xdg_surface_base_t{ctx, xdg_shell_client, client, surface, id}
{
	weston_log("ALLOC xdg_surface_popup_t %p\n", this);

	auto _xparent = reinterpret_cast<xdg_surface_base_t*>(parent->configure_private);

	_resource =
			wl_resource_create(client, &xdg_popup_interface, 1, id);

	wl_resource_set_implementation(_resource,
			&xx_xdg_popup_interface,
			this,
			&xdg_surface_popup_t::xdg_popup_delete);

}

xdg_surface_popup_t::~xdg_surface_popup_t() {
	weston_log("DELETE xdg_surface_popup_t %p\n", this);
	/* should not be usefull, delete must be call on resource destroy */
	if(_resource) {
		wl_resource_set_user_data(_resource, nullptr);
	}

}

void xdg_surface_popup_t::xdg_popup_destroy(wl_client * client, wl_resource * resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	assert(resource == _resource);
	wl_resource_destroy(resource);
}

auto xdg_surface_popup_t::create_view() -> xdg_surface_popup_view_p {
	auto view = make_shared<xdg_surface_popup_view_t>(this);
	_master_view = view;
	return view;
}

auto xdg_surface_popup_t::master_view() -> xdg_surface_popup_view_w {
	return _master_view;
}

}
