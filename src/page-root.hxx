/*
 * Copyright (2015-2016) Benoit Gschwind
 *
 * This file is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SRC_PAGE_ROOT_HXX_
#define SRC_PAGE_ROOT_HXX_

#include "page-component.hxx"
#include "page-context.hxx"

class page_root_t : public tree_t {
	friend class compositor_t;


	page_context_t * _ctx;

	rect _root_position;

	unsigned int _current_desktop;
	vector<shared_ptr<workspace_t>> _desktop_list;

	shared_ptr<tree_t> below;
	shared_ptr<tree_t> root_subclients;
	shared_ptr<tree_t> tooltips;
	shared_ptr<tree_t> notifications;
	shared_ptr<tree_t> above;

	//shared_ptr<compositor_overlay_t> _fps_overlay;
	/** store the order of last shown desktop **/
	shared_ptr<tree_t> _desktop_stack;

	shared_ptr<tree_t> _overlays;

public:

	page_root_t(page_context_t * ctx);
	~page_root_t();

	/**
	 * tree_t virtual API
	 **/

	//virtual void hide();
	//virtual void show();
	virtual auto get_node_name() const -> string;
	//virtual void remove(shared_ptr<tree_t> t);

	//virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	//virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area);

	virtual auto get_opaque_region() -> region;
	virtual auto get_visible_region() -> region;
	virtual auto get_damaged() -> region;

	virtual void activate();
	virtual void activate(shared_ptr<tree_t> t);
	//virtual bool button_press(xcb_button_press_event_t const * ev);
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	//virtual void expose(xcb_expose_event_t const * ev);
	//virtual void trigger_redraw();

	//virtual auto get_xid() const -> xcb_window_t;
	//virtual auto get_parent_xid() const -> xcb_window_t;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

	/**
	 * page_component_t virtual API
	 **/

//	virtual void set_allocation(rect const & area);
//	virtual rect allocation() const;
//	virtual void replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by);


};




#endif /* SRC_PAGE_ROOT_HXX_ */
