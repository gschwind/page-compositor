/*
 * managed_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "leak_checker.hxx"

#include <cairo.h>

#include "renderable_floating_outer_gradien.hxx"
#include "client_managed.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"

namespace page {

using namespace std;

void xdg_surface_toplevel_t::xdg_surface_delete(struct wl_resource *resource) {
	/* TODO */
}

xdg_surface_toplevel_t::xdg_surface_toplevel_t(page_context_t * ctx, wl_client * client,
		weston_surface * surface, uint32_t id) :
				xdg_surface_base_t{ctx, client, surface, id},
				_floating_wished_position{},
				_notebook_wished_position{},
				_wished_position{},
				_orig_position{},
				_base_position{},
				_surf{nullptr},
				_icon(nullptr),
				_has_focus{false},
				_is_iconic{true},
				_demands_attention{false},
				_default_view{nullptr}
{

	rect pos{0,0,surface->width, surface->height};

	printf("window default position = %s\n", pos.to_string().c_str());

	_managed_type = MANAGED_UNDEFINED;

	_floating_wished_position = pos;
	_notebook_wished_position = pos;
	_base_position = pos;
	_orig_position = pos;

	_apply_floating_hints_constraint();


	_resource = wl_resource_create(client, &xdg_surface_interface, 1, id);
	wl_resource_set_user_data(_resource, this);

	surface->configure = &xdg_surface_toplevel_t::_weston_configure;
	surface->configure_private = this;

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

	update_floating_areas();

	uint32_t cursor;

	//cursor = cnx()->xc_top_side;

	select_inputs_unsafe();

	update_icon();

}

xdg_surface_toplevel_t::~xdg_surface_toplevel_t() {

	_ctx->add_global_damage(get_visible_region());

	if(_default_view) {
		weston_layer_entry_remove(&_default_view->layer_link);
		weston_view_destroy(_default_view);
		_default_view = nullptr;
	}

	on_destroy.signal(this);

//	unselect_inputs_unsafe();
//
//	if (_surf != nullptr) {
//		warn(cairo_surface_get_reference_count(_surf) == 1);
//		cairo_surface_destroy(_surf);
//		_surf = nullptr;
//	}
//
//	destroy_back_buffer();

}

auto xdg_surface_toplevel_t::shared_from_this() -> shared_ptr<xdg_surface_toplevel_t> {
	return dynamic_pointer_cast<xdg_surface_toplevel_t>(tree_t::shared_from_this());
}

void xdg_surface_toplevel_t::reconfigure() {
	if(not _default_view)
		return;

	if (is(MANAGED_FLOATING)) {
		_wished_position = _floating_wished_position;

		if (prefer_window_border()) {
			_base_position.x = _wished_position.x
					- _ctx->theme()->floating.margin.left;
			_base_position.y = _wished_position.y - _ctx->theme()->floating.margin.top;
			_base_position.w = _wished_position.w + _ctx->theme()->floating.margin.left
					+ _ctx->theme()->floating.margin.right;
			_base_position.h = _wished_position.h + _ctx->theme()->floating.margin.top
					+ _ctx->theme()->floating.margin.bottom + _ctx->theme()->floating.title_height;

			_orig_position.x = _ctx->theme()->floating.margin.left;
			_orig_position.y = _ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;

		} else {
			/* floating window without borders */
			_base_position = _wished_position;

			_orig_position.x = 0;
			_orig_position.y = 0;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;

		}

		/* avoid to hide title bar of floating windows */
		if(_base_position.y < 0) {
			_base_position.y = 0;
		}

		//cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);

	} else {
		_wished_position = _notebook_wished_position;
		_base_position = _notebook_wished_position;
		_orig_position = rect(0, 0, _base_position.w, _base_position.h);
	}

	_is_resized = true;
	destroy_back_buffer();
	update_floating_areas();

	if(_is_iconic or not _is_visible) {

	} else {
		weston_view_set_position(_default_view, _base_position.x, _base_position.y);
		weston_view_geometry_dirty(_default_view);
	}

	_update_opaque_region();
	_update_visible_region();

}

