/*
 * page_context.hxx
 *
 *  Created on: 13 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_PAGE_CONTEXT_HXX_
#define SRC_PAGE_CONTEXT_HXX_

#include <typeinfo>

#include "tree-types.hxx"
#include "theme_split.hxx"
#include "keymap.hxx"
#include "pointer-grab-handler.hxx"
#include "keyboard-grab-handler.hxx"
#include "pixmap.hxx"

#include "surface.hxx"

namespace page {

using namespace std;

class theme_t;
class mainloop_t;



struct page_configuration_t {
	bool _replace_wm;
	bool _menu_drop_down_shadow;
	bool _auto_refocus;
	bool _mouse_focus;
	bool _enable_shade_windows;
	int64_t _fade_in_time;
};

/**
 * Page context is a pseudo global context for page.
 * This interface allow any page component to perform action on page context.
 **/
class page_context_t {

public:
	wl_display * _dpy;
	weston_compositor * ec;

	page_context_t() : _dpy{nullptr}, ec{nullptr} { }

	virtual ~page_context_t() { }

	/**
	 * page_context_t virtual API
	 **/

	virtual auto conf() const -> page_configuration_t const & = 0;
	virtual auto theme() const -> theme_t const * = 0;
//	virtual void overlay_add(shared_ptr<tree_t> x) = 0;
//	virtual void add_global_damage(region const & r) = 0;
	virtual auto find_mouse_viewport(int x, int y) const -> viewport_p = 0;
	virtual auto get_current_workspace() const -> workspace_p const & = 0;
	virtual auto get_workspace(int id) const -> workspace_p const & = 0;
	virtual int  get_workspace_count() const = 0;
	virtual int  create_workspace() = 0;
	virtual void grab_start(weston_pointer * pointer, pointer_grab_handler_t * handler) = 0;
	virtual void grab_stop(weston_pointer * pointer) = 0;
	virtual void detach(tree_p t) = 0;
	virtual void insert_window_in_notebook(view_p x, notebook_p n, bool prefer_activate) = 0;
	virtual void fullscreen_client_to_viewport(view_p c, viewport_p v) = 0;
	virtual void unbind_window(view_p mw) = 0;
	virtual void split_left(notebook_p nbk, view_p c) = 0;
	virtual void split_right(notebook_p nbk, view_p c) = 0;
	virtual void split_top(notebook_p nbk, view_p c) = 0;
	virtual void split_bottom(notebook_p nbk, view_p c) = 0;
	virtual void set_keyboard_focus(weston_pointer * pointer, view_p w) = 0;
	virtual void notebook_close(notebook_p nbk) = 0;
//	virtual int  left_most_border() = 0;
//	virtual int  top_most_border() = 0;
	virtual auto global_client_focus_history() -> list<view_w> = 0;
//	virtual auto net_client_list() -> list<shared_ptr<xdg_surface_toplevel_t>> = 0;
//	virtual auto keymap() const -> keymap_t const * = 0;
//	virtual void switch_to_desktop(unsigned int desktop) = 0;
//	virtual auto create_view(xcb_window_t w) -> shared_ptr<client_view_t> = 0;
//	virtual void make_surface_stats(int & size, int & count) = 0;
//	virtual auto mainloop() -> mainloop_t * = 0;
	virtual void sync_tree_view() = 0;
	virtual void manage_client(surface_t * s) = 0;
	virtual auto create_pixmap(uint32_t width, uint32_t height) -> pixmap_p = 0;
	virtual void bind_window(view_p mw, bool activate) = 0;
	virtual void manage_popup(surface_t * s) = 0;
	virtual void configure_popup(surface_t * s) = 0;
	virtual void schedule_repaint() = 0;
	virtual void destroy_surface(surface_t * s) = 0;

//	virtual void manage(page_surface_interface * s) = 0;
//	virtual void unmanage(page_surface_interface * s) = 0;
//	virtual void show_window_menu(page_surface_interface * s, struct weston_seat *seat, int32_t x, int32_t y);
//	virtual void start_move(page_surface_interface * s, struct weston_seat *seat, uint32_t serial);
//	virtual void start_resize(page_surface_interface * s, struct weston_seat *seat, uint32_t serial, int edges);

};


}

#endif /* SRC_PAGE_CONTEXT_HXX_ */
