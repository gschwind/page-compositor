/*
 * client_base.cxx
 *
 *  Created on: 5 ao��t 2015
 *      Author: gschwind
 */

#include <xdg-shell-v5-surface-base.hxx>

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
}

xdg_surface_base_t::~xdg_surface_base_t() {
	on_surface_commit.disconnect();
	on_surface_destroy.disconnect();
}

}