void xdg_surface_toplevel_t::fake_configure_unsafe() {

}

void xdg_surface_toplevel_t::delete_window(xcb_timestamp_t t) {

}

void xdg_surface_toplevel_t::set_managed_type(managed_window_type_e type)
{
	_managed_type = type;
	reconfigure();
}

void xdg_surface_toplevel_t::focus(xcb_timestamp_t t) {
	icccm_focus_unsafe(t);
}

rect xdg_surface_toplevel_t::get_base_position() const {
	return _base_position;
}

managed_window_type_e xdg_surface_toplevel_t::get_type() {
	return _managed_type;
}

bool xdg_surface_toplevel_t::is(managed_window_type_e type) {
	return _managed_type == type;
}

void xdg_surface_toplevel_t::icccm_focus_unsafe(xcb_timestamp_t t) {


}

void xdg_surface_toplevel_t::compute_floating_areas() {

//	rect position{0, 0, _base_position.w, _base_position.h};
//
//	_floating_area.close_button = compute_floating_close_position(position);
//	_floating_area.bind_button = compute_floating_bind_position(position);
//
//	int x0 = _ctx->theme()->floating.margin.left;
//	int x1 = position.w - _ctx->theme()->floating.margin.right;
//
//	int y0 = _ctx->theme()->floating.margin.bottom;
//	int y1 = position.h - _ctx->theme()->floating.margin.bottom;
//
//	int w0 = position.w - _ctx->theme()->floating.margin.left
//			- _ctx->theme()->floating.margin.right;
//	int h0 = position.h - _ctx->theme()->floating.margin.bottom
//			- _ctx->theme()->floating.margin.bottom;
//
//	_floating_area.title_button = rect(x0, y0, w0, _ctx->theme()->floating.title_height);
//
//	_floating_area.grip_top = rect(x0, 0, w0, _ctx->theme()->floating.margin.top);
//	_floating_area.grip_bottom = rect(x0, y1, w0, _ctx->theme()->floating.margin.bottom);
//	_floating_area.grip_left = rect(0, y0, _ctx->theme()->floating.margin.left, h0);
//	_floating_area.grip_right = rect(x1, y0, _ctx->theme()->floating.margin.right, h0);
//	_floating_area.grip_top_left = rect(0, 0, _ctx->theme()->floating.margin.left, _ctx->theme()->floating.margin.top);
//	_floating_area.grip_top_right = rect(x1, 0, _ctx->theme()->floating.margin.right, _ctx->theme()->floating.margin.top);
//	_floating_area.grip_bottom_left = rect(0, y1, _ctx->theme()->floating.margin.left, _ctx->theme()->floating.margin.bottom);
//	_floating_area.grip_bottom_right = rect(x1, y1, _ctx->theme()->floating.margin.right, _ctx->theme()->floating.margin.bottom);

}

rect xdg_surface_toplevel_t::compute_floating_close_position(rect const & allocation) const {

	rect position;
	position.x = allocation.w - _ctx->theme()->floating.close_width;
	position.y = 0.0;
	position.w = _ctx->theme()->floating.close_width;
	position.h = _ctx->theme()->floating.title_height;

	return position;
}

rect xdg_surface_toplevel_t::compute_floating_bind_position(
		rect const & allocation) const
{

	rect position;
	position.x = allocation.w - _ctx->theme()->floating.bind_width
			- _ctx->theme()->floating.close_width;
	position.y = 0.0;
	position.w = _ctx->theme()->floating.bind_width;
	position.h = _ctx->theme()->floating.title_height;

	return position;
}

