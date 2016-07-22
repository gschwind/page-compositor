/*
 * xdg-surface-popup-view.cxx
 *
 *  Created on: 14 juil. 2016
 *      Author: gschwind
 */

#include "xdg-surface-popup-view.hxx"

#include "xdg-surface-toplevel-view.hxx"

namespace page {

xdg_surface_popup_view_t::xdg_surface_popup_view_t(xdg_surface_popup_t * p) :
		_xdg_surface_popup{p}
{

	_default_view = weston_view_create(_xdg_surface_popup->_surface);

	if(strcmp("xdg_toplevel", _xdg_surface_popup->parent->role_name) == 0) {
		auto _xparent = reinterpret_cast<xdg_surface_toplevel_t*>(_xdg_surface_popup->parent->configure_private)->master_view();
		if(not _xparent.expired()) {
			weston_view_set_position(_default_view, p->x, p->y);
			weston_view_set_transform_parent(_default_view, _xparent.lock()->get_default_view());
		}
	} else if (strcmp("xdg_popup", _xdg_surface_popup->parent->role_name) == 0) {
		auto _xparent = reinterpret_cast<xdg_surface_popup_t*>(_xdg_surface_popup->parent->configure_private)->master_view();
		if(not _xparent.expired()) {
			weston_view_set_position(_default_view, p->x, p->y);
			weston_view_set_transform_parent(_default_view, _xparent.lock()->get_default_view());
		}
	}

}

xdg_surface_popup_view_t::~xdg_surface_popup_view_t()
{
	if(_default_view) {
		weston_view_destroy(_default_view);
		_default_view = nullptr;
	}
}

void xdg_surface_popup_view_t::add_popup_child(xdg_surface_popup_view_p child) {
	_children.push_back(child);
	connect(child->destroy, this, &xdg_surface_popup_view_t::destroy_popup_child);
}

void xdg_surface_popup_view_t::destroy_popup_child(xdg_surface_popup_view_t * c) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	disconnect(c->destroy);
	_children.remove(c->shared_from_this());
}

auto xdg_surface_popup_view_t::get_default_view() const -> weston_view * {
	return _default_view;
}

void xdg_surface_popup_view_t::signal_destroy() {
	destroy.signal(this);
}

}
