/*
 * xdg-surface-popup-view.cxx
 *
 *  Created on: 14 juil. 2016
 *      Author: gschwind
 */

#include "view-popup.hxx"

#include "view-toplevel.hxx"

namespace page {

view_popup_t::view_popup_t(page_context_t * ctx, page_surface_interface * p) :
		_page_surface{p}
{
	_default_view = weston_view_create(_page_surface->surface());
}

view_popup_t::~view_popup_t()
{
	if(_default_view) {
		weston_view_destroy(_default_view);
		_default_view = nullptr;
	}
}

void view_popup_t::add_popup_child(view_popup_p c,
		int x, int y) {
	_children.push_back(c);
	connect(c->destroy, this, &view_popup_t::destroy_popup_child);
	weston_view_set_transform_parent(c->get_default_view(), _default_view);
	weston_view_set_position(c->get_default_view(), x, y);
	weston_view_schedule_repaint(c->get_default_view());

}

void view_popup_t::destroy_popup_child(view_popup_t * c) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	disconnect(c->destroy);
	_children.remove(c->shared_from_this());
}

auto view_popup_t::get_default_view() const -> weston_view * {
	return _default_view;
}

void view_popup_t::signal_destroy() {
	destroy.signal(this);
}


}