/**
 * set usual passive button grab for a focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void xdg_surface_toplevel_t::grab_button_focused_unsafe() {


}

/**
 * set usual passive button grab for a not focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void xdg_surface_toplevel_t::grab_button_unfocused_unsafe() {

}


/**
 * Remove all passive grab on windows
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void xdg_surface_toplevel_t::ungrab_all_button_unsafe() {

}

/**
 * select usual input events
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void xdg_surface_toplevel_t::select_inputs_unsafe() {

}

/**
 * Remove all selected input event
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void xdg_surface_toplevel_t::unselect_inputs_unsafe() {

}

bool xdg_surface_toplevel_t::is_fullscreen() {
	return _managed_type == MANAGED_FULLSCREEN;
}

bool xdg_surface_toplevel_t::skip_task_bar() {
	return false;
}

bool xdg_surface_toplevel_t::get_wm_normal_hints(XSizeHints * size_hints) {
	return false;
}

void xdg_surface_toplevel_t::net_wm_state_add(atom_e atom) {
	/* TODO: configure */
}

void xdg_surface_toplevel_t::net_wm_state_remove(atom_e atom) {
	/* TODO: configure */
}

void xdg_surface_toplevel_t::net_wm_state_delete() {

}

void xdg_surface_toplevel_t::normalize() {
	if(not _is_iconic)
		return;
	_is_iconic = false;
}

void xdg_surface_toplevel_t::iconify() {
	if(_is_iconic)
		return;
	_is_iconic = true;
}

void xdg_surface_toplevel_t::wm_state_delete() {
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/

	//_client_proxy->delete_wm_state();
}

void xdg_surface_toplevel_t::set_floating_wished_position(rect const & pos) {
	_floating_wished_position = pos;
}

void xdg_surface_toplevel_t::set_notebook_wished_position(rect const & pos) {
	_notebook_wished_position = pos;
}

rect const & xdg_surface_toplevel_t::get_wished_position() {
	return _wished_position;
}

rect const & xdg_surface_toplevel_t::get_floating_wished_position() {
	return _floating_wished_position;
}

void xdg_surface_toplevel_t::destroy_back_buffer() {

}

void xdg_surface_toplevel_t::create_back_buffer() {

//	if (not is(MANAGED_FLOATING) or not prefer_window_border()) {
//		destroy_back_buffer();
//		return;
//	}
//
//	{
//		int w = _base_position.w;
//		int h = _ctx->theme()->floating.margin.top
//				+ _ctx->theme()->floating.title_height;
//		if(w > 0 and h > 0) {
//			_top_buffer = make_shared<pixmap_t>(PIXMAP_RGBA, w, h);
//		}
//	}
//
//	{
//		int w = _base_position.w;
//		int h = _ctx->theme()->floating.margin.bottom;
//		if(w > 0 and h > 0) {
//			_bottom_buffer =
//					make_shared<pixmap_t>(PIXMAP_RGBA, w, h);
//		}
//	}
//
//	{
//		int w = _ctx->theme()->floating.margin.left;
//		int h = _base_position.h - _ctx->theme()->floating.margin.top
//				- _ctx->theme()->floating.margin.bottom;
//		if(w > 0 and h > 0) {
//			_left_buffer =
//					make_shared<pixmap_t>(PIXMAP_RGBA, w, h);
//		}
//	}
//
//	{
//		int w = _ctx->theme()->floating.margin.right;
//		int h = _base_position.h - _ctx->theme()->floating.margin.top
//				- _ctx->theme()->floating.margin.bottom;
//		if(w > 0 and h > 0) {
//			_right_buffer =
//					make_shared<pixmap_t>(PIXMAP_RGBA, w, h);
//		}
//	}
}

