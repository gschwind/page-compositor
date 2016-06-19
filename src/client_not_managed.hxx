/*
 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_NOT_MANAGED_HXX_
#define CLIENT_NOT_MANAGED_HXX_

#include <memory>
#include <libweston-0/compositor.h>
#include "xdg-shell-server-protocol.h"

#include "client_base.hxx"

#include "renderable.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"
#include "renderable_pixmap.hxx"

namespace page {

class xdg_surface_popup_t : public xdg_surface_base_t {
private:

	mutable rect _base_position;

	weston_seat * _seat;
	weston_surface * _partent;

	/* avoid copy */
	xdg_surface_popup_t(xdg_surface_popup_t const &);
	xdg_surface_popup_t & operator=(xdg_surface_popup_t const &);

	void _update_visible_region();
	void _update_opaque_region();

	static void _weston_configure(weston_surface * es, int32_t sx, int32_t sy);

public:

	/** called on surface commit */
	void weston_configure(weston_surface * es, int32_t sx, int32_t sy);


	xdg_surface_popup_t(page_context_t * ctx, wl_client * client,
			  wl_resource * resource,
			  uint32_t id,
			  weston_surface * surface,
			  weston_surface * parent,
			  weston_seat * seat,
			  uint32_t serial,
			  int32_t x, int32_t y);
	~xdg_surface_popup_t();

	/**
	 * tree_t virtual API
	 **/

	virtual void hide() { }
	virtual void show() { }
	// virtual auto get_node_name() const -> string;
	// virtual void remove(shared_ptr<tree_t> t);

	// virtual void children(vector<shared_ptr<tree_t>> & out) const;
//	virtual void update_layout(time64_t const time);
//	virtual void render(cairo_t * cr, region const & area);
//	virtual void render_finished();
//
//	virtual auto get_opaque_region() -> region;
//	virtual auto get_visible_region() -> region;
//	virtual auto get_damaged() -> region;

	static void xdg_popup_delete(wl_resource *resource);
	static xdg_surface_popup_t * get(wl_resource *resource);

	// virtual void activate();
	// virtual void activate(shared_ptr<tree_t> t);
	// virtual bool button_press(xcb_button_press_event_t const * ev);
	// virtual bool button_release(xcb_button_release_event_t const * ev);
	// virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	// virtual void expose(xcb_expose_event_t const * ev);
	// virtual void trigger_redraw();

	// virtual auto get_xid() const -> xcb_window_t;
	// virtual auto get_parent_xid() const -> xcb_window_t;
	// virtual rect get_window_position() const;
	// virtual void queue_redraw();

	/**
	 * client base API
	 **/

//	virtual bool has_window(xcb_window_t w) const;
//	virtual auto base() const -> xcb_window_t;
//	virtual auto orig() const -> xcb_window_t;
//	virtual auto base_position() const -> rect const &;
//	virtual auto orig_position() const -> rect const &;
//	virtual void on_property_notify(xcb_property_notify_event_t const * e);

};

}

#endif /* CLIENT_NOT_MANAGED_HXX_ */
