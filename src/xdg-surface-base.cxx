/*
 * client_base.cxx
 *
 *  Created on: 5 ao��t 2015
 *      Author: gschwind
 */

#include <xdg-surface-base.hxx>

namespace page {

using namespace std;

xdg_surface_base_t::xdg_surface_base_t(
		page_context_t * ctx,
		wl_client * client,
		weston_surface * surface,
		uint32_t id) :
	_ctx{ctx},
	_resource{nullptr},
	_client{client},
	_surface{surface},
	_id{id}
{
	/**
	 * weston_surface are used for wl_surface, but those surfaces can have
	 * several role, configure_private may hold xdg_surface_toplevel or
	 * xdg_surface_popup_t. To avoid mistake configure_private always store
	 * xdg_surface_base, allowing dynamic_cast.
	 **/
	_surface->committed_private = this;
	_surface->committed = &xdg_surface_base_t::_weston_configure;

	_surface_destroy.notify = [] (wl_listener *l, void *data) {
		auto surface = reinterpret_cast<weston_surface*>(data);
		auto ths = xdg_surface_base_t::get(surface);
		ths->weston_destroy();
	};

	wl_signal_add(&surface->destroy_signal, &_surface_destroy);

}

xdg_surface_base_t::~xdg_surface_base_t() {

	if(_surface) {
		wl_list_remove(&_surface_destroy.link);
		_surface->committed_private = nullptr;
		_surface->committed = nullptr;
		_surface = nullptr;
	}

}

void xdg_surface_base_t::_weston_configure(weston_surface * es, int32_t sx,
		int32_t sy)
{
	auto ths = xdg_surface_base_t::get(es);
	ths->weston_configure(es, sx, sy);
}

xdg_surface_base_t * xdg_surface_base_t::get(weston_surface * surface)
{
	return reinterpret_cast<xdg_surface_base_t *>(surface->committed_private);
}


}


