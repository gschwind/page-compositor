/*
 * grab_handlers.cxx
 *
 *  Created on: 24 juin 2015
 *      Author: gschwind
 */

#include <iostream>

#include "grab_handlers.hxx"
#include "page_context.hxx"
#include "view.hxx"

namespace page {

using namespace std;


grab_popup_t::grab_popup_t(page_context_t * ctx, surface_t * s) :
		_ctx{ctx},
		_surface{s}
{

}

grab_popup_t::~grab_popup_t() {

}

void grab_popup_t::focus() {
	auto pointer = base.grab.pointer;
	struct weston_view *view;
	wl_fixed_t sx, sy;

	if (pointer->button_count > 0)
		return;

	view = weston_compositor_pick_view(pointer->seat->compositor,
					   pointer->x, pointer->y,
					   &sx, &sy);

	if (pointer->focus != view || pointer->sx != sx || pointer->sy != sy)
		weston_pointer_set_focus(pointer, view, sx, sy);

}

void grab_popup_t::button(uint32_t time, uint32_t button, uint32_t state) {
	weston_pointer_send_button(base.grab.pointer, time, button, state);
}

void grab_popup_t::motion(uint32_t time, weston_pointer_motion_event *event) {
	weston_pointer_send_motion(base.grab.pointer, time, event);
}

void grab_popup_t::axis(uint32_t time, weston_pointer_axis_event *event) {
	weston_pointer_send_axis(base.grab.pointer, time, event);
}

void grab_popup_t::axis_source(uint32_t source) {
	weston_pointer_send_axis_source(base.grab.pointer, source);
}
void grab_popup_t::frame() {
	weston_pointer_send_frame(base.grab.pointer);
}

void grab_popup_t::cancel() {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_ctx->grab_stop(base.grab.pointer);
}


grab_split_t::grab_split_t(page_context_t * ctx, split_p s) :
		_ctx{ctx},
		_split{s}
{
	_slider_area = s->to_root_position(s->get_split_bar_area());
	_split_ratio = s->ratio();
	_split_root_allocation = s->root_location();
	//_ps = make_shared<popup_split_t>(ctx, s);
	//_ctx->overlay_add(_ps);
	//_ps->show();
}

grab_split_t::~grab_split_t() {
	//if(_ps != nullptr)
	//	_ctx->detach(_ps);
}

void grab_split_t::motion(uint32_t time, weston_pointer_motion_event * event) {
	auto pointer = base.grab.pointer;

	/** update pointer position **/
	weston_pointer_move(pointer, event);

	if(_split.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	/** current global position **/
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	if (_split.lock()->type() == VERTICAL_SPLIT) {
		_split_ratio = (x - _split_root_allocation.x) / _split_root_allocation.w;
	} else {
		_split_ratio = (y - _split_root_allocation.y) / _split_root_allocation.h;
	}

	_split_ratio = _split.lock()->compute_split_constaint(_split_ratio);

	//_ps->set_position(_split_ratio);
}

void grab_split_t::button(uint32_t time, uint32_t button, uint32_t state) {
	auto pointer = base.grab.pointer;

	if(_split.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	if (pointer->button_count == 0
			and state == WL_POINTER_BUTTON_STATE_RELEASED) {

		if (_split.lock()->type() == VERTICAL_SPLIT) {
			_split_ratio = (x - _split_root_allocation.x)
					/ (double) (_split_root_allocation.w);
		} else {
			_split_ratio = (y - _split_root_allocation.y)
					/ (double) (_split_root_allocation.h);
		}

		if (_split_ratio > 0.95)
			_split_ratio = 0.95;
		if (_split_ratio < 0.05)
			_split_ratio = 0.05;

		_split_ratio = _split.lock()->compute_split_constaint(_split_ratio);

		_split.lock()->queue_redraw();
		_split.lock()->set_split(_split_ratio);

		_ctx->sync_tree_view();
		_ctx->grab_stop(pointer);

	}

}

grab_bind_client_t::grab_bind_client_t(page_context_t * ctx,
		view_p c, uint32_t button,
		rect const & pos) :
		ctx{ctx},
		c{c},
		start_position{pos},
		target_notebook{},
		zone{NOTEBOOK_AREA_NONE},
		//pn0{},
		_button{button}
{


}

grab_bind_client_t::~grab_bind_client_t() {
	//if(pn0 != nullptr)
	//	ctx->detach(pn0);
}

void grab_bind_client_t::_find_target_notebook(int x, int y,
		notebook_p & target, notebook_area_e & zone) {

	target = nullptr;
	zone = NOTEBOOK_AREA_NONE;

	/* place the popup */
	auto ln = filter_class<notebook_t>(
			ctx->get_current_workspace()->get_all_children());
	for (auto i : ln) {
		if (i->_area.tab.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TAB;
			target = i;
			break;
		} else if (i->_area.right.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_RIGHT;
			target = i;
			break;
		} else if (i->_area.top.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TOP;
			target = i;
			break;
		} else if (i->_area.bottom.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_BOTTOM;
			target = i;
			break;
		} else if (i->_area.left.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_LEFT;
			target = i;
			break;
		} else if (i->_area.popup_center.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_CENTER;
			target = i;
			break;
		}
	}
}

void grab_bind_client_t::motion(uint32_t time,
		weston_pointer_motion_event * event)
{
	auto pointer = base.grab.pointer;

	/** update pointer position **/
	weston_pointer_move(pointer, event);

	/** current global position **/
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	/* do not start drag&drop for small move */
//	if (not start_position.is_inside(x, y) and pn0 == nullptr) {
//		pn0 = make_shared<popup_notebook0_t>(ctx);
//		ctx->overlay_add(pn0);
//		pn0->show();
//	}

//	if (pn0 == nullptr)
//		return;

	notebook_p new_target;
	notebook_area_e new_zone;
	_find_target_notebook(x, y, new_target, new_zone);

	if ((new_target != target_notebook.lock() or new_zone != zone)
			and new_zone != NOTEBOOK_AREA_NONE) {
		target_notebook = new_target;
		zone = new_zone;
		switch (zone) {
		case NOTEBOOK_AREA_TAB:
			//pn0->move_resize(new_target->_area.tab);
			break;
		case NOTEBOOK_AREA_RIGHT:
			//pn0->move_resize(new_target->_area.popup_right);
			break;
		case NOTEBOOK_AREA_TOP:
			//pn0->move_resize(new_target->_area.popup_top);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			//pn0->move_resize(new_target->_area.popup_bottom);
			break;
		case NOTEBOOK_AREA_LEFT:
			//pn0->move_resize(new_target->_area.popup_left);
			break;
		case NOTEBOOK_AREA_CENTER:
			//pn0->move_resize(new_target->_area.popup_center);
			break;
		}
	}
}

void grab_bind_client_t::button(uint32_t time, uint32_t button, uint32_t state)
{
	auto pointer = base.grab.pointer;

	if(c.expired()) {
		ctx->grab_stop(pointer);
		return;
	}

	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	auto c = this->c.lock();

	if (pointer->button_count == 0
			and state == WL_POINTER_BUTTON_STATE_RELEASED) {

		notebook_p new_target;
		notebook_area_e new_zone;
		_find_target_notebook(x, y, new_target, new_zone);

		/* if the mouse is no where, keep old location */
		if((new_target == nullptr or new_zone == NOTEBOOK_AREA_NONE) and not target_notebook.expired()) {
			new_zone = zone;
			new_target = target_notebook.lock();
		}

		if(new_target == nullptr or new_zone == NOTEBOOK_AREA_NONE
				or start_position.is_inside(x, y)) {
			if(c->is(MANAGED_FLOATING)) {
				ctx->detach(c);
				ctx->insert_window_in_notebook(c, nullptr);
			}

			ctx->set_keyboard_focus(pointer->seat, c);
			ctx->sync_tree_view();
			ctx->grab_stop(pointer);
			return;
		}

		switch(zone) {
		case NOTEBOOK_AREA_TAB:
		case NOTEBOOK_AREA_CENTER:
			if(new_target != c->parent()->shared_from_this()) {
				new_target->queue_redraw();
				c->queue_redraw();
				ctx->detach(c);
				ctx->insert_window_in_notebook(c, new_target);
				ctx->set_keyboard_focus(pointer->seat, c);
			}
			break;
		case NOTEBOOK_AREA_TOP:
			ctx->split_top(new_target, c);
			c->activate();
			//ctx->set_focus(c, time);
			break;
		case NOTEBOOK_AREA_LEFT:
			ctx->split_left(new_target, c);
			c->activate();
			//ctx->set_focus(c, time);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			ctx->split_bottom(new_target, c);
			c->activate();
			//ctx->set_focus(c, time);
			break;
		case NOTEBOOK_AREA_RIGHT:
			ctx->split_right(new_target, c);
			c->activate();
			//ctx->set_focus(c, time);
			break;
		default:
			if(c->parent() != nullptr and c->is(MANAGED_NOTEBOOK)) {
				if(ctx->conf()._enable_shade_windows) {
					if(c->is_visible()) {
						//ctx->set_focus(nullptr, time);
						dynamic_pointer_cast<notebook_t>(c->parent())->iconify_client(c);
					} else {
						cout << "activate = " << c << endl;
						c->activate();
						//ctx->set_focus(c, time);
					}
				} else {
					c->activate();
					//ctx->set_focus(c, time);
				}
			}
		}

		ctx->sync_tree_view();
		ctx->grab_stop(pointer);

	}
}


grab_floating_move_t::grab_floating_move_t(page_context_t * ctx,
		view_p f, uint32_t button, int x, int y) :
		_ctx{ctx},
		f{f},
		original_position{f->get_wished_position()},
		final_position{f->get_wished_position()},
		x_root{x},
		y_root{y},
		_button{button},
		popup_original_position{f->get_base_position()}
		//pfm{}
{

	//f->activate();
	//pfm = make_shared<popup_notebook0_t>(_ctx);
	//pfm->move_resize(popup_original_position);
	//_ctx->overlay_add(pfm);
	//pfm->show();
	//_ctx->dpy()->set_window_cursor(f->base(), _ctx->dpy()->xc_fleur);
	//_ctx->dpy()->set_window_cursor(f->orig(), _ctx->dpy()->xc_fleur);
}

grab_floating_move_t::~grab_floating_move_t() {
	//if (pfm != nullptr)
	//	_ctx->detach(pfm);
}


void grab_floating_move_t::motion(uint32_t time,
		weston_pointer_motion_event * event)
{
	auto pointer = base.grab.pointer;

	/** update pointer position **/
	weston_pointer_move(pointer, event);

	if(f.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	auto f = this->f.lock();

	/** current global position **/
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	weston_log("x = %f, y = %f\n", x, y);

	/* compute new window position */
	rect new_position = original_position;
	new_position.x += x - x_root;
	new_position.y += y - y_root;
	final_position = new_position;

	f->set_floating_wished_position(new_position);

	/* moving the window, does not need a buffer resize, thus we do not need
	 * reconfigure */
	f->update_view();

	rect new_popup_position = popup_original_position;
	new_popup_position.x += x - x_root;
	new_popup_position.y += y - y_root;
	//pfm->move_resize(new_popup_position);

}

void grab_floating_move_t::button(uint32_t time, uint32_t button,
		uint32_t state)
{
	auto pointer = base.grab.pointer;

	if (f.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	auto f = this->f.lock();

	if (pointer->button_count == 0
			and state == WL_POINTER_BUTTON_STATE_RELEASED) {

		//_ctx->dpy()->set_window_cursor(f->base(), XCB_NONE);
		//_ctx->dpy()->set_window_cursor(f->orig(), XCB_NONE);

		f->set_floating_wished_position(final_position);
		f->reconfigure();

		//_ctx->set_focus(f, time);
		_ctx->grab_stop(pointer);
	}
}


grab_floating_resize_t::grab_floating_resize_t(page_context_t * ctx,
		view_p f, uint32_t button,
		int x, int y, edge_e mode) :
		_ctx{ctx},
		f{f},
		mode{mode},
		x_root{x},
		y_root{y},
		original_position{f->get_wished_position()},
		final_position{f->get_wished_position()},
		_button{_button}

{

	f->activate();
	//pfm = make_shared<popup_notebook0_t>(_ctx);
	//pfm->move_resize(f->base_position());
	//_ctx->overlay_add(pfm);
	//pfm->show();

	//_ctx->dpy()->set_window_cursor(f->base(), _get_cursor());

}

grab_floating_resize_t::~grab_floating_resize_t() {
	//if(pfm != nullptr)
	//	_ctx->detach(pfm);
}

void grab_floating_resize_t::motion(uint32_t time,
		weston_pointer_motion_event * event)
{

	auto pointer = base.grab.pointer;

	if (f.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	auto f = this->f.lock();

	/** update pointer position **/
	weston_pointer_move(pointer, event);

	/** current global position **/
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	rect size = original_position;

	switch(mode) {
	case EDGE_NONE:
	case EDGE_TOP_LEFT:
		size.w -= x - x_root;
		size.h -= y - y_root;
		break;
	case EDGE_TOP:
		size.h -= y - y_root;
		break;
	case EDGE_TOP_RIGHT:
		size.w += x - x_root;
		size.h -= y - y_root;
		break;
	case EDGE_LEFT:
		size.w -= x - x_root;
		break;
	case EDGE_RIGHT:
		size.w += x - x_root;
		break;
	case EDGE_BOTTOM_LEFT:
		size.w -= x - x_root;
		size.h += y - y_root;
		break;
	case EDGE_BOTTOM:
		size.h += y - y_root;
		break;
	case EDGE_BOTTOM_RIGHT:
		size.w += x - x_root;
		size.h += y - y_root;
		break;
	}

	/* apply normal hints */
	//dimention_t<unsigned> final_size =
	//		f.lock()->compute_size_with_constrain(size.w, size.h);
	dimention_t<unsigned> final_size(size.w, size.h);
	size.w = final_size.width;
	size.h = final_size.height;

	if (size.h < 1)
		size.h = 1;
	if (size.w < 1)
		size.w = 1;

	/* do not allow to large windows */
//	if (size.w > _root_position.w - 100)
//		size.w = _root_position.w - 100;
//	if (size.h > _root_position.h - 100)
//		size.h = _root_position.h - 100;

	int x_diff = 0;
	int y_diff = 0;

	switch(mode) {
	case EDGE_NONE:
	case EDGE_TOP_LEFT:
		x_diff = original_position.w - size.w;
		y_diff = original_position.h - size.h;
		break;
	case EDGE_TOP:
		y_diff = original_position.h - size.h;
		break;
	case EDGE_TOP_RIGHT:
		y_diff = original_position.h - size.h;
		break;
	case EDGE_LEFT:
		x_diff = original_position.w - size.w;
		break;
	case EDGE_RIGHT:
		break;
	case EDGE_BOTTOM_LEFT:
		x_diff = original_position.w - size.w;
		break;
	case EDGE_BOTTOM:
		break;
	case EDGE_BOTTOM_RIGHT:
		break;
	}

	size.x += x_diff;
	size.y += y_diff;
	final_position = size;

//	f->set_floating_wished_position(final_position);
//	f->reconfigure();

//	rect popup_new_position = size;
//	if (false) {
//		popup_new_position.x -= _ctx->theme()->floating.margin.left;
//		popup_new_position.y -= _ctx->theme()->floating.margin.top;
//		popup_new_position.w += _ctx->theme()->floating.margin.left
//				+ _ctx->theme()->floating.margin.right;
//		popup_new_position.h += _ctx->theme()->floating.margin.top
//				+ _ctx->theme()->floating.margin.bottom;
//	}

	//pfm->move_resize(popup_new_position);

}

void grab_floating_resize_t::button(uint32_t time, uint32_t _button,
		uint32_t state)
{
	auto pointer = base.grab.pointer;

	if (f.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	auto f = this->f.lock();

	if (pointer->button_count == 0
			and state == WL_POINTER_BUTTON_STATE_RELEASED) {
		//_ctx->dpy()->set_window_cursor(f->base(), XCB_NONE);
		//_ctx->dpy()->set_window_cursor(f->orig(), XCB_NONE);
		f->set_floating_wished_position(final_position);
		f->reconfigure();
		//_ctx->set_focus(f, time);
		_ctx->grab_stop(pointer);
	}
}

grab_fullscreen_client_t::grab_fullscreen_client_t(page_context_t * ctx,
		view_p mw, uint32_t button, int x, int y) :
 _ctx{ctx},
 mw{mw},
 //pn0{nullptr},
 _button{button}
{
	v = _ctx->find_mouse_viewport(x, y);
//	pn0 = make_shared<popup_notebook0_t>(ctx);
//	pn0->move_resize(mw->base_position());
//	_ctx->overlay_add(pn0);
//	pn0->show();

}

grab_fullscreen_client_t::~grab_fullscreen_client_t() {
//	if (pn0 != nullptr)
//		_ctx->detach(pn0);
}

void grab_fullscreen_client_t::motion(uint32_t time,
		weston_pointer_motion_event * event) {
	auto pointer = base.grab.pointer;

	if (mw.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	/** update pointer position **/
	weston_pointer_move(pointer, event);

	/** current global position **/
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	viewport_p new_viewport = _ctx->find_mouse_viewport(x, y);

	if(new_viewport != v.lock()) {
		if(new_viewport != nullptr) {
			//pn0->move_resize(new_viewport->raw_area());
		}
		v = new_viewport;
	}

}

void grab_fullscreen_client_t::button(uint32_t time, uint32_t _button,
		uint32_t state) {
	auto pointer = base.grab.pointer;

	if(mw.expired()) {
		_ctx->grab_stop(pointer);
		return;
	}

	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	if (pointer->button_count == 0
			and state == WL_POINTER_BUTTON_STATE_RELEASED) {
		/** drop the fullscreen window to the new viewport **/

		auto new_viewport = _ctx->find_mouse_viewport(x, y);

		if(new_viewport != nullptr) {
			_ctx->fullscreen_client_to_viewport(mw.lock(), new_viewport);
		}

		_ctx->grab_stop(pointer);

	}
}

//void grab_alt_tab_t::_destroy_client(xdg_surface_toplevel_t * c) {
//	_destroy_func_map.erase(c);
//
//	_client_list.remove_if([](client_managed_w const & x) -> bool { return x.expired(); });
//
//	for(auto & x: _popup_list) {
//		x->destroy_client(c);
//	}
//}

//grab_alt_tab_t::grab_alt_tab_t(page_context_t * ctx, list<client_managed_p> managed_window, xcb_timestamp_t time) : _ctx{ctx} {
//	_client_list = weak(managed_window);
//
//	auto viewport_list = _ctx->get_current_workspace()->get_viewports();
//
//	for(auto v: viewport_list) {
//		auto pat = popup_alt_tab_t::create(_ctx, managed_window, v);
//		pat->show();
//		if(_client_list.size() > 0)
//			pat->selected(_client_list.front());
//		_ctx->overlay_add(pat);
//		_popup_list.push_back(pat);
//	}
//
//	if(_client_list.size() > 0) {
//		_selected = _client_list.front();
//	}
////
////	auto ck = xcb_grab_pointer(
////		_ctx->dpy()->xcb(),
////		false,
////		_ctx->dpy()->root(),
////		XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_BUTTON_MOTION|XCB_EVENT_MASK_POINTER_MOTION,
////		XCB_GRAB_MODE_ASYNC,
////		XCB_GRAB_MODE_ASYNC,
////		XCB_NONE,
////		XCB_NONE,
////		time);
////
////	xcb_generic_error_t * err;
////	auto reply = xcb_grab_pointer_reply(_ctx->dpy()->xcb(), ck, &err);
////
////	if(err != nullptr) {
////		cout << "grab_alt_tab_t::grab_alt_tab_t error while trying to grab : " << xcb_event_get_error_label(err->error_code) << endl;
////		xcb_discard_reply(_ctx->dpy()->xcb(), ck.sequence);
////	} else {
////		if(reply->status != XCB_GRAB_STATUS_SUCCESS) {
////			cout << "grab_alt_tab_t::grab_alt_tab_t: grab fail" << endl;
////		}
////		free(reply);
////	}
//
//}
//
//grab_alt_tab_t::~grab_alt_tab_t() {
//
//	for(auto x: _popup_list) {
//		_ctx->detach(x);
//	}
//
//	//xcb_ungrab_pointer(_ctx->dpy()->xcb(), XCB_CURRENT_TIME);
//}
//
//void grab_alt_tab_t::button_press(xcb_button_press_event_t const * e) {
//
//	if (e->detail == XCB_BUTTON_INDEX_1) {
//		for(auto & pat: _popup_list) {
//			pat->grab_button_press(e);
//			auto _mw = pat->selected();
//			if(not _mw.expired()) {
//				auto mw = _mw.lock();
//				mw->activate();
//				_ctx->set_focus(mw, e->time);
//				break;
//			}
//		}
//
//		//xcb_ungrab_keyboard(_ctx->dpy()->xcb(), e->time);
//		_ctx->grab_stop();
//
//	}
//}
//
//void grab_alt_tab_t::button_motion(xcb_motion_notify_event_t const * e) {
//	for(auto & pat: _popup_list) {
//		pat->grab_button_motion(e);
//		auto _mw = pat->selected();
//		if(not _mw.expired()) {
//			_selected = _mw;
//		}
//	}
//}
//
//void grab_alt_tab_t::key_press(xcb_key_press_event_t const * e) {
//	/* get KeyCode for Unmodified Key */
//	xcb_keysym_t k = _ctx->keymap()->get(e->detail);
//
//	if (k == 0)
//		return;
//
//	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
//	unsigned int state = e->state;
//	if(_ctx->keymap()->numlock_mod_mask() != 0) {
//		state &= ~(_ctx->keymap()->numlock_mod_mask());
//	}
//
//	if (k == XK_Tab and (state == XCB_MOD_MASK_1)) {
//		if(_selected.expired()) {
//			if(_client_list.size() > 0) {
//				_selected = _client_list.front();
//				for(auto & pat: _popup_list) {
//					pat->selected(_selected);
//				}
//			}
//		} else {
//			auto c = _selected.lock();
//			_selected.reset();
//			auto xc = std::find_if(_client_list.begin(), _client_list.end(),
//				[c](client_managed_w & x) -> bool { return c == x.lock(); });
//
//			if(xc != _client_list.end())
//				++xc;
//			if(xc != _client_list.end()) {
//				_selected = (*xc);
//			}
//
//			for(auto & pat: _popup_list) {
//				pat->selected(_selected);
//			}
//		}
//	}
//}
//
//void grab_alt_tab_t::key_release(xcb_key_release_event_t const * e) {
//	/* get KeyCode for Unmodified Key */
//	xcb_keysym_t k = _ctx->keymap()->get(e->detail);
//
//	if (k == 0)
//		return;
//
//	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
//	unsigned int state = e->state;
//	if(_ctx->keymap()->numlock_mod_mask() != 0) {
//		state &= ~(_ctx->keymap()->numlock_mod_mask());
//	}
//
//	if (XK_Escape == k) {
//		//xcb_ungrab_keyboard(_ctx->dpy()->xcb(), e->time);
//		_ctx->grab_stop();
//	}
//
//	/** here we guess Mod1 is bound to Alt **/
//	if (XK_Alt_L == k or XK_Alt_R == k) {
//		//xcb_ungrab_keyboard(_ctx->dpy()->xcb(), e->time);
//		if(not _selected.expired()) {
//			auto mw = _selected.lock();
//			mw->activate();
//			_ctx->set_focus(mw, e->time);
//		}
//		_ctx->grab_stop();
//	}
//
//}

}
