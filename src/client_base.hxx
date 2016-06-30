/*
 * client.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 * client_base store/cache all client windows properties define in ICCCM or EWMH.
 *
 * most of them are store with pointer which is null if the properties is not set on client.
 *
 */

#ifndef CLIENT_BASE_HXX_
#define CLIENT_BASE_HXX_

#include <cassert>
#include <memory>

#include "utils.hxx"
#include "region.hxx"

#include "exception.hxx"

#include "tree.hxx"
#include "page_context.hxx"

namespace page {

using namespace std;

using card32 = long;


/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
class xdg_surface_base_t : public tree_t {
protected:
	page_context_t * _ctx;

	/* weston surface */
	weston_surface * _surface;

	/* client that own the xdg_surface */
	wl_client * _client;

	/* the wayland resource create for this xdg_surface */
	wl_resource * _xdg_surface_resource;

	/* weston view related to this xdg_surface */
	list<weston_view *> _views;

	weston_view * _default_view;

public:

	xdg_surface_base_t(xdg_surface_base_t const & c) = delete;

	xdg_surface_base_t(page_context_t * ctx, wl_client * client,
			weston_surface * surface, uint32_t id);

	virtual ~xdg_surface_base_t();

	bool has_motif_border();

	auto surface() const -> weston_surface * { return _surface; }

	void add_subclient(shared_ptr<xdg_surface_base_t> s);

	bool is_window(wl_client * client, uint32_t id);

	uint32_t wm_type();

	void print_window_attributes();
	void print_properties();

//	auto wa() const -> xcb_get_window_attributes_reply_t const * ;
//	auto geometry() const -> xcb_get_geometry_reply_t const *;

//	auto shape() const -> region const *;
//	auto position() -> rect;

	weston_view * create_view();

	/* find the bigger window that is smaller than w and h */
	dimention_t<unsigned> compute_size_with_constrain(unsigned w, unsigned h);

	static void _weston_configure(weston_surface * es, int32_t sx, int32_t sy);

	/** called on surface commit */
	virtual void weston_configure(weston_surface * es, int32_t sx,
			int32_t sy) = 0;

	virtual auto get_default_view() const -> weston_view *;

	/**
	 * tree_t virtual API
	 **/

	using tree_t::append_children;

	// virtual void hide();
	// virtual void show();
	virtual auto get_node_name() const -> string;
	virtual void remove(shared_ptr<tree_t> t);

	virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	// virtual void update_layout(time64_t const time);
	// virtual void render(cairo_t * cr, region const & area);

	// virtual auto get_opaque_region() -> region;
	// virtual auto get_visible_region() -> region;
	// virtual auto get_damaged() -> region;

	using tree_t::activate; // virtual void activate();
	virtual void activate(shared_ptr<tree_t> t);
	// virtual bool button_press(xcb_button_press_event_t const * ev);
	// virtual bool button_release(xcb_button_release_event_t const * ev);
	// virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	// virtual void expose(xcb_expose_event_t const * ev);
	// virtual void trigger_redraw();

//	virtual auto get_xid() const -> uint32_t;
//	virtual auto get_parent_xid() const -> uint32_t;
	// virtual rect get_window_position() const;
	// virtual void queue_redraw();

	/**
	 * client base API
	 **/

//	virtual bool has_window(uint32_t w) const = 0;
//	virtual auto base() const -> uint32_t = 0;
//	virtual auto orig() const -> uint32_t = 0;
//	virtual auto base_position() const -> rect const & = 0;
//	virtual auto orig_position() const -> rect const & = 0;

	virtual void set_output(weston_output * output);

};

}

#endif /* CLIENT_HXX_ */