void xdg_surface_toplevel_t::update_floating_areas() {
//	theme_managed_window_t tm;
//	tm.position = _base_position;
//	tm.title = _title;
//
//	int x0 = _ctx->theme()->floating.margin.left;
//	int x1 = _base_position.w - _ctx->theme()->floating.margin.right;
//
//	int y0 = _ctx->theme()->floating.margin.bottom;
//	int y1 = _base_position.h - _ctx->theme()->floating.margin.bottom;
//
//	int w0 = _base_position.w - _ctx->theme()->floating.margin.left
//			- _ctx->theme()->floating.margin.right;
//	int h0 = _base_position.h - _ctx->theme()->floating.margin.bottom
//			- _ctx->theme()->floating.margin.bottom;
//
//	_area_top = rect(x0, 0, w0, _ctx->theme()->floating.margin.bottom);
//	_area_bottom = rect(x0, y1, w0, _ctx->theme()->floating.margin.bottom);
//	_area_left = rect(0, y0, _ctx->theme()->floating.margin.left, h0);
//	_area_right = rect(x1, y0, _ctx->theme()->floating.margin.right, h0);
//
//	_area_top_left = rect(0, 0, _ctx->theme()->floating.margin.left,
//			_ctx->theme()->floating.margin.bottom);
//	_area_top_right = rect(x1, 0, _ctx->theme()->floating.margin.right,
//			_ctx->theme()->floating.margin.bottom);
//	_area_bottom_left = rect(0, y1, _ctx->theme()->floating.margin.left,
//			_ctx->theme()->floating.margin.bottom);
//	_area_bottom_right = rect(x1, y1, _ctx->theme()->floating.margin.right,
//			_ctx->theme()->floating.margin.bottom);
//
//	compute_floating_areas();

}

bool xdg_surface_toplevel_t::has_window(xcb_window_t w) const {
	return false;
}

string xdg_surface_toplevel_t::get_node_name() const {
	string s = _get_node_name<'M'>();
	ostringstream oss;
	oss << s << " " << orig() << " " << title();
	return oss.str();
}

//display_t * xdg_surface_toplevel_t::cnx() {
//	return _client_proxy->cnx();
//}

rect const & xdg_surface_toplevel_t::base_position() const {
	return _base_position;
}

rect const & xdg_surface_toplevel_t::orig_position() const {
	return _orig_position;
}

void xdg_surface_toplevel_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

//	/** update damage_cache **/
//	region dmg = _default_view->get_damaged();
//	dmg.translate(_base_position.x, _base_position.y);
//	_damage_cache += dmg;
//	_default_view->clear_damaged();

}

void xdg_surface_toplevel_t::render_finished() {

}


void xdg_surface_toplevel_t::set_focus_state(bool is_focused) {
	_has_change = true;

	_has_focus = is_focused;
	if (_has_focus) {
		net_wm_state_add(_NET_WM_STATE_FOCUSED);
	} else {
		net_wm_state_remove(_NET_WM_STATE_FOCUSED);
	}

	on_focus_change.signal(shared_from_this());
	queue_redraw();
}

void xdg_surface_toplevel_t::map_unsafe() {

}

void xdg_surface_toplevel_t::unmap_unsafe() {

}

void xdg_surface_toplevel_t::hide() {
	for(auto x: _children) {
		x->hide();
	}

	_ctx->add_global_damage(get_visible_region());

	_is_visible = false;
	// do not unmap, just put it outside the screen.
	//unmap();
	reconfigure();

	/* we no not need the view anymore */
	_default_view = nullptr;

}

void xdg_surface_toplevel_t::show() {
	_is_visible = true;
	reconfigure();
	map_unsafe();
	for(auto x: _children) {
		x->show();
	}

	if(_default_view == nullptr)
		_default_view = create_view();
}

bool xdg_surface_toplevel_t::is_iconic() {
	return _is_iconic;
}

bool xdg_surface_toplevel_t::is_stiky() {
	return false;
}

bool xdg_surface_toplevel_t::is_modal() {
	return false;
}

void xdg_surface_toplevel_t::activate() {

	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}

	if(is_iconic()) {
		normalize();
		queue_redraw();
	}

}

