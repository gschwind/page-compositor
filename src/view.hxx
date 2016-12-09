/*
 * xdg_surface_toplevel_view.hxx
 *
 * copyright (2016) Benoit Gschwind
 *
 */

#ifndef SRC_XDG_SURFACE_TOPLEVEL_VIEW_HXX_
#define SRC_XDG_SURFACE_TOPLEVEL_VIEW_HXX_

#include "tree.hxx"
#include "page_context.hxx"
#include "page-surface-interface.hxx"
#include "listener.hxx"

namespace page {

class page_t;

using namespace std;

enum managed_window_type_e {
	MANAGED_UNCONFIGURED,
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN,
	MANAGED_DOCK,
	MANAGED_POPUP
};

class view_t : public tree_t {

	friend class page_t;
	page_context_t * _ctx;
	page_surface_interface * _page_surface;

	/** hold floating position **/
	rect _floating_wished_position;

	/** hold notebook position **/
	rect _notebook_wished_position;

	/** -- **/
	rect _wished_position;

	weston_view * _default_view;

	shared_ptr<tree_t> _transient_childdren;
	shared_ptr<tree_t> _popups_childdren;

	weston_transform _transform;

	/* handle the state of management: notebook, floating, fullscreen */
	managed_window_type_e _managed_type;

	/* private to avoid copy */
	view_t(view_t const &) = delete;
	view_t & operator=(view_t const &) = delete;

	void set_wished_position(rect const & position);
	rect const & get_wished_position() const;

	void set_theme(theme_t const * theme);

	static void xdg_surface_delete(wl_resource *resource);

	bool _has_change;
	bool _has_keyboard_focus;

	listener_t<struct weston_view> weston_view_destroy_listener;
	void weston_view_destroyed(struct weston_view * v);

public:

	signal_t<view_t*> destroy;
	signal_t<view_t*> focus_change;
	signal_t<view_t*> title_change;

	void add_transient_child(view_p c);
	virtual void add_popup_child(view_p child, int x, int y);

	void signal_title_change();

	weston_surface * surface() const;

	void destroy_popup_child(view_t * c);

	auto shared_from_this() -> shared_ptr<view_t>;

	/** called on surface commit */
	void weston_configure(weston_surface * es, int32_t sx, int32_t sy);

	view_t(page_context_t * ctx, page_surface_interface * s);
	virtual ~view_t();

	/* read only attributes */
	auto resource() const -> wl_resource * __attribute__((deprecated));
	auto title() const -> string const &;
	bool has_focus();

	void update_view(); // send configure request.

	bool is(managed_window_type_e type);
	auto get_wished_position() -> rect const &;
	void set_floating_wished_position(rect const & pos);
	rect get_base_position() const;
	void reconfigure();
//	void normalize();
//	void iconify();
//	bool has_focus() const;
//	bool is_iconic();
	//void delete_window(xcb_timestamp_t);
//	auto icon() const -> shared_ptr<icon16>;
	void set_notebook_wished_position(rect const & pos);
	//void set_current_desktop(unsigned int n);
	//bool is_stiky();
	//bool is_modal();
	//void net_wm_state_add(atom_e atom);
	//void net_wm_state_remove(atom_e atom);
	//void net_wm_state_delete();
	//void wm_state_delete();
	bool is_fullscreen();
	//bool skip_task_bar();
	auto get_floating_wished_position() -> rect const & ;
	//bool lock();
	//void unlock();
	void set_focus_state(bool is_focused);
	//void set_demands_attention(bool x);
	//bool demands_attention();
	//void focus(xcb_timestamp_t t);
	auto get_type() -> managed_window_type_e;
	void set_managed_type(managed_window_type_e type);
	//void grab_button_focused_unsafe();
	//void grab_button_unfocused_unsafe();

	void signal_destroy();

	void send_close();

	void set_maximized();
	void unset_maximized();
	void set_fullscreen();
	void unset_fullscreen();
	void set_minimized();

	void set_window_geometry(int32_t x, int32_t y, int32_t w, int32_t h);

	/**
	 * tree_t virtual API
	 **/

	virtual void hide();
	virtual void show();
	virtual auto get_node_name() const -> string;
	// virtual void remove(shared_ptr<tree_t> t);

	// virtual void children(vector<shared_ptr<tree_t>> & out) const;
	virtual void update_layout(time64_t const time);
	//virtual void render(cairo_t * cr, region const & area);
	virtual void render_finished();

	//virtual auto get_opaque_region() -> region;
	//virtual auto get_visible_region() -> region;
	//virtual auto get_damaged() -> region;

	virtual void activate();
	//virtual void activate(shared_ptr<tree_t> t);
	virtual bool button(weston_pointer_grab * grab, uint32_t time, uint32_t button, uint32_t state);
	// virtual bool motion(weston_pointer_grab * grab, uint32_t time, weston_pointer_motion_event * event);

	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	// virtual void expose(xcb_expose_event_t const * ev);

	// virtual auto get_xid() const -> xcb_window_t;
	// virtual auto get_parent_xid() const -> xcb_window_t;
	// virtual rect get_window_position() const;

	virtual void queue_redraw();
	virtual void trigger_redraw();
	virtual auto get_default_view() const -> weston_view *;

	/**
	 * client base API
	 **/

//	virtual bool has_window(uint32_t w) const;
//	virtual auto base() const -> uint32_t;
//	virtual auto orig() const -> uint32_t;
//	virtual auto base_position() const -> rect const &;
//	virtual auto orig_position() const -> rect const &;
	//virtual void on_property_notify(xcb_property_notify_event_t const * e);

};

}

#else

class view_t;

#endif /* SRC_XDG_SURFACE_TOPLEVEL_VIEW_HXX_ */
