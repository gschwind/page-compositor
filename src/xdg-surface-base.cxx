/*
 * client_base.cxx
 *
 *  Created on: 5 août 2015
 *      Author: gschwind
 */

#include <xdg-surface-base.hxx>

namespace page {

using namespace std;

xdg_surface_base_t::xdg_surface_base_t(
		page_context_t * ctx,
		xdg_shell_client_t * xdg_shell_client,
		wl_client * client,
		weston_surface * surface,
		uint32_t id) :
	_ctx{ctx},
	_xdg_shell_client{xdg_shell_client},
	_resource{nullptr},
	_client{client},
	_surface{surface},
	_id{id}
{

	_surface->configure = &xdg_surface_base_t::_weston_configure;

	/**
	 * weston_surface are used for wl_surface, but those surfaces can have
	 * several role, configure_private may hold xdg_surface_toplevel or
	 * xdg_surface_popup_t. To avoid mistake configure_private always store
	 * xdg_surface_base, allowing dynamic_cast.
	 **/
	_surface->configure_private = this;

	_surface_destroy.notify = [] (wl_listener *l, void *data) {
		weston_surface * surface = reinterpret_cast<weston_surface*>(data);
		auto ths = reinterpret_cast<xdg_surface_base_t*>(surface->configure_private);
		ths->weston_destroy();
	};

	wl_signal_add(&surface->destroy_signal, &_surface_destroy);

}

xdg_surface_base_t::~xdg_surface_base_t() {

	if(_surface) {
		wl_list_remove(&_surface_destroy.link);
		_surface->configure_private = nullptr;
		_surface->configure = nullptr;
		_surface = nullptr;
	}

}

void xdg_surface_base_t::_weston_configure(weston_surface * es, int32_t sx,
		int32_t sy)
{
	auto ths = reinterpret_cast<xdg_surface_base_t *>(es->configure_private);
	ths->weston_configure(es, sx, sy);
}

void xdg_surface_base_t::weston_configure(weston_surface * es, int32_t sx,
		int32_t sy) {

}

void xdg_surface_base_t::weston_destroy() {

}


}


