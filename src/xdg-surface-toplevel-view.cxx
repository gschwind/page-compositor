/*
 * xdg_surface_toplevel_view.cxx
 *
 * copyright (2016) Benoit Gschwind
 *
 */

#include "xdg-surface-toplevel-view.hxx"

#include <typeinfo>

#include <cairo.h>
#include <linux/input.h>

#include "xdg-surface-popup-view.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"
#include "xdg-shell-server-protocol.h"
#include "xdg-surface-toplevel.hxx"

namespace page {

using namespace std;

auto xdg_surface_toplevel_view_t::xdg_surface() -> xdg_surface_toplevel_t * {
	return _xdg_surface;
}

void xdg_surface_toplevel_view_t::add_transient_child(xdg_surface_toplevel_view_p c) {
	_transient_childdren->push_back(c);
}

void xdg_surface_toplevel_view_t::add_popup_child(xdg_surface_popup_view_p c) {
	_popups_childdren->push_back(c);
}

xdg_surface_toplevel_view_t::xdg_surface_toplevel_view_t(
		xdg_surface_toplevel_t * xdg_surface) :
	_floating_wished_position{},
	_notebook_wished_position{},
	_wished_position{},
	_xdg_surface{xdg_surface}
{
	weston_log("ALLOC xdg_surface_toplevel_t %p\n", this);

	rect pos{0,0,_xdg_surface->surface()->width, _xdg_surface->surface()->height};

	weston_log("window default position = %s\n", pos.to_string().c_str());

	_managed_type = MANAGED_UNCONFIGURED;

	_floating_wished_position = pos;
	_notebook_wished_position = pos;

	_popups_childdren = make_shared<tree_t>();
	push_back(_popups_childdren);
	_transient_childdren = make_shared<tree_t>();
	push_back(_transient_childdren);

//	/** if x == 0 then place window at center of the screen **/
//	if (_floating_wished_position.x == 0 and not is(MANAGED_DOCK)) {
//		_floating_wished_position.x =
//				(_client_proxy->geometry().width - _floating_wished_position.w) / 2;
//	}
//
//	if(_floating_wished_position.x - _ctx->theme()->floating.margin.left < 0) {
//		_floating_wished_position.x = _ctx->theme()->floating.margin.left;
//	}
//
//	/**
//	 * if y == 0 then place window at center of the screen
//	 **/
//	if (_floating_wished_position.y == 0 and not is(MANAGED_DOCK)) {
//		_floating_wished_position.y = (_client_proxy->geometry().height - _floating_wished_position.h) / 2;
//	}
//
//	if(_floating_wished_position.y - _ctx->theme()->floating.margin.top < 0) {
//		_floating_wished_position.y = _ctx->theme()->floating.margin.top;
//	}

	/**
	 * Create the base window, window that will content managed window
	 **/

	rect b = _floating_wished_position;

	//update_floating_areas();

	uint32_t cursor;

	//cursor = cnx()->xc_top_side;

	//select_inputs_unsafe();

	//update_icon();

}

xdg_surface_toplevel_view_t::~xdg_surface_toplevel_view_t() {
	weston_log("DELETE xdg_surface_toplevel_t %p\n", this);
}

auto xdg_surface_toplevel_view_t::shared_from_this() -> xdg_surface_toplevel_view_p {
	return dynamic_pointer_cast<xdg_surface_toplevel_view_t>(tree_t::shared_from_this());
}

void xdg_surface_toplevel_view_t::reconfigure() {
	if(not _default_view)
		return;

	if (is(MANAGED_FLOATING)) {
		_wished_position = _floating_wished_position;
	} else {
		_wished_position = _notebook_wished_position;
	}

	if(_default_view) {
		weston_view_set_position(_default_view, _wished_position.x,
				_wished_position.y);
		weston_view_geometry_dirty(_default_view);
	}

	_xdg_surface->_ack_serial = weston_compositor_get_time();

	wl_array array;
	wl_array_init(&array);
	wl_array_add(&array, sizeof(uint32_t));
	((uint32_t*)array.data)[0] = XDG_SURFACE_STATE_MAXIMIZED;
	if(_has_focus) {
		wl_array_add(&array, sizeof(uint32_t));
		((uint32_t*)array.data)[1] = XDG_SURFACE_STATE_ACTIVATED;
	}
	xdg_surface_send_configure(_xdg_surface->_resource, _wished_position.w,
			_wished_position.h, &array, _xdg_surface->_ack_serial);
	wl_array_release(&array);
	wl_client_flush(_xdg_surface->_client);

}


void xdg_surface_toplevel_view_t::set_managed_type(managed_window_type_e type)
{
	_managed_type = type;
	reconfigure();
}

rect xdg_surface_toplevel_view_t::get_base_position() const {
	return _wished_position;
}

managed_window_type_e xdg_surface_toplevel_view_t::get_type() {
	return _managed_type;
}

bool xdg_surface_toplevel_view_t::is(managed_window_type_e type) {
	return _managed_type == type;
}


/**
 * set usual passive button grab for a focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
//void xdg_surface_toplevel_t::grab_button_focused_unsafe() {
//
//
//}

/**
 * set usual passive button grab for a not focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
//void xdg_surface_toplevel_t::grab_button_unfocused_unsafe() {
//
//}

bool xdg_surface_toplevel_view_t::is_fullscreen() {
	return _managed_type == MANAGED_FULLSCREEN;
}

//bool xdg_surface_toplevel_t::skip_task_bar() {
//	return false;
//}
//
//void xdg_surface_toplevel_t::net_wm_state_delete() {
//
//}

//void xdg_surface_toplevel_t::normalize() {
//	if(not _is_iconic)
//		return;
//	_is_iconic = false;
//}
//
//void xdg_surface_toplevel_t::iconify() {
//	if(_is_iconic)
//		return;
//	_is_iconic = true;
//	_is_activated = false;
//}

//void xdg_surface_toplevel_t::wm_state_delete() {
//	/**
//	 * This one is for removing the window manager tag, thus only check if the window
//	 * still exist. (don't need lock);
//	 **/
//
//	//_client_proxy->delete_wm_state();
//}

void xdg_surface_toplevel_view_t::set_floating_wished_position(rect const & pos) {
	_floating_wished_position = pos;
}

void xdg_surface_toplevel_view_t::set_notebook_wished_position(rect const & pos) {
	_notebook_wished_position = pos;
}

rect const & xdg_surface_toplevel_view_t::get_wished_position() {
	return _wished_position;
}

rect const & xdg_surface_toplevel_view_t::get_floating_wished_position() {
	return _floating_wished_position;
}
//
//bool xdg_surface_toplevel_t::has_window(xcb_window_t w) const {
//	return false;
//}

string xdg_surface_toplevel_view_t::get_node_name() const {
	string s = _get_node_name<'M'>();
	ostringstream oss;
	oss << s << " " << (void*)nullptr << " " << title();
	return oss.str();
}

//rect const & xdg_surface_toplevel_t::base_position() const {
//	return _wished_position;
//}
//
//rect const & xdg_surface_toplevel_t::orig_position() const {
//	return _wished_position;
//}

void xdg_surface_toplevel_view_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

}