//bool xdg_surface_toplevel_t::button_press(xcb_button_press_event_t const * e) {
//
//	if (not has_window(e->event)) {
//		return false;
//	}
//
//	if (is(MANAGED_FLOATING)
//			and e->detail == XCB_BUTTON_INDEX_1
//			and (e->state & XCB_MOD_MASK_1)) {
//		_ctx->grab_start(new grab_floating_move_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
//		return true;
//	} else if (is(MANAGED_FLOATING)
//			and e->detail == XCB_BUTTON_INDEX_3
//			and (e->state & XCB_MOD_MASK_1)) {
//		_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_RIGHT});
//		return true;
//	} else if (is(MANAGED_FLOATING)
//			and e->detail == XCB_BUTTON_INDEX_1
//			and e->child != orig()
//			and e->event == deco()) {
//
//		if (_floating_area.close_button.is_inside(e->event_x, e->event_y)) {
//			delete_window(e->time);
//		} else if (_floating_area.bind_button.is_inside(e->event_x, e->event_y)) {
//			rect absolute_position = _floating_area.bind_button;
//			absolute_position.x += base_position().x;
//			absolute_position.y += base_position().y;
//			_ctx->grab_start(new grab_bind_client_t{_ctx, shared_from_this(), e->detail, absolute_position});
//		} else if (_floating_area.title_button.is_inside(e->event_x, e->event_y)) {
//			_ctx->grab_start(new grab_floating_move_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
//		} else {
//			if (_floating_area.grip_top.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_TOP});
//			} else if (_floating_area.grip_bottom.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM});
//			} else if (_floating_area.grip_left.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_LEFT});
//			} else if (_floating_area.grip_right.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_RIGHT});
//			} else if (_floating_area.grip_top_left.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_TOP_LEFT});
//			} else if (_floating_area.grip_top_right.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_TOP_RIGHT});
//			} else if (_floating_area.grip_bottom_left.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_LEFT});
//			} else if (_floating_area.grip_bottom_right.is_inside(e->event_x, e->event_y)) {
//				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_RIGHT});
//			} else {
//				_ctx->grab_start(new grab_floating_move_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
//			}
//		}
//
//		return true;
//
//	} else if (is(MANAGED_FULLSCREEN)
//			and e->detail == (XCB_BUTTON_INDEX_1)
//			and (e->state & XCB_MOD_MASK_1)) {
//		/** start moving fullscreen window **/
//		_ctx->grab_start(new grab_fullscreen_client_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
//		return true;
//	} else if (is(MANAGED_NOTEBOOK) and e->detail == (XCB_BUTTON_INDEX_3)
//			and (e->state & (XCB_MOD_MASK_1))) {
//		_ctx->grab_start(new grab_bind_client_t{_ctx, shared_from_this(), e->detail, rect{e->root_x-10, e->root_y-10, 20, 20}});
//		return true;
//	}
//
//	return false;
//}

void xdg_surface_toplevel_t::queue_redraw() {
	if(is(MANAGED_FLOATING)) {
		_is_resized = true;
	} else {
		tree_t::queue_redraw();
	}
}

void xdg_surface_toplevel_t::_update_backbuffers() {
//	if(not is(MANAGED_FLOATING) or not prefer_window_border())
//		return;
//
//	theme_managed_window_t fw;
//
//	if (_bottom_buffer != nullptr) {
//		fw.cairo_bottom = cairo_create(_bottom_buffer->get_cairo_surface());
//	} else {
//		fw.cairo_bottom = nullptr;
//	}
//
//	if (_top_buffer != nullptr) {
//		fw.cairo_top = cairo_create(_top_buffer->get_cairo_surface());
//	} else {
//		fw.cairo_top = nullptr;
//	}
//
//	if (_right_buffer != nullptr) {
//		fw.cairo_right = cairo_create(_right_buffer->get_cairo_surface());
//	} else {
//		fw.cairo_right = nullptr;
//	}
//
//	if (_left_buffer != nullptr) {
//		fw.cairo_left = cairo_create(_left_buffer->get_cairo_surface());
//	} else {
//		fw.cairo_left = nullptr;
//	}
//
//	fw.focuced = has_focus();
//	fw.position = base_position();
//	fw.icon = icon();
//	fw.title = title();
//	fw.demand_attention = _demands_attention;
//
//	_ctx->theme()->render_floating(&fw);
}

