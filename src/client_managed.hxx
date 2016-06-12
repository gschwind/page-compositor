/*
 * managed_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_MANAGED_HXX_
#define CLIENT_MANAGED_HXX_

#include <string>
#include <vector>
#include <typeinfo>

#include "xdg-surface-interface.hxx"
#include "icon_handler.hxx"
#include "theme.hxx"

#include "floating_event.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_pixmap.hxx"


namespace page {

class page_t;

using namespace std;

enum managed_window_type_e {
	MANAGED_UNDEFINED,
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN,
	MANAGED_DOCK
};

class xdg_surface_toplevel_t : public xdg_surface_vtable, public xdg_surface_base_t {

	friend class page::page_t;

	struct _state {
		std::string title;
		bool fullscreen;
		bool maximize;
		bool minimize;
		xdg_surface_toplevel_t * transient_for;
		rect geometry;

		_state() {
			fullscreen = false;
			maximize = false;
			minimize = false;
			title = "";
			transient_for = nullptr;
			geometry = rect{0,0,0,0};
		}

	} _pending;

	weston_view * _default_view;

	/** hold floating position **/
	rect _floating_wished_position;

	/** hold notebook position **/
	rect _notebook_wished_position;

	/** the absolute position without border **/
	rect _wished_position;

	rect _orig_position;
	rect _base_position;

	// the output surface (i.e. surface where we write things)
	cairo_surface_t * _surf;

	// window title cache
	string _title;

	// icon cache
	shared_ptr<icon16> _icon;

	xdg_surface_toplevel_t * _transient_for;

	bool _has_focus;
	bool _is_iconic;
	bool _demands_attention;
	bool _is_resized;
	bool _is_exposed;
	bool _has_change;

	/* handle the state of management: notebook, floating, fullscreen */
	managed_window_type_e _managed_type;

	/* private to avoid copy */
	xdg_surface_toplevel_t(xdg_surface_toplevel_t const &) = delete;
	xdg_surface_toplevel_t & operator=(xdg_surface_toplevel_t const &) = delete;

	void set_wished_position(rect const & position);
	rect const & get_wished_position() const;

	void set_theme(theme_t const * theme);


	static void xdg_surface_delete(wl_resource *resource);

	/** called on surface commit */
	static void _weston_configure(struct weston_surface *es, int32_t sx, int32_t sy);


public:

	auto shared_from_this() -> shared_ptr<xdg_surface_toplevel_t>;


	static auto get(wl_resource * r) -> xdg_surface_toplevel_t *;

	xdg_surface_toplevel_t(page_context_t * ctx, wl_client * client,
			weston_surface * surface, uint32_t id);
	virtual ~xdg_surface_toplevel_t();

	signal_t<xdg_surface_toplevel_t *> on_destroy;
	signal_t<shared_ptr<xdg_surface_toplevel_t>> on_title_change;
	signal_t<shared_ptr<xdg_surface_toplevel_t>> on_focus_change;
	signal_t<shared_ptr<xdg_surface_toplevel_t>, int32_t, int32_t> on_configure;

	auto resource() const -> wl_resource *;

	bool is(managed_window_type_e type);
	auto title() const -> string const &;
	auto get_wished_position() -> rect const &;
	void set_floating_wished_position(rect const & pos);
	rect get_base_position() const;
	void reconfigure();
	void normalize();
	void iconify();
	bool has_focus() const;
	bool is_iconic();
	//void delete_window(xcb_timestamp_t);
	auto icon() const -> shared_ptr<icon16>;
	void set_notebook_wished_position(rect const & pos);
	void set_current_desktop(unsigned int n);
	bool is_stiky();
	bool is_modal();
	//void net_wm_state_add(atom_e atom);
	//void net_wm_state_remove(atom_e atom);
	void net_wm_state_delete();
	void wm_state_delete();
	bool is_fullscreen();
	bool skip_task_bar();
	auto get_floating_wished_position() -> rect const & ;
	bool lock();
	void unlock();
	void set_focus_state(bool is_focused);
	void set_demands_attention(bool x);
	bool demands_attention();
	//void focus(xcb_timestamp_t t);
	auto get_type() -> managed_window_type_e;
	void set_managed_type(managed_window_type_e type);
	void grab_button_focused_unsafe();
	void grab_button_unfocused_unsafe();


	void set_title(char const *);
	void set_transient_for(xdg_surface_toplevel_t * s);
	auto transient_for() const -> xdg_surface_toplevel_t *;

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
	// virtual bool button_press(xcb_button_press_event_t const * ev);
	// virtual bool button_release(xcb_button_release_event_t const * ev);
	// virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	// virtual void expose(xcb_expose_event_t const * ev);

	// virtual auto get_xid() const -> xcb_window_t;
	// virtual auto get_parent_xid() const -> xcb_window_t;
	// virtual rect get_window_position() const;

	virtual void queue_redraw();
	virtual void trigger_redraw();

	/**
	 * client base API
	 **/

//	virtual bool has_window(uint32_t w) const;
//	virtual auto base() const -> uint32_t;
//	virtual auto orig() const -> uint32_t;
	virtual auto base_position() const -> rect const &;
	virtual auto orig_position() const -> rect const &;
	//virtual void on_property_notify(xcb_property_notify_event_t const * e);

	virtual auto get_default_view() const -> weston_view *;

	/**
	 * xdg-surface-interface
	 **/
	virtual void xdg_surface_destroy(wl_client * client, wl_resource * resource)
			override;
	virtual void xdg_surface_set_parent(wl_client * client,
			wl_resource * resource, wl_resource * parent_resource) override;
	virtual void xdg_surface_set_app_id(wl_client * client,
			wl_resource * resource, const char * app_id) override;
	virtual void xdg_surface_show_window_menu(wl_client * client,
			wl_resource * surface_resource, wl_resource * seat_resource,
			uint32_t serial, int32_t x, int32_t y) override;
	virtual void xdg_surface_set_title(wl_client * client,
			wl_resource * resource, const char * title) override;
	virtual void xdg_surface_move(wl_client * client, wl_resource * resource,
			wl_resource* seat_resource, uint32_t serial) override;
	virtual void xdg_surface_resize(wl_client* client, wl_resource * resource,
			wl_resource * seat_resource, uint32_t serial, uint32_t edges)
					override;
	virtual void xdg_surface_ack_configure(wl_client * client,
			wl_resource * resource, uint32_t serial) override;
	virtual void xdg_surface_set_window_geometry(wl_client * client,
			wl_resource * resource, int32_t x, int32_t y, int32_t width,
			int32_t height) override;
	virtual void xdg_surface_set_maximized(wl_client * client,
			wl_resource * resource) override;
	virtual void xdg_surface_unset_maximized(wl_client* client,
			wl_resource* resource) override;
	virtual void xdg_surface_set_fullscreen(wl_client * client,
			wl_resource * resource, wl_resource * output_resource) override;
	virtual void xdg_surface_unset_fullscreen(wl_client * client,
			wl_resource * resource) override;
	virtual void xdg_surface_set_minimized(wl_client * client,
			wl_resource * resource) override;

};


using client_managed_p = shared_ptr<xdg_surface_toplevel_t>;
using client_managed_w = weak_ptr<xdg_surface_toplevel_t>;

}


#endif /* MANAGED_WINDOW_HXX_ */
