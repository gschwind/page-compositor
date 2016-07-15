/*
 * page.hxx
 *
 * copyright (2010-2015) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef PAGE_HXX_
#define PAGE_HXX_

#include <memory>
#include <list>
#include <vector>
#include <string>
#include <map>
#include <array>
#include <thread>

#include "config.hxx"

#include "buffer-manager.hxx"

#include "time.hxx"

#include "config_handler.hxx"

#include "key_desc.hxx"
#include "keymap.hxx"

#include "theme.hxx"

#include "popup_alt_tab.hxx"
#include "popup_notebook0.hxx"
#include "popup_split.hxx"

#include "dropdown_menu.hxx"

#include "page_component.hxx"
#include "notebook.hxx"
#include "split.hxx"
#include "viewport.hxx"
#include "workspace.hxx"
#include "compositor_overlay.hxx"

#include "page_event.hxx"

#include "utils.hxx"
#include "mainloop.hxx"
#include "page_root.hxx"
#include "listener.hxx"
#include "xdg-shell-client.hxx"
#include "xdg-surface-base.hxx"
#include "xdg-surface-popup.hxx"
#include "xdg-surface-toplevel.hxx"
#include "xdg-surface-base-view.hxx"
#include "xdg-surface-popup-view.hxx"
#include "xdg-surface-toplevel-view.hxx"

namespace page {

using namespace std;

struct fullscreen_data_t {
	weak_ptr<xdg_surface_toplevel_t> client;
	weak_ptr<workspace_t> workspace;
	weak_ptr<viewport_t> viewport;
	managed_window_type_e revert_type;
	weak_ptr<notebook_t> revert_notebook;
};

/* process_mode_e define possible state of page */
enum process_mode_e {
	PROCESS_NORMAL,						// default evant processing
	PROCESS_SPLIT_GRAB,					// when split is moving
	PROCESS_NOTEBOOK_GRAB,				// when notebook tab is moved
	PROCESS_NOTEBOOK_BUTTON_PRESS,		// when click on close/unbind button
	PROCESS_FLOATING_MOVE,				// when a floating window is moved
	PROCESS_FLOATING_RESIZE,			// when resizing a floating window
	PROCESS_FLOATING_CLOSE,				// when clicking close button of floating window
	PROCESS_FLOATING_BIND,				// when clicking bind button
	PROCESS_FULLSCREEN_MOVE,			// when mod4+click to change fullscreen window screen
	PROCESS_FLOATING_MOVE_BY_CLIENT,	// when moving a floating window started by client himself
	PROCESS_FLOATING_RESIZE_BY_CLIENT,	// when resizing a floating window started by client himself
	PROCESS_NOTEBOOK_MENU,				// when notebook menu is shown
	PROCESS_NOTEBOOK_CLIENT_MENU,		// when switch desktop menu is shown
	PROCESS_ALT_TAB						// when alt-tab running
};

struct key_bind_cmd_t {
	key_desc_t key;
	string cmd;
};

struct page_t : public page_context_t {
	shared_ptr<page_root_t> _root;
	weston_layer default_layer;
	theme_t * _theme;
	page_configuration_t configuration;
	config_handler_t _conf;

	pointer_grab_handler_t * _grab_handler;

	list<signal_handler_t> _slots;


	list<weston_output *> _outputs;

	thread _buffer_manager;

	/**
	 * Store data to allow proper revert fullscreen window to
	 * their original positions
	 **/
	map<xdg_surface_toplevel_t *, fullscreen_data_t> _fullscreen_client_to_viewport;

	string page_base_dir;
	string _theme_engine;

	wl_listener destroy;

	/* surface signals */
	wl_listener create_surface;
	wl_listener activate;
	wl_listener transform;

	wl_listener kill;
	wl_listener idle;
	wl_listener wake;

	wl_listener show_input_panel;
	wl_listener hide_input_panel;
	wl_listener update_input_panel;

	wl_listener seat_created;
	listener_t<weston_output> output_created;
	wl_listener output_destroyed;
	wl_listener output_moved;

	wl_listener session;

	wl_resource * _buffer_manager_resource;
	list<pixmap_p> pixmap_list;

	xdg_surface_toplevel_view_w _current_focus;

	using repaint_func = int (*)(weston_output *, pixman_region32_t *);

	map<weston_output *, repaint_func> repaint_functions;

	struct _default_grab_interface_t {
		weston_pointer_grab_interface grab_interface;
		page_t * ths;
	} default_grab_pod;

	weston_pointer_grab_interface const * old_grab_interface;


	struct _page_clients {
		xdg_shell_client_t * client;
		signal_handler_t destroy;
		signal_handler_t create_popup;
		signal_handler_t create_toplevel;
	};