void xdg_surface_toplevel_t::trigger_redraw() {

	if(_is_resized) {
		_is_resized = false;
		_has_change = true;
		create_back_buffer();
	}

	if(_has_change) {
		_has_change = false;
		_is_exposed = true;
		_update_backbuffers();
	}

	if(_is_exposed) {
		_is_exposed = false;
		_paint_exposed();
	}

}

void xdg_surface_toplevel_t::set_title(char const * title) {
	_has_change = true;
	_pending.title = string{title};
}

void xdg_surface_toplevel_t::update_title() {

}

bool xdg_surface_toplevel_t::prefer_window_border() const {
	return false;
}

shared_ptr<icon16> xdg_surface_toplevel_t::icon() const {
	return _icon;
}

void xdg_surface_toplevel_t::update_icon() {
	_icon = make_shared<icon16>(this);
}

bool xdg_surface_toplevel_t::has_focus() const {
	return _has_focus;
}

void xdg_surface_toplevel_t::set_current_desktop(unsigned int n) {

}

void xdg_surface_toplevel_t::set_demands_attention(bool x) {
	_demands_attention = x;
}

bool xdg_surface_toplevel_t::demands_attention() {
	return _demands_attention;
}

string const & xdg_surface_toplevel_t::title() const {
	return _title;
}

void xdg_surface_toplevel_t::render(cairo_t * cr, region const & area) {

}

void xdg_surface_toplevel_t::_update_visible_region() {

}

void xdg_surface_toplevel_t::_update_opaque_region() {

}

void xdg_surface_toplevel_t::on_property_notify(xcb_property_notify_event_t const * e) {
//	if (e->atom == A(_NET_WM_NAME) or e->atom == A(WM_NAME)) {
//		update_title();
//		queue_redraw();
//	} else if (e->atom == A(_NET_WM_ICON)) {
//		update_icon();
//		queue_redraw();
//	} if (e->atom == A(_NET_WM_OPAQUE_REGION)) {
//		_update_opaque_region();
//	}

}

void xdg_surface_toplevel_t::_apply_floating_hints_constraint() {

//	if(_client_proxy->wm_normal_hints()!= nullptr) {
//		XSizeHints const * s = _client_proxy->wm_normal_hints();
//
//		if (s->flags & PBaseSize) {
//			if (_floating_wished_position.w < s->base_width)
//				_floating_wished_position.w = s->base_width;
//			if (_floating_wished_position.h < s->base_height)
//				_floating_wished_position.h = s->base_height;
//		} else if (s->flags & PMinSize) {
//			if (_floating_wished_position.w < s->min_width)
//				_floating_wished_position.w = s->min_width;
//			if (_floating_wished_position.h < s->min_height)
//				_floating_wished_position.h = s->min_height;
//		}
//
//		if (s->flags & PMaxSize) {
//			if (_floating_wished_position.w > s->max_width)
//				_floating_wished_position.w = s->max_width;
//			if (_floating_wished_position.h > s->max_height)
//				_floating_wished_position.h = s->max_height;
//		}
//
//	}
}

//void xdg_surface_toplevel_t::expose(xcb_expose_event_t const * ev) {
//	_is_exposed = true;
//}