void xdg_surface_toplevel_view_t::render_finished() {

}

void xdg_surface_toplevel_view_t::set_focus_state(bool is_focused) {
	_has_change = true;
	_has_focus = is_focused;
	queue_redraw();
}

void xdg_surface_toplevel_view_t::hide() {

	for(auto x: _children) {
		x->hide();
	}

	if(_default_view) {
		weston_view_unmap(_default_view);
		weston_view_destroy(_default_view);
		_default_view = nullptr;
		_ctx->sync_tree_view();
	}

	_is_visible = false;
}

void xdg_surface_toplevel_view_t::show() {
	_is_visible = true;

	if(not _default_view) {
		_default_view = weston_view_create(_xdg_surface->_surface);
		reconfigure();
		weston_log("bbXX %p\n", _xdg_surface->_surface->compositor);
		_ctx->sync_tree_view();
	}

	for(auto x: _children) {
		x->show();
	}

}

//bool xdg_surface_toplevel_t::is_iconic() {
//	return _is_iconic;
//}

//bool xdg_surface_toplevel_t::is_stiky() {
//	return false;
//}
//
//bool xdg_surface_toplevel_t::is_modal() {
//	return false;
//}

void xdg_surface_toplevel_view_t::activate() {

	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}

	reconfigure();

	queue_redraw();

}

void xdg_surface_toplevel_view_t::queue_redraw() {

	if(_default_view) {
		weston_view_schedule_repaint(_default_view);
	}

	if(is(MANAGED_FLOATING)) {
		//_is_resized = true;
	} else {
		tree_t::queue_redraw();
	}
}

void xdg_surface_toplevel_view_t::trigger_redraw() {

}

void xdg_surface_toplevel_view_t::send_close() {
	xdg_surface_send_close(_xdg_surface->_resource);
}

string const & xdg_surface_toplevel_view_t::title() const {
	return _xdg_surface->_current.title;
}



void xdg_surface_toplevel_view_t::weston_configure(struct weston_surface * es,
		int32_t sx, int32_t sy)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	/* configuration is invalid */
	if(_xdg_surface->_ack_serial != 0)
		return;

	auto ptr = shared_from_this();

	if(_xdg_surface->_pending.maximized != _xdg_surface->_current.maximized) {
		if(_xdg_surface->_pending.maximized) {
			/* on maximize */
		} else {
			/* on unmaximize */
		}
	}

	if(_xdg_surface->_pending.minimized != _xdg_surface->_current.minimized) {
		if(_xdg_surface->_pending.minimized) {
			/* on minimize */
		} else {
			/* on unminimize */
		}
	}

	_xdg_surface->_current = _xdg_surface->_pending;

	if(is(MANAGED_UNCONFIGURED)) {
		_ctx->manage_client(ptr);
	} else {
		/* TODO: update the state if necessary */
		if(_default_view)
			weston_view_schedule_repaint(_default_view);
	}

}

auto xdg_surface_toplevel_view_t::get(wl_resource * r) -> xdg_surface_toplevel_t * {
	return reinterpret_cast<xdg_surface_toplevel_t*>(wl_resource_get_user_data(r));
}

auto xdg_surface_toplevel_view_t::resource() const -> wl_resource * {
	return _xdg_surface->_resource;
}

}
