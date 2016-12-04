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
	on_surface_commit.connect(&_surface->commit_signal, this, &xdg_surface_base_t::surface_commited);
	on_surface_destroy.connect(&_surface->destroy_signal, this, &xdg_surface_base_t::surface_destroyed);
}

xdg_surface_base_t::~xdg_surface_base_t() {

}

}