void xdg_surface_toplevel_t::_paint_exposed() {
//	if (is(MANAGED_FLOATING) and prefer_window_border()) {
//
//		cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);
//
//		cairo_t * _cr = cairo_create(_surf);
//
//		/** top **/
//		if (_top_buffer != nullptr) {
//			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
//			cairo_rectangle(_cr, 0, 0, _base_position.w,
//					_ctx->theme()->floating.margin.top+_ctx->theme()->floating.title_height);
//			cairo_set_source_surface(_cr, _top_buffer->get_cairo_surface(), 0, 0);
//			cairo_fill(_cr);
//		}
//
//		/** bottom **/
//		if (_bottom_buffer != nullptr) {
//			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
//			cairo_rectangle(_cr, 0,
//					_base_position.h - _ctx->theme()->floating.margin.bottom,
//					_base_position.w, _ctx->theme()->floating.margin.bottom);
//			cairo_set_source_surface(_cr, _bottom_buffer->get_cairo_surface(), 0,
//					_base_position.h - _ctx->theme()->floating.margin.bottom);
//			cairo_fill(_cr);
//		}
//
//		/** left **/
//		if (_left_buffer != nullptr) {
//			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
//			cairo_rectangle(_cr, 0.0, _ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height,
//					_ctx->theme()->floating.margin.left,
//					_base_position.h - _ctx->theme()->floating.margin.top
//							- _ctx->theme()->floating.margin.bottom - _ctx->theme()->floating.title_height);
//			cairo_set_source_surface(_cr, _left_buffer->get_cairo_surface(), 0.0,
//					_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height);
//			cairo_fill(_cr);
//		}
//
//		/** right **/
//		if (_right_buffer != nullptr) {
//			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
//			cairo_rectangle(_cr,
//					_base_position.w - _ctx->theme()->floating.margin.right,
//					_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height, _ctx->theme()->floating.margin.right,
//					_base_position.h - _ctx->theme()->floating.margin.top
//							- _ctx->theme()->floating.margin.bottom - _ctx->theme()->floating.title_height);
//			cairo_set_source_surface(_cr, _right_buffer->get_cairo_surface(),
//					_base_position.w - _ctx->theme()->floating.margin.right,
//					_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height);
//			cairo_fill(_cr);
//		}
//
//		cairo_surface_flush(_surf);
//
//		warn(cairo_get_reference_count(_cr) == 1);
//		cairo_destroy(_cr);
//	}
}

void xdg_surface_toplevel_t::_weston_configure(struct weston_surface * es,
		int32_t sx, int32_t sy)
{
	auto ths = reinterpret_cast<xdg_surface_toplevel_t *>(es->configure_private);

	ths->on_configure.signal(ths->shared_from_this(), sx, sy);

	/* once configure is finished apply pending states */
	ths->_title = ths->_pending.title;
	ths->_transient_for = ths->_pending.transient_for;

}

void xdg_surface_toplevel_t::set_transient_for(xdg_surface_toplevel_t * s) {
	_pending.transient_for = s;
}

auto xdg_surface_toplevel_t::transient_for() const -> xdg_surface_toplevel_t * {
	return _transient_for;
}

auto xdg_surface_toplevel_t::get(wl_resource * r) -> xdg_surface_toplevel_t * {
	return reinterpret_cast<xdg_surface_toplevel_t*>(wl_resource_get_user_data(r));
}

void xdg_surface_toplevel_t::set_maximized() {
	_pending.maximize = true;
}

void xdg_surface_toplevel_t::unset_maximized() {
	_pending.maximize = false;
}

void xdg_surface_toplevel_t::set_fullscreen() {
	_pending.fullscreen = true;
}

void xdg_surface_toplevel_t::unset_fullscreen() {
	_pending.fullscreen = false;
}

void xdg_surface_toplevel_t::set_minimized() {
	_pending.minimize = true;
}

void xdg_surface_toplevel_t::set_window_geometry(int32_t x, int32_t y, int32_t w, int32_t h) {
	_pending.geometry = rect(x, y, w, h);
}

}

