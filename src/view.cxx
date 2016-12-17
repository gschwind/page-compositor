/*
 * xdg_surface_toplevel_view.cxx
 *
 * copyright (2016) Benoit Gschwind
 *
 */

#include "view.hxx"

#include <typeinfo>

#include <cairo.h>
#include <linux/input.h>
#include <xdg-shell-v5-surface-toplevel.hxx>

#include "renderable_floating_outer_gradien.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"

namespace page {

using namespace std;

void view_t::add_transient_child(view_p c) {
	_transient_childdren->push_back(c);
}

void view_t::add_popup_child(view_p c,
		int x, int y)
{
	c->set_managed_type(MANAGED_POPUP);
	_popups_childdren->push_back(c);
	weston_view_set_transform_parent(c->get_default_view(), _default_view);
	weston_view_set_position(c->get_default_view(), x, y);
	weston_view_schedule_repaint(c->get_default_view());
}

void view_t::destroy_popup_child(view_t * c) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_popups_childdren->remove(c->shared_from_this());
}

view_t::view_t(
		page_context_t * ctx,
		surface_t * xdg_surface) :
	_ctx{ctx},
	_floating_wished_position{},
	_notebook_wished_position{},
	_wished_position{},
	_page_surface{xdg_surface},
	_default_view{nullptr},
	_has_keyboard_focus{false},
	_has_change{true}
{
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	rect pos{0, 0, std::max(_page_surface->width(), 1), std::max(_page_surface->height(), 1)};

	weston_log("window default position = %s\n", pos.to_string().c_str());

	_managed_type = MANAGED_UNCONFIGURED;

	_floating_wished_position = pos;
	_notebook_wished_position = pos;

	_popups_childdren = make_shared<tree_t>();
	push_back(_popups_childdren);
	_transient_childdren = make_shared<tree_t>();
	push_back(_transient_childdren);

	weston_matrix_init(&_transform.matrix);
	wl_list_init(&_transform.link);

	_default_view = _page_surface->create_weston_view();
	update_view();

	_is_visible = true;

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

	show();

}

view_t::~view_t() {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	if(_default_view) {
		weston_view_destroy(_default_view);
		_default_view = nullptr;
	}
}

auto view_t::shared_from_this() -> view_p {
	return dynamic_pointer_cast<view_t>(tree_t::shared_from_this());
}

void view_t::update_view() {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if (is(MANAGED_NOTEBOOK) or is(MANAGED_FULLSCREEN)) {
		_wished_position = _notebook_wished_position;

		double ratio = compute_ratio_to_fit(_page_surface->width(),
				_page_surface->height(), _wished_position.w,
				_wished_position.h);

		/* if ratio > 1.0 then do not scale, just center */
		if(ratio >= 1.0) {
			ratio = 1.0;
		}

		if(_transform.link.next)
			wl_list_remove(&_transform.link);

		if(ratio != 1.0) {
			weston_matrix_init(&_transform.matrix);
			weston_matrix_scale(&_transform.matrix, ratio, ratio, 1.0);
			wl_list_insert(&_default_view->geometry.transformation_list, &_transform.link);
		}

		float x = floor(_wished_position.x + (_wished_position.w -
				_page_surface->width() * ratio)/2.0);
		float y = floor(_wished_position.y + (_wished_position.h -
				_page_surface->height() * ratio)/2.0);

		weston_view_set_position(_default_view, x, y);
		weston_view_schedule_repaint(_default_view);

	} else {
		_wished_position = _floating_wished_position;

		if(_transform.link.next)
			wl_list_remove(&_transform.link);

		weston_view_set_position(_default_view, _floating_wished_position.x,
				_floating_wished_position.y);
		weston_view_schedule_repaint(_default_view);

	}

}

void view_t::reconfigure() {

	if (is(MANAGED_NOTEBOOK) or is(MANAGED_FULLSCREEN)) {
		_wished_position = _notebook_wished_position;
	} else {
		_wished_position = _floating_wished_position;
	}

	//_xdg_surface->_ack_serial = wl_display_next_serial(_ctx->_dpy);

	set<uint32_t> state;

	if(is(MANAGED_NOTEBOOK)) {
		state.insert(XDG_SURFACE_STATE_MAXIMIZED);
	} else if(is(MANAGED_FULLSCREEN)) {
		state.insert(XDG_SURFACE_STATE_FULLSCREEN);
	}

	if(_has_keyboard_focus) {
		state.insert(XDG_SURFACE_STATE_ACTIVATED);
	}

	_page_surface->send_configure(_wished_position.w,
			_wished_position.h, state);

	update_view();

}