	list<_page_clients> _clients;

	wl_global * _global_xdg_shell;
	wl_global * _global_buffer_manager;

//	key_desc_t bind_page_quit;
//	key_desc_t bind_toggle_fullscreen;
//	key_desc_t bind_toggle_compositor;
//	key_desc_t bind_close;
//
//	key_desc_t bind_exposay_all;
//
//	key_desc_t bind_right_desktop;
//	key_desc_t bind_left_desktop;
//
//	key_desc_t bind_bind_window;
//	key_desc_t bind_fullscreen_window;
//	key_desc_t bind_float_window;
//
//	keymap_t * _keymap;
//
//	key_desc_t bind_debug_1;
//	key_desc_t bind_debug_2;
//	key_desc_t bind_debug_3;
//	key_desc_t bind_debug_4;
//
//	array<key_bind_cmd_t, 10> bind_cmd;

	//xcb_timestamp_t _last_focus_time;
	//xcb_timestamp_t _last_button_press;

	/** store all client in mapping order, older first **/
	list<xdg_surface_toplevel_view_w> _net_client_list;
	list<xdg_surface_toplevel_view_w> _global_focus_history;

//	int _left_most_border;
//	int _top_most_border;

	/* do no allow copy */
	page_t(page_t const &) = delete;
	page_t &operator=(page_t const &) = delete;

	page_t(int argc, char ** argv);

	virtual ~page_t();

	/* run page main loop */
	void run();