void view_t::set_managed_type(managed_window_type_e type)
{
	_managed_type = type;
	reconfigure();
}

rect view_t::get_base_position() const {
	return _wished_position;
}

managed_window_type_e view_t::get_type() {
	return _managed_type;
}

bool view_t::is(managed_window_type_e type) {
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

//bool view_t::is_fullscreen() {
//	return _managed_type == MANAGED_FULLSCREEN;
//}

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

void view_t::set_floating_wished_position(rect const & pos) {
	_floating_wished_position = pos;
}

void view_t::set_notebook_wished_position(rect const & pos) {
	_notebook_wished_position = pos;
}

rect const & view_t::get_wished_position() {
	return _wished_position;
}

rect const & view_t::get_floating_wished_position() {
	return _floating_wished_position;
}
//
//bool xdg_surface_toplevel_t::has_window(xcb_window_t w) const {
//	return false;
//}

string view_t::get_node_name() const {
	string s = _get_node_name<'T'>();
	ostringstream oss;
	oss << s << " " << (void*)_page_surface->surface() << " " << title();
	return oss.str();
}

//rect const & xdg_surface_toplevel_t::base_position() const {
//	return _wished_position;
//}
//
//rect const & xdg_surface_toplevel_t::orig_position() const {
//	return _wished_position;
//}

void view_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

}

void view_t::render_finished() {

}

void view_t::set_focus_state(bool is_focused) {
	weston_log("set_focus_state(%s) %p\n", is_focused?"true":"false", this);
	_has_keyboard_focus = is_focused;
	focus_change.signal(this);
}

void view_t::hide() {

//	for(auto x: _children) {
//		x->hide();
//	}
//
//	if(_default_view) {
//		weston_view_unmap(_default_view);
//		weston_view_destroy(_default_view);
//		_default_view = nullptr;
//		_ctx->sync_tree_view();
//	}

//	_is_visible = false;
}

void view_t::show() {
//	_is_visible = true;
//
//	if(not _default_view) {
//		_default_view = weston_view_create(_xdg_surface->_surface);
//		reconfigure();
//		weston_log("bbXX %p\n", _xdg_surface->_surface->compositor);
//		//_ctx->sync_tree_view();
//	}
//
////	for(auto x: _children) {
////		x->show();
////	}

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

void view_t::activate() {
	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}
	reconfigure();
	queue_redraw();
}

bool view_t::button(weston_pointer_grab * grab, uint32_t time, uint32_t button, uint32_t state) {
	auto pointer = grab->pointer;
	wl_fixed_t view_x;
	wl_fixed_t view_y;
	int view_ix;
	int view_iy;
	int ix = wl_fixed_to_int(pointer->x);
	int iy = wl_fixed_to_int(pointer->y);

	if (!pixman_region32_contains_point(
			&_default_view->transform.boundingbox, ix, iy, NULL))
		return false;

	weston_view_from_global_fixed(_default_view, pointer->x, pointer->y, &view_x, &view_y);
	view_ix = wl_fixed_to_int(view_x);
	view_iy = wl_fixed_to_int(view_y);

	if (!pixman_region32_contains_point(&_default_view->surface->input,
					    view_ix, view_iy, NULL))
		return false;

	if (_default_view->geometry.scissor_enabled &&
	    !pixman_region32_contains_point(&_default_view->geometry.scissor,
					    view_ix, view_iy, NULL))
		return false;

	if (pointer->button_count == 0 &&
			 state == WL_POINTER_BUTTON_STATE_RELEASED) {
		_ctx->set_keyboard_focus(pointer->seat, shared_from_this());
	}

	return true;

}

void view_t::queue_redraw() {

	if(_default_view) {
		weston_view_schedule_repaint(_default_view);
	}

	if(is(MANAGED_FLOATING)) {
		//_is_resized = true;
	} else {
		tree_t::queue_redraw();
	}
}

void view_t::trigger_redraw() {

}

void view_t::send_close() {
	_page_surface->send_close();
}

string const & view_t::title() const {
	return _page_surface->title();
}

bool view_t::has_focus() {
	return _has_keyboard_focus;
}

void view_t::weston_configure(struct weston_surface * es,
		int32_t sx, int32_t sy)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	/* IS NEVER CALLED */

}

void view_t::signal_title_change() {
	title_change.signal(this);
}

auto view_t::get_default_view() const -> weston_view * {
	return _default_view;
}

weston_surface * view_t::surface() const {
	return _page_surface->surface();
}

}