	void client_destroy(xdg_shell_client_t *);
	void client_create_popup(xdg_shell_client_t *, xdg_surface_popup_t *);
	void client_create_toplevel(xdg_shell_client_t *, xdg_surface_toplevel_t *);


//	void set_default_pop(shared_ptr<notebook_t> x);
//	display_compositor_t * get_render_context();
//
//	/** user inputs **/
//	void process_key_press_event(xcb_generic_event_t const * e);
//	void process_key_release_event(xcb_generic_event_t const * e);
//	void process_button_press_event(xcb_generic_event_t const * e);
//	void process_motion_notify(xcb_generic_event_t const * _e);
//	void process_button_release(xcb_generic_event_t const * _e);
//	/* SubstructureNotifyMask */
//	//void process_event(xcb_circulate_notify_event_t const * e);
//	void process_configure_notify_event(xcb_generic_event_t const * e);
//	void process_create_notify_event(xcb_generic_event_t const * e);
//	void process_destroy_notify_event(xcb_generic_event_t const * e);
//	void process_gravity_notify_event(xcb_generic_event_t const * e);
//	void process_map_notify_event(xcb_generic_event_t const * e);
//	void process_reparent_notify_event(xcb_generic_event_t const * e);
//	void process_unmap_notify_event(xcb_generic_event_t const * e);
//	void process_fake_unmap_notify_event(xcb_generic_event_t const * e);
//	void process_mapping_notify_event(xcb_generic_event_t const * e);
//	void process_selection_clear_event(xcb_generic_event_t const * e);
//	void process_focus_in_event(xcb_generic_event_t const * e);
//	void process_focus_out_event(xcb_generic_event_t const * e);
//	void process_enter_window_event(xcb_generic_event_t const * e);
//	void process_leave_window_event(xcb_generic_event_t const * e);
//
//	void process_expose_event(xcb_generic_event_t const * e);
//
//	void process_randr_notify_event(xcb_generic_event_t const * e);
//	void process_shape_notify_event(xcb_generic_event_t const * e);
//
//	/* SubstructureRedirectMask */
//	void process_circulate_request_event(xcb_generic_event_t const * e);
//	void process_configure_request_event(xcb_generic_event_t const * e);
//	void process_map_request_event(xcb_generic_event_t const * e);
//
//	/* PropertyChangeMask */
//	void process_property_notify_event(xcb_generic_event_t const * e);
//
//	/* Unmaskable Events */
//	void process_fake_client_message_event(xcb_generic_event_t const * e);
//
//	/* extension events */
//	void process_damage_notify_event(xcb_generic_event_t const * ev);
//
//	void process_event(xcb_generic_event_t const * e);
//
//	/* update _NET_CLIENT_LIST_STACKING and _NET_CLIENT_LIST */
//	void update_client_list();
//	void update_client_list_stacking();
//
//	/* update _NET_SUPPORTED */
//	void update_net_supported();
//
//	/* setup and create managed window */
//	xdg_surface_toplevel_t * manage(xcb_atom_t net_wm_type, std::shared_ptr<client_proxy_t> wa);
//
//	/* unmanage a managed window */
//	void unmanage(shared_ptr<xdg_surface_toplevel_t> mw);
//
//	/* update viewport and childs allocation */
//	void update_workarea();
//
//	/* turn a managed window into fullscreen */
//	void fullscreen(shared_ptr<xdg_surface_toplevel_t> c);
//	void fullscreen(shared_ptr<xdg_surface_toplevel_t> c, shared_ptr<viewport_t> v);
//
//	/* switch a fullscreened and managed window into floating or notebook window */
//	void unfullscreen(shared_ptr<xdg_surface_toplevel_t> c);
//
//	/* toggle fullscreen */
//	void toggle_fullscreen(shared_ptr<xdg_surface_toplevel_t> c);
//
//	/* split a notebook into two notebook */
//	void split(shared_ptr<notebook_t> nbk, split_type_e type);
//
//	/* compute the allocation of viewport taking in account DOCKs */
//	void compute_viewport_allocation(shared_ptr<workspace_t> d, shared_ptr<viewport_t> v);
//
//	//void cleanup_not_managed_client(shared_ptr<xdg_surface_popup_t> c);
//
//	void process_net_vm_state_client_message(xcb_window_t c, long type, xcb_atom_t state_properties);
//
	void insert_in_tree_using_transient_for(xdg_surface_toplevel_view_p c);
//
//	void safe_update_transient_for(shared_ptr<xdg_surface_base_t> c);
//
//	shared_ptr<xdg_surface_base_t> get_transient_for(shared_ptr<xdg_surface_base_t> c);
//	void logical_raise(shared_ptr<xdg_surface_base_t> c);
//
//	/* attach floating window in a notebook */
	void bind_window(shared_ptr<xdg_surface_toplevel_view_t> mw, bool activate);
//	void grab_pointer();
//	/* if grab is linked to a given window remove this grab */
//	void cleanup_grab();
//	/* find a valid notebook, that is in subtree base and that is no nbk */
	shared_ptr<notebook_t> get_another_notebook(shared_ptr<tree_t> base, shared_ptr<tree_t> nbk);
//	/* find where the managed window is */
	static shared_ptr<notebook_t> find_parent_notebook_for(shared_ptr<xdg_surface_toplevel_view_t> mw);
//	shared_ptr<xdg_surface_toplevel_t> find_managed_window_with(xcb_window_t w);
	static shared_ptr<viewport_t> find_viewport_of(shared_ptr<tree_t> n);
	static shared_ptr<workspace_t> find_desktop_of(shared_ptr<tree_t> n);
//	void set_window_cursor(xcb_window_t w, xcb_cursor_t c);
	void update_windows_stack();
	void update_viewport_layout();
	void remove_viewport(shared_ptr<workspace_t> d, shared_ptr<viewport_t> v);
//	void onmap(xcb_window_t w);
//	void create_managed_window(xcb_window_t w, xcb_atom_t type);
//	void ackwoledge_configure_request(xcb_configure_request_event_t const * e);
	void create_unmanaged_window(xcb_window_t w, xcb_atom_t type);
//	bool get_safe_net_wm_user_time(shared_ptr<xdg_surface_base_t> c, xcb_timestamp_t & time);
//	void update_page_areas();
//	void set_desktop_geometry(long width, long height);
//	shared_ptr<xdg_surface_base_t> find_client_with(xcb_window_t w);
//	shared_ptr<xdg_surface_base_t> find_client(xcb_window_t w);
	void remove_client(xdg_surface_base_view_p c);
//
//	void raise_child(shared_ptr<tree_t> t);
//	void process_notebook_client_menu(shared_ptr<xdg_surface_toplevel_t> c, int selected);
//
//	void check_x11_extension();
//
//	void create_identity_window();
//	void register_wm();
//	void register_cm();
//
//	void render(cairo_t * cr, time64_t time);
//	bool need_render(time64_t time);
//
//	bool check_for_managed_window(xcb_window_t w);
//	bool check_for_destroyed_window(xcb_window_t w);
//
//	void update_keymap();
//	void update_grabkey();
//
//	shared_ptr<xdg_surface_toplevel_t> find_hidden_client_with(xcb_window_t w);
//
//	vector<page_event_t> compute_page_areas(viewport_t * v) const;
//
//	void render();
//
//	/** debug function that try to print the state of page in stdout **/
//	void print_state() const;
//	void update_current_desktop() const;
//	void switch_to_desktop(unsigned int desktop);
//	void update_fullscreen_clients_position();
//	void update_desktop_visibility();
//	//void process_error(xcb_generic_event_t const * e);
//	void start_compositor();
//	void stop_compositor();
	void run_cmd(string const & cmd_with_args);
//
//	void start_alt_tab(xcb_timestamp_t time);
//
//	vector<shared_ptr<xdg_surface_toplevel_t>> get_sticky_client_managed(shared_ptr<tree_t> base);
//	void reconfigure_docks(shared_ptr<workspace_t> const & d);
//
//	void mark_durty(shared_ptr<tree_t> t);
//
	unsigned int find_current_desktop(shared_ptr<xdg_surface_base_t> c);
//
//	void process_pending_events();
//
	bool global_focus_history_front(shared_ptr<xdg_surface_toplevel_view_t> & out);
	void global_focus_history_remove(shared_ptr<xdg_surface_toplevel_view_t> in);
	void global_focus_history_move_front(shared_ptr<xdg_surface_toplevel_view_t> in);
	bool global_focus_history_is_empty();
//
//	void on_visibility_change_handler(xcb_window_t xid, bool visible);
//	void on_block_mainloop_handler();
//
//	auto find_client_managed_with(xcb_window_t w) -> shared_ptr<xdg_surface_toplevel_t>;

	void process_focus(weston_pointer_grab * grab);
	void process_motion(weston_pointer_grab * grab, uint32_t time, weston_pointer_motion_event *event);
	void process_button(weston_pointer_grab * grab, uint32_t time, uint32_t button, uint32_t state);
	void process_axis(weston_pointer_grab * grab, uint32_t time, weston_pointer_axis_event *event);
	void process_axis_source(weston_pointer_grab * grab, uint32_t source);
	void process_frame(weston_pointer_grab * grab);
	void process_cancel(weston_pointer_grab * grab);

	void connect_all();
	void on_output_created(weston_output * output);
	void load_x11_backend(weston_compositor* ec);
	static void bind_xdg_shell(wl_client * client, void * data,
					      uint32_t version, uint32_t id);
	static void bind_zzz_buffer_manager(struct wl_client * client, void * data,
		      uint32_t version, uint32_t id);
	static void print_tree_binding(struct weston_keyboard *keyboard, uint32_t time,
			  uint32_t key, void *data);
	static int page_repaint(struct weston_output *output_base,
			   pixman_region32_t *damage);

	void configure_surface(xdg_surface_toplevel_view_p,
			int32_t sx, int32_t sy);

//	/**
//	 * page_context_t virtual API
//	 **/
//
	virtual auto conf() const -> page_configuration_t const &;
	virtual auto theme() const -> theme_t const *;
//	virtual void overlay_add(shared_ptr<tree_t> x);
//	virtual void add_global_damage(region const & r);
	virtual auto find_mouse_viewport(int x, int y) const -> viewport_p;
	virtual auto get_current_workspace() const -> workspace_p const &;
	virtual auto get_workspace(int id) const -> workspace_p const &;
	virtual int  get_workspace_count() const;
	virtual int  create_workspace();
	virtual void grab_start(weston_pointer * pointer, pointer_grab_handler_t * handler);
	virtual void grab_stop(weston_pointer * pointer);
	virtual void detach(tree_p t);
	virtual void insert_window_in_notebook(xdg_surface_toplevel_view_p x, notebook_p n, bool prefer_activate);
	virtual void fullscreen_client_to_viewport(xdg_surface_toplevel_view_p c, viewport_p v);
	virtual void unbind_window(xdg_surface_toplevel_view_p mw);
	virtual void split_left(notebook_p nbk, xdg_surface_toplevel_view_p c);
	virtual void split_right(notebook_p nbk, xdg_surface_toplevel_view_p c);
	virtual void split_top(notebook_p nbk, xdg_surface_toplevel_view_p c);
	virtual void split_bottom(notebook_p nbk, xdg_surface_toplevel_view_p c);
	virtual void set_focus(weston_pointer * pointer, xdg_surface_toplevel_view_p w);
	virtual void notebook_close(notebook_p nbk);
//	virtual int  left_most_border();
//	virtual int  top_most_border();
	virtual auto global_client_focus_history() -> list<xdg_surface_toplevel_view_w>;
//	virtual auto net_client_list() -> list<shared_ptr<xdg_surface_toplevel_t>>;
//	virtual auto keymap() const -> keymap_t const *;
//	virtual auto create_view(xcb_window_t w) -> shared_ptr<client_view_t>;
//	virtual void make_surface_stats(int & size, int & count);
//	virtual auto mainloop() -> mainloop_t *;
	virtual void sync_tree_view();
	virtual void manage_client(xdg_surface_toplevel_view_p mw);
	virtual auto create_pixmap(uint32_t width, uint32_t height) -> pixmap_p;

	/**
	 * the xdg-popup
	 **/
	virtual void xdg_popup_destroy(wl_client * client, wl_resource * resource);


};


}



#endif /* PAGE_HXX_ */
