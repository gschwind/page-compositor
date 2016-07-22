/*
 * page.cxx
 *
 * copyright (2010-2015) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

/* According to POSIX.1-2001 */
#include <sys/select.h>
#include <poll.h>

#include <cairo.h>

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <string>
#include <sstream>
#include <limits>
#include <stdint.h>
#include <stdexcept>
#include <set>
#include <stack>
#include <vector>
#include <typeinfo>
#include <memory>
#include <utility>
#include <list>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/input.h>
#include <libweston-0/compositor.h>
#include <libweston-0/compositor-x11.h>
#include <wayland-client-protocol.h>
#include <xdg-surface-base.hxx>
#include <xdg-surface-popup.hxx>
#include <xdg-surface-toplevel.hxx>
#include "xdg-shell-server-protocol.h"
#include "buffer-manager-server-protocol.h"
#include "buffer-manager-client-protocol.h"

#include "utils.hxx"

#include "renderable.hxx"
#include "key_desc.hxx"
#include "time.hxx"
#include "grab_handlers.hxx"

#include "simple2_theme.hxx"
#include "tiny_theme.hxx"

#include "notebook.hxx"
#include "workspace.hxx"
#include "split.hxx"
#include "page.hxx"

#include "popup_alt_tab.hxx"

/* ICCCM definition */
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

namespace page {

static void _page_focus(weston_pointer_grab * grab) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_focus(grab);
}

static void _page_motion(weston_pointer_grab * grab, uint32_t time, weston_pointer_motion_event *event) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_motion(grab, time, event);
}

static void _page_button(weston_pointer_grab * grab, uint32_t time, uint32_t button, uint32_t state) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_button(grab, time, button, state);
}

static void _page_axis(weston_pointer_grab * grab, uint32_t time, weston_pointer_axis_event *event) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_axis(grab, time, event);
}

static void _page_axis_source(weston_pointer_grab * grab, uint32_t source) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_axis_source(grab, source);
}

static void _page_frame(weston_pointer_grab * grab) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_frame(grab);
}

static void _page_cancel(weston_pointer_grab * grab) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_cancel(grab);
}


int page_t::page_repaint(struct weston_output *output_base,
		   pixman_region32_t *damage) {
	auto ths = reinterpret_cast<page_t*>(weston_compositor_get_user_data(output_base->compositor));

	weston_log("call %s\n", __PRETTY_FUNCTION__);
	ths->_root->broadcast_trigger_redraw();

	auto func = ths->repaint_functions[output_base];
	output_base->repaint = func;
	int ret = (*output_base->repaint)(output_base, damage);
	output_base->repaint = &page_t::page_repaint;
	return ret;

}

static void ack_buffer(struct wl_client *client,
		   wl_resource * resource,
		   uint32_t serial,
		   wl_resource * surface,
		   wl_resource * buffer) {
	auto ths = reinterpret_cast<page_t*>(wl_resource_get_user_data(resource));

	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto x = std::find_if(ths->pixmap_list.begin(), ths->pixmap_list.end(),
			[serial](pixmap_p p) -> bool{
				return p->serial() == serial;
			});

	if(x != ths->pixmap_list.end())
		(*x)->ack_buffer(client, resource, serial, surface, buffer);

}

static void xx_buffer_delete(wl_resource * r) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	/* TODO */
}

static const struct zzz_buffer_manager_interface _zzz_buffer_manager_interface = {
		ack_buffer
};

//time64_t const page_t::default_wait{1000000000L / 120L};

void page_t::bind_xdg_shell(struct wl_client * client, void * data,
				      uint32_t version, uint32_t id) {
	page_t * ths = reinterpret_cast<page_t *>(data);

	_page_clients c;
	c.client = new xdg_shell_client_t{ths, client, id};
//	c.create_popup = c.client->create_popup.connect(ths, &page_t::client_create_popup);
//	c.create_toplevel = c.client->create_toplevel.connect(ths, &page_t::client_create_toplevel);
	c.destroy = c.client->destroy.connect(ths, &page_t::client_destroy);

	ths->_clients.push_back(c);

}

void page_t::bind_zzz_buffer_manager(struct wl_client * client, void * data,
	      uint32_t version, uint32_t id) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	page_t * ths = reinterpret_cast<page_t *>(data);

	if(ths->_buffer_manager_resource)
		throw exception_t{"only one buffer manager is allowed"};

	/* ONLY one those client */
	ths->_buffer_manager_resource = wl_resource_create(client, &::zzz_buffer_manager_interface, 1,
			id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	wl_resource_set_implementation(ths->_buffer_manager_resource,
			&_zzz_buffer_manager_interface, ths, &xx_buffer_delete);


	for(auto & p: ths->pixmap_list) {
		p->bind_buffer_manager();
	}

}

void page_t::print_tree_binding(struct weston_keyboard *keyboard, uint32_t time,
		  uint32_t key, void *data) {
	page_t * ths = reinterpret_cast<page_t *>(data);
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	ths->_root->print_tree(0);
}

page_t::page_t(int argc, char ** argv)
{

	char const * conf_file_name = 0;

	configuration._replace_wm = false;
	configuration._menu_drop_down_shadow = false;

	/** parse command line **/

	int k = 1;
	while(k < argc) {
		string x = argv[k];
		if(x == "--replace") {
			configuration._replace_wm = true;
		} else {
			conf_file_name = argv[k];
		}
		++k;
	}

	/* load configurations, from lower priority to high one */

	/* load default configuration */
	_conf.merge_from_file_if_exist(string{DATA_DIR "/page/page.conf"});

	/* load homedir configuration */
	{
		char const * chome = getenv("HOME");
		if(chome != nullptr) {
			string xhome = chome;
			string file = xhome + "/.page.conf";
			_conf.merge_from_file_if_exist(file);
		}
	}

	/* load file in arguments if provided */
	if (conf_file_name != nullptr) {
		string s(conf_file_name);
		_conf.merge_from_file_if_exist(s);
	}

	page_base_dir = _conf.get_string("default", "theme_dir");
	_theme_engine = _conf.get_string("default", "theme_engine");

//	_left_most_border = std::numeric_limits<int>::max();
//	_top_most_border = std::numeric_limits<int>::max();

	_theme = nullptr;

	_buffer_manager_resource = nullptr;

	_grab_handler = nullptr;

//	bind_page_quit           = _conf.get_string("default", "bind_page_quit");
//	bind_close               = _conf.get_string("default", "bind_close");
//	bind_exposay_all         = _conf.get_string("default", "bind_exposay_all");
//	bind_toggle_fullscreen   = _conf.get_string("default", "bind_toggle_fullscreen");
//	bind_toggle_compositor   = _conf.get_string("default", "bind_toggle_compositor");
//	bind_right_desktop       = _conf.get_string("default", "bind_right_desktop");
//	bind_left_desktop        = _conf.get_string("default", "bind_left_desktop");
//
//	bind_bind_window         = _conf.get_string("default", "bind_bind_window");
//	bind_fullscreen_window   = _conf.get_string("default", "bind_fullscreen_window");
//	bind_float_window        = _conf.get_string("default", "bind_float_window");
//
//	bind_debug_1 = _conf.get_string("default", "bind_debug_1");
//	bind_debug_2 = _conf.get_string("default", "bind_debug_2");
//	bind_debug_3 = _conf.get_string("default", "bind_debug_3");
//	bind_debug_4 = _conf.get_string("default", "bind_debug_4");
//
//	bind_cmd[0].key = _conf.get_string("default", "bind_cmd_0");
//	bind_cmd[1].key = _conf.get_string("default", "bind_cmd_1");
//	bind_cmd[2].key = _conf.get_string("default", "bind_cmd_2");
//	bind_cmd[3].key = _conf.get_string("default", "bind_cmd_3");
//	bind_cmd[4].key = _conf.get_string("default", "bind_cmd_4");
//	bind_cmd[5].key = _conf.get_string("default", "bind_cmd_5");
//	bind_cmd[6].key = _conf.get_string("default", "bind_cmd_6");
//	bind_cmd[7].key = _conf.get_string("default", "bind_cmd_7");
//	bind_cmd[8].key = _conf.get_string("default", "bind_cmd_8");
//	bind_cmd[9].key = _conf.get_string("default", "bind_cmd_9");
//
//	bind_cmd[0].cmd = _conf.get_string("default", "exec_cmd_0");
//	bind_cmd[1].cmd = _conf.get_string("default", "exec_cmd_1");
//	bind_cmd[2].cmd = _conf.get_string("default", "exec_cmd_2");
//	bind_cmd[3].cmd = _conf.get_string("default", "exec_cmd_3");
//	bind_cmd[4].cmd = _conf.get_string("default", "exec_cmd_4");
//	bind_cmd[5].cmd = _conf.get_string("default", "exec_cmd_5");
//	bind_cmd[6].cmd = _conf.get_string("default", "exec_cmd_6");
//	bind_cmd[7].cmd = _conf.get_string("default", "exec_cmd_7");
//	bind_cmd[8].cmd = _conf.get_string("default", "exec_cmd_8");
//	bind_cmd[9].cmd = _conf.get_string("default", "exec_cmd_9");

	if(_conf.get_string("default", "auto_refocus") == "true") {
		configuration._auto_refocus = true;
	} else {
		configuration._auto_refocus = false;
	}

	if(_conf.get_string("default", "enable_shade_windows") == "true") {
		configuration._enable_shade_windows = true;
	} else {
		configuration._enable_shade_windows = false;
	}

	if(_conf.get_string("default", "mouse_focus") == "true") {
		configuration._mouse_focus = true;
	} else {
		configuration._mouse_focus = false;
	}

	if(_conf.get_string("default", "menu_drop_down_shadow") == "true") {
		configuration._menu_drop_down_shadow = true;
	} else {
		configuration._menu_drop_down_shadow = false;
	}

	configuration._fade_in_time = _conf.get_long("compositor", "fade_in_time");


	default_grab_pod.grab_interface.focus = &_page_focus;
	default_grab_pod.grab_interface.motion = &_page_motion;
	default_grab_pod.grab_interface.button = &_page_button;
	default_grab_pod.grab_interface.axis = &_page_axis;
	default_grab_pod.grab_interface.axis_source = &_page_axis_source;
	default_grab_pod.grab_interface.frame = &_page_frame;
	default_grab_pod.grab_interface.cancel = &_page_cancel;
	default_grab_pod.ths = this;

}

page_t::~page_t() {
	// cleanup cairo, for valgrind happiness.
	//cairo_debug_reset_static_data();
}

void page_t::run() {

	/** initialize the empty desktop **/
	_root = make_shared<page_root_t>(this);

	{
		/** create the first desktop **/
		auto d = make_shared<workspace_t>(this, 0);
		_root->_desktop_list.push_back(d);
		_root->_desktop_stack->push_front(d);
		d->hide();
	}

	/** Initialize theme **/

	if(_theme_engine == "tiny") {
		cout << "using tiny theme engine" << endl;
		_theme = new tiny_theme_t{_conf};
	} else {
		/* The default theme engine */
		cout << "using simple theme engine" << endl;
		_theme = new simple2_theme_t{_conf};
	}

	/* start listen root event before anything, each event will be stored to be processed later */
	/** TODO: set default grab **/

	/**
	 * listen RRCrtcChangeNotifyMask for possible change in screen layout.
	 **/
	/** TODO: define a handler for input/output creation **/


	//update_keymap();
	//update_grabkey();

	// no window at the moment remove : update_windows_stack();

	//get_current_workspace()->show();

	/* process messages as soon as we get messages, or every 1/60 of seconds */
	// TODO: use the wayland mainloop
	//_mainloop.add_poll(_dpy->fd(), POLLIN|POLLPRI|POLLERR, [this](struct pollfd const & x) -> void { this->process_pending_events(); });
	//auto on_visibility_change_func = _dpy->on_visibility_change.connect(this, &page_t::on_visibility_change_handler);

	/* dummy on block function */
	//auto mainloop_on_block_slot = _mainloop.on_block.connect(this, &page_t::on_block_mainloop_handler);

	/* TODO: wl_run */
	//_mainloop.run();


    weston_log_set_handler(vprintf, vprintf);

	/* first create the wayland serveur */
	_dpy = wl_display_create();

	auto sock_name = wl_display_add_socket_auto(_dpy);
	weston_log("socket name = %s\n", sock_name);

	/* set the environment for children */
	setenv("WAYLAND_DISPLAY", sock_name, 1);

	int xxx[2];

	socketpair(AF_UNIX, SOCK_STREAM, 0, xxx);

	/* create the client to one hand */
	_buffer_manager = std::thread{buffer_manager_main, xxx[1]};

	/* connect to the serveur the other hand */
	auto client = wl_client_create(_dpy, xxx[0]);

	/*
	 * Weston compositor will create all core globals:
	 *  - wl_compositor
	 *  - wl_output
	 *  - wl_subcompositor
	 *  - wl_presentation
	 *  - wl_data_device_manager
	 *  - wl_seat
	 *  - zwp_linux_dmabuf_v1
	 *  - [...]
	 *  but not the xdg_shell one.
	 *
	 */
	ec = weston_compositor_create(_dpy, nullptr);
	weston_log("weston_compositor = %p\n", ec);

	ec->user_data = this;

	weston_layer_init(&default_layer, &ec->cursor_layer.link);

	_global_xdg_shell = wl_global_create(_dpy, &xdg_shell_interface, 1, this,
			&page_t::bind_xdg_shell);
	_global_buffer_manager = wl_global_create(_dpy,
			&zzz_buffer_manager_interface, 1, this,
			&page_t::bind_zzz_buffer_manager);


	connect_all();

	/* setup the keyboard layout (MANDATORY) */
	xkb_rule_names names = {
			"",					/*rules*/
			"pc104",			/*model*/
			"us",				/*layout*/
			"",					/*variant*/
			""					/*option*/
	};
	weston_compositor_set_xkb_rule_names(ec, &names);

	weston_compositor_add_key_binding(ec, KEY_7,
				          (enum weston_keyboard_modifier)((int)MODIFIER_CTRL | (int)MODIFIER_ALT),
				          print_tree_binding, this);

	load_x11_backend(ec);

	update_viewport_layout();

	old_grab_interface = ec->default_pointer_grab;
	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);

	weston_compositor_wake(ec);

    wl_display_run(_dpy);


	cout << "Page END" << endl;

	//_dpy->on_visibility_change.remove(on_visibility_change_func);
	//_mainloop.remove_poll(_dpy->fd());



	/** destroy the tree **/
	_root = nullptr;

	//delete _keymap; _keymap = nullptr;
	delete _theme; _theme = nullptr;

}

//void page_t::unmanage(shared_ptr<xdg_surface_toplevel_t> mw) {
//	if(mw == nullptr)
//		return;
//
//	/* if window is in move/resize/notebook move, do cleanup */
//	cleanup_grab();
//
//	detach(mw);
//
//	printf("unmanaging : '%s'\n", mw->title().c_str());
//
//	if (has_key(_fullscreen_client_to_viewport, mw.get())) {
//		fullscreen_data_t & data = _fullscreen_client_to_viewport[mw.get()];
//		if(not data.workspace.expired() and not data.viewport.expired()) {
//			if(data.workspace.lock()->is_visible()) {
//				data.viewport.lock()->show();
//			}
//		}
//		_fullscreen_client_to_viewport.erase(mw.get());
//	}
//
//	/* if managed window have active clients */
//	for(auto i: mw->children()) {
//		auto c = dynamic_pointer_cast<xdg_surface_base_t>(i);
//		if(c != nullptr) {
//			insert_in_tree_using_transient_for(c);
//		}
//	}
//
//	if(not mw->skip_task_bar()) {
//		_need_update_client_list = true;
//	}
//
//	update_workarea();
//
//	/** if the window is destroyed, this not work, see fix on destroy **/
//	for(auto x: _root->_desktop_list) {
//		x->client_focus_history_remove(mw);
//	}
//
//	global_focus_history_remove(mw);
//
//	shared_ptr<xdg_surface_toplevel_t> new_focus;
//	set_focus(nullptr, XCB_CURRENT_TIME);
//
//}
//
//void page_t::scan() {
//		/* TODO: remove */
//}
//
//void page_t::update_net_supported() {
//	/* TODO: remove */
//}
//
//void page_t::update_client_list() {
//	/* TODO: remove */
//}
//
//void page_t::update_client_list_stacking() {
//	/* TODO: remove */
//}
//
//void page_t::process_key_press_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_key_press_event_t const *>(_e);
//
//	/* TODO: global key bindings */
//
//////	printf("%s key = %d, mod4 = %s, mod1 = %s\n",
//////			e->response_type == XCB_KEY_PRESS ? "KeyPress" : "KeyRelease",
//////			e->detail,
//////			e->state & XCB_MOD_MASK_4 ? "true" : "false",
//////			e->state & XCB_MOD_MASK_1 ? "true" : "false");
////
////	/* get KeyCode for Unmodified Key */
////
////	key_desc_t key;
////
////	key.ks = _keymap->get(e->detail);
////	key.mod = e->state;
////
////	if (key.ks == 0)
////		return;
////
////
////	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
////	if(_keymap->numlock_mod_mask() != 0) {
////		key.mod &= ~_keymap->numlock_mod_mask();
////	}
////
////	if (key == bind_page_quit) {
////		_mainloop.stop();
////	}
////
////	if(_grab_handler != nullptr) {
////		_grab_handler->key_press(e);
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_close) {
////		shared_ptr<client_managed_t> mw;
////		if (get_current_workspace()->client_focus_history_front(mw)) {
////			mw->delete_window(e->time);
////		}
////
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_exposay_all) {
////		auto child = filter_class<notebook_t>(get_current_workspace()->get_all_children());
////		for (auto c : child) {
////			c->start_exposay();
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_toggle_fullscreen) {
////		shared_ptr<client_managed_t> mw;
////		if (get_current_workspace()->client_focus_history_front(mw)) {
////			toggle_fullscreen(mw);
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_toggle_compositor) {
////		if (_compositor == nullptr) {
////			start_compositor();
////		} else {
////			stop_compositor();
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_right_desktop) {
////		unsigned new_desktop = ((_root->_current_desktop + _root->_desktop_list.size()) + 1) % _root->_desktop_list.size();
////		shared_ptr<client_managed_t> mw;
////		if (get_workspace(new_desktop)->client_focus_history_front(mw)) {
////			mw->activate();
////			set_focus(mw, e->time);
////		} else {
////			switch_to_desktop(new_desktop);
////			set_focus(nullptr, e->time);
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_left_desktop) {
////		unsigned new_desktop = ((_root->_current_desktop + _root->_desktop_list.size()) - 1) % _root->_desktop_list.size();
////		shared_ptr<client_managed_t> mw;
////		if (get_workspace(new_desktop)->client_focus_history_front(mw)) {
////			mw->activate();
////			set_focus(mw, e->time);
////		} else {
////			switch_to_desktop(new_desktop);
////			set_focus(nullptr, e->time);
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_bind_window) {
////		shared_ptr<client_managed_t> mw;
////		if (get_current_workspace()->client_focus_history_front(mw)) {
////			if (mw->is(MANAGED_FULLSCREEN)) {
////				unfullscreen(mw);
////			} else if (mw->is(MANAGED_FLOATING)) {
////				bind_window(mw, true);
////			}
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_fullscreen_window) {
////		shared_ptr<client_managed_t> mw;
////		if (get_current_workspace()->client_focus_history_front(mw)) {
////			if (not mw->is(MANAGED_FULLSCREEN)) {
////				fullscreen(mw);
////			}
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (key == bind_float_window) {
////		shared_ptr<client_managed_t> mw;
////		if (get_current_workspace()->client_focus_history_front(mw)) {
////			if (mw->is(MANAGED_FULLSCREEN)) {
////				unfullscreen(mw);
////			}
////
////			if (mw->is(MANAGED_NOTEBOOK)) {
////				unbind_window(mw);
////			}
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	if (_compositor != nullptr) {
////		if (key == bind_debug_1) {
////			if (_root->_fps_overlay == nullptr) {
////
////				auto v = get_current_workspace()->get_any_viewport();
////				int y_pos = v->allocation().y + v->allocation().h - 100;
////				int x_pos = v->allocation().x + (v->allocation().w - 400)/2;
////
////				_root->_fps_overlay = make_shared<compositor_overlay_t>(this, rect{x_pos, y_pos, 400, 100});
////				_root->push_back(_root->_fps_overlay);
////				_root->_fps_overlay->show();
////			} else {
////				_root->remove(_root->_fps_overlay);
////				_root->_fps_overlay = nullptr;
////			}
////			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////			return;
////		}
////
////		if (key == bind_debug_2) {
////			if (_compositor->show_damaged()) {
////				_compositor->set_show_damaged(false);
////			} else {
////				_compositor->set_show_damaged(true);
////			}
////			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////			return;
////		}
////
////		if (key == bind_debug_3) {
////			if (_compositor->show_opac()) {
////				_compositor->set_show_opac(false);
////			} else {
////				_compositor->set_show_opac(true);
////			}
////			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////			return;
////		}
////	}
////
////	if (key == bind_debug_4) {
////		_root->print_tree(0);
////		for (auto i : net_client_list()) {
////			switch (i->get_type()) {
////			case MANAGED_NOTEBOOK:
////				cout << "[" << i->orig() << "] notebook : " << i->title()
////						<< endl;
////				break;
////			case MANAGED_FLOATING:
////				cout << "[" << i->orig() << "] floating : " << i->title()
////						<< endl;
////				break;
////			case MANAGED_FULLSCREEN:
////				cout << "[" << i->orig() << "] fullscreen : " << i->title()
////						<< endl;
////				break;
////			case MANAGED_DOCK:
////				cout << "[" << i->orig() << "] dock : " << i->title() << endl;
////				break;
////			}
////		}
////
////		if(not global_focus_history_is_empty()) {
////			cout << "active window is : ";
////			for(auto & focus: global_client_focus_history()) {
////				cout << focus.lock()->orig() << ",";
////			}
////			cout << endl;
////		} else {
////			cout << "active window is : " << "NONE" << endl;
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	for(int i; i < bind_cmd.size(); ++i) {
////		if (key == bind_cmd[i].key) {
////			run_cmd(bind_cmd[i].cmd);
////			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////			return;
////		}
////	}
////
////	if (key.ks == XK_Tab and (key.mod == XCB_MOD_MASK_1)) {
////		if (_grab_handler == nullptr) {
////			start_alt_tab(e->time);
////		}
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
////		return;
////	}
////
////	xcb_allow_events(_dpy->xcb(), XCB_ALLOW_REPLAY_KEYBOARD, e->time);
//
//}
//
//void page_t::process_key_release_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_key_release_event_t const *>(_e);
//
//	if(_grab_handler != nullptr) {
//		_grab_handler->key_release(e);
//		return;
//	}
//
//}
//
///* Button event make page to grab pointer */
//void page_t::process_button_press_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_button_press_event_t const *>(_e);
//
//	/* TODO */
//
////	std::cout << "Button Event Press "
////			<< " event=" << e->event
////			<< " child=" << e->child
////			<< " root=" << e->root
////			<< " button=" << static_cast<int>(e->detail)
////			<< " mod1=" << (e->state & XCB_MOD_MASK_1 ? "true" : "false")
////			<< " mod2=" << (e->state & XCB_MOD_MASK_2 ? "true" : "false")
////			<< " mod3=" << (e->state & XCB_MOD_MASK_3 ? "true" : "false")
////			<< " mod4=" << (e->state & XCB_MOD_MASK_4 ? "true" : "false")
////			<< std::endl;
////
////
////	if(_grab_handler != nullptr) {
////		_grab_handler->button_press(e);
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_POINTER, e->time);
////		return;
////	}
////
////    if(e->root_x == 0 and e->root_y == 0) {
////        start_alt_tab(e->time);
////    } else {
////        _root->broadcast_button_press(e);
////    }
////
////	/**
////	 * if no change happened to process mode
////	 * We allow events (remove the grab), and focus those window.
////	 **/
////	if (_grab_handler == nullptr) {
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_REPLAY_POINTER, e->time);
////		auto mw = find_managed_window_with(e->event);
////		if (mw != nullptr) {
////			mw->activate();
////			set_focus(mw, e->time);
////		}
////		/* imediatly replay event, to reduce latency */
////		xcb_flush(_dpy->xcb());
////	} else {
////		/* Do not replay events, grab them and process them until Release Button */
////		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_POINTER, e->time);
////	}
//
//}
//
//void page_t::process_configure_notify_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_configure_notify_event_t const *>(_e);
//
////	//printf("configure (%d) %dx%d+%d+%d\n", e->window, e->width, e->height, e->x, e->y);
////
////	shared_ptr<client_base_t> c = find_client(e->window);
////	if(c != nullptr) {
////		c->process_event(e);
////	}
////
////	/** damage corresponding area **/
////	if(e->event == _dpy->root()) {
////		//add_compositor_damaged(_root->_root_position);
////	}
//
//}
//
///* track all created window */
//void page_t::process_create_notify_event(xcb_generic_event_t const * e) {
////	std::cout << format("08", e->sequence) << " create_notify " << e->width << "x" << e->height << "+" << e->x << "+" << e->y
////			<< " overide=" << (e->override_redirect?"true":"false")
////			<< " boder_width=" << e->border_width << std::endl;
//}
//
//void page_t::process_destroy_notify_event(xcb_generic_event_t const * _e) {
////	auto e = reinterpret_cast<xcb_destroy_notify_event_t const *>(_e);
////	auto c = find_client(e->window);
////	if (c != nullptr) {
////		if(typeid(*c.get()) == typeid(client_managed_t)) {
////			cout << "WARNING: client destroyed a window without sending synthetic unmap" << endl;
////			cout << "Sent Event: " << "false" << endl;
////			auto mw = dynamic_pointer_cast<client_managed_t>(c);
////			unmanage(mw);
////		} else if(typeid(*c) == typeid(client_not_managed_t)) {
////			cleanup_not_managed_client(dynamic_pointer_cast<client_not_managed_t>(c));
////		}
////
////	}
//}
//
//void page_t::process_gravity_notify_event(xcb_generic_event_t const * e) {
//	/* Ignore it, never happen ? */
//}
//
//void page_t::process_map_notify_event(xcb_generic_event_t const * _e) {
////	auto e = reinterpret_cast<xcb_map_notify_event_t const *>(_e);
////	/* if map event does not occur within root, ignore it */
////	if (e->event != _dpy->root())
////		return;
////	onmap(e->window);
//}
//
//void page_t::process_reparent_notify_event(xcb_generic_event_t const * _e) {
////	auto e = reinterpret_cast<xcb_reparent_notify_event_t const *>(_e);
////	//printf("Reparent window: %lu, parent: %lu, overide: %d, send_event: %d\n",
////	//		e.window, e.parent, e.override_redirect, e.send_event);
////	/* Reparent the root window ? hu :/ */
////	if(e->window == _dpy->root())
////		return;
////
////	/* If reparent occur on managed windows and new parent is an unknown window then unmanage */
////	auto mw = find_managed_window_with(e->window);
////	if (mw != nullptr) {
////		if (e->window == mw->orig() and e->parent != mw->base()) {
////			/* unmanage the window */
////			unmanage(mw);
////		}
////	}
////
////	/* if a unmanaged window leave the root window for any reason, this client is forgoten */
////	auto uw = dynamic_pointer_cast<client_not_managed_t>(find_client_with(e->window));
////	if(uw != nullptr and e->parent != _dpy->root()) {
////		cleanup_not_managed_client(uw);
////	}
//
//}
//
//void page_t::process_unmap_notify_event(xcb_generic_event_t const * _e) {
////	auto e = reinterpret_cast<xcb_unmap_notify_event_t const *>(_e);
////	auto c = find_client(e->window);
////	if (c != nullptr) {
////		//add_compositor_damaged(c->get_visible_region());
////		if(typeid(*c) == typeid(client_not_managed_t)) {
////			cleanup_not_managed_client(dynamic_pointer_cast<client_not_managed_t>(c));
////		} else if (typeid(*c) == typeid(client_managed_t)) {
////			auto mw = dynamic_pointer_cast<client_managed_t>(c);
////			if(c->base() == e->event) {
////				_dpy->reparentwindow(mw->orig(), _dpy->root(), 0.0, 0.0);
////				unmanage(mw);
////			}
////		}
////	}
//}
//
//void page_t::process_fake_unmap_notify_event(xcb_generic_event_t const * _e) {
////	auto e = reinterpret_cast<xcb_unmap_notify_event_t const *>(_e);
////	/**
////	 * Client must send a fake unmap event if he want get back the window.
////	 * (i.e. he want that we unmanage it.
////	 **/
////
////	/* if client is managed */
////	auto c = find_client(e->window);
////
////	if (c != nullptr) {
////		//add_compositor_damaged(c->get_visible_region());
////		if(typeid(*c) == typeid(client_managed_t)) {
////			auto mw = dynamic_pointer_cast<client_managed_t>(c);
////			_dpy->reparentwindow(mw->orig(), _dpy->root(), 0.0, 0.0);
////			unmanage(mw);
////		}
////		//render();
////	}
//}
//
//void page_t::process_circulate_request_event(xcb_generic_event_t const * _e) {
////	auto e = reinterpret_cast<xcb_circulate_request_event_t const *>(_e);
////	/* will happpen ? */
////	auto c = find_client_with(e->window);
////	if (c != nullptr) {
////		if (e->place == XCB_PLACE_ON_TOP) {
////			c->activate();
////		} else if (e->place == XCB_PLACE_ON_BOTTOM) {
////			_dpy->lower_window(e->window);
////		}
////	}
//}
//
//void page_t::process_configure_request_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_configure_request_event_t const *>(_e);
////	if (e.value_mask & CWX)
////		printf("has x: %d\n", e.x);
////	if (e.value_mask & CWY)
////		printf("has y: %d\n", e.y);
////	if (e.value_mask & CWWidth)
////		printf("has width: %d\n", e.width);
////	if (e.value_mask & CWHeight)
////		printf("has height: %d\n", e.height);
////	if (e.value_mask & CWSibling)
////		printf("has sibling: %lu\n", e.above);
////	if (e.value_mask & CWStackMode)
////		printf("has stack mode: %d\n", e.detail);
////	if (e.value_mask & CWBorderWidth)
////		printf("has border: %d\n", e.border_width);
//
//
////	auto c = find_client(e->window);
////
////	if (c != nullptr) {
////
////		//add_compositor_damaged(c->get_visible_region());
////
////		if(typeid(*c) == typeid(client_managed_t)) {
////
////			auto mw = dynamic_pointer_cast<client_managed_t>(c);
////
////			if ((e->value_mask & (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)) != 0) {
////
////				rect old_size = mw->get_floating_wished_position();
////				/** compute floating size **/
////				rect new_size = mw->get_floating_wished_position();
////
////				if (e->value_mask & XCB_CONFIG_WINDOW_X) {
////					new_size.x = e->x;
////				}
////
////				if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
////					new_size.y = e->y;
////				}
////
////				if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
////					new_size.w = e->width;
////				}
////
////				if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
////					new_size.h = e->height;
////				}
////
////				//printf("new_size = %s\n", new_size.to_std::string().c_str());
////
//////					if ((e.value_mask & (CWX)) and (e.value_mask & (CWY))
//////							and e.x == 0 and e.y == 0
//////							and !viewport_outputs.empty()) {
//////						viewport_t * v = viewport_outputs.begin()->second;
//////						i_rect b = v->raw_area();
//////						/* place on center */
//////						new_size.x = (b.w - new_size.w) / 2 + b.x;
//////						new_size.y = (b.h - new_size.h) / 2 + b.y;
//////					}
////
////				dimention_t<unsigned> final_size = mw->compute_size_with_constrain(new_size.w, new_size.h);
////
////				new_size.w = final_size.width;
////				new_size.h = final_size.height;
////
////				//printf("new_size = %s\n", new_size.to_string().c_str());
////
////				if (new_size != old_size) {
////					/** only affect floating windows **/
////					mw->set_floating_wished_position(new_size);
////					mw->reconfigure();
////				}
////			}
////
////		} else {
////			/** validate configure when window is not managed **/
////			ackwoledge_configure_request(e);
////		}
////
////	} else {
////		/** validate configure when window is not managed **/
////		ackwoledge_configure_request(e);
////	}
//
//}
//
//void page_t::ackwoledge_configure_request(xcb_configure_request_event_t const * e) {
//	//printf("ackwoledge_configure_request ");
//
////	int i = 0;
////	uint32_t value[7] = {0};
////	uint32_t mask = 0;
////	if(e->value_mask & XCB_CONFIG_WINDOW_X) {
////		mask |= XCB_CONFIG_WINDOW_X;
////		value[i++] = e->x;
////		//printf("x = %d ", e->x);
////	}
////
////	if(e->value_mask & XCB_CONFIG_WINDOW_Y) {
////		mask |= XCB_CONFIG_WINDOW_Y;
////		value[i++] = e->y;
////		//printf("y = %d ", e->y);
////	}
////
////	if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
////		mask |= XCB_CONFIG_WINDOW_WIDTH;
////		value[i++] = e->width;
////		//printf("w = %d ", e->width);
////	}
////
////	if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
////		mask |= XCB_CONFIG_WINDOW_HEIGHT;
////		value[i++] = e->height;
////		//printf("h = %d ", e->height);
////	}
////
////	if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
////		mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
////		value[i++] = e->border_width;
////		//printf("border = %d ", e->border_width);
////	}
////
////	if(e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
////		mask |= XCB_CONFIG_WINDOW_SIBLING;
////		value[i++] = e->sibling;
////		//printf("sibling = %d ", e->sibling);
////	}
////
////	if(e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
////		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
////		value[i++] = e->stack_mode;
////		//printf("stack_mode = %d ", e->stack_mode);
////	}
////
////	//printf("\n");
////
////	xcb_void_cookie_t ck = xcb_configure_window(_dpy->xcb(), e->window, mask, value);
//
//}
//
//void page_t::process_map_request_event(xcb_generic_event_t const * _e) {
////	auto e = reinterpret_cast<xcb_map_request_event_t const *>(_e);
////	if (e->parent != _dpy->root()) {
////		xcb_map_window(_dpy->xcb(), e->window);
////		return;
////	}
////
////	onmap(e->window);
//
//}
//
//void page_t::process_property_notify_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_property_notify_event_t const *>(_e);
////	if(e->window == _dpy->root())
////		return;
////
////	/** update the property **/
////	auto c = find_client(e->window);
////	auto mw = dynamic_pointer_cast<client_managed_t>(c);
////	if(mw == nullptr)
////		return;
////
////	mw->on_property_notify(e);
////
////	if (e->atom == A(_NET_WM_USER_TIME)) {
////		/* ignore */
////	} else if (e->atom == A(_NET_WM_STRUT_PARTIAL)) {
////		update_workarea();
////	} else if (e->atom == A(_NET_WM_STRUT)) {
////		update_workarea();
////	} else if (e->atom == A(_NET_WM_WINDOW_TYPE)) {
////		/* window type must be set on map, I guess it should never change ? */
////		/* update cache */
////
////		//window_t::page_window_type_e old = x->get_window_type();
////		//x->read_transient_for();
////		//x->find_window_type();
////		/* I do not see something in ICCCM */
////		//if(x->get_window_type() == window_t::PAGE_NORMAL_WINDOW_TYPE && old != window_t::PAGE_NORMAL_WINDOW_TYPE) {
////		//	manage_notebook(x);
////		//}
////	} else if (e->atom == A(WM_NORMAL_HINTS)) {
////		if (mw->is(MANAGED_NOTEBOOK)) {
////			find_parent_notebook_for(mw)->update_client_position(mw);
////		}
////
////		/* apply normal hint to floating window */
////		rect new_size = mw->get_wished_position();
////
////		dimention_t<unsigned> final_size = mw->compute_size_with_constrain(
////				new_size.w, new_size.h);
////		new_size.w = final_size.width;
////		new_size.h = final_size.height;
////		mw->set_floating_wished_position(new_size);
////		mw->reconfigure();
////	} else if (e->atom == A(WM_PROTOCOLS)) {
////		/* do nothing */
////	} else if (e->atom == A(WM_TRANSIENT_FOR)) {
////		safe_update_transient_for(mw);
////		_need_restack = true;
////	} else if (e->atom == A(WM_HINTS)) {
////		/* do nothing */
////	} else if (e->atom == A(_NET_WM_STATE)) {
////		/* this event are generated by page */
////		/* change of net_wm_state must be requested by client message */
////	} else if (e->atom == A(WM_STATE)) {
////		/** this is set by page ... don't read it **/
////	} else if (e->atom == A(_NET_WM_DESKTOP)) {
////		/* this set by page in most case */
////	} else if (e->atom == A(_MOTIF_WM_HINTS)) {
////		mw->reconfigure();
////	}
//
//}
//
//void page_t::process_fake_client_message_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_client_message_event_t const *>(_e);
//	//std::shared_ptr<char> name = cnx->get_atom_name(e->type);
//	//std::cout << "ClientMessage type = " << cnx->get_atom_name(e->type) << std::endl;
////
////	xcb_window_t w = e->window;
////	if (w == XCB_NONE)
////		return;
////
////	auto mw = find_managed_window_with(e->window);
////
////	if (e->type == A(_NET_ACTIVE_WINDOW)) {
////		if (mw != nullptr) {
////			mw->activate();
////			if (e->data.data32[1] == XCB_CURRENT_TIME) {
////				set_focus(mw, XCB_CURRENT_TIME);
////			} else {
////				set_focus(mw, e->data.data32[1]);
////			}
////		}
////	} else if (e->type == A(_NET_WM_STATE)) {
////
////		/* process first request */
////		process_net_vm_state_client_message(w, e->data.data32[0], e->data.data32[1]);
////		/* process second request */
////		process_net_vm_state_client_message(w, e->data.data32[0], e->data.data32[2]);
////
//////		for (int i = 1; i < 3; ++i) {
//////			if (std::find(supported_list.begin(), supported_list.end(),
//////					e->data.data32[i]) != supported_list.end()) {
//////				switch (e->data.data32[0]) {
//////				case _NET_WM_STATE_REMOVE:
//////					//w->unset_net_wm_state(e->data.l[i]);
//////					break;
//////				case _NET_WM_STATE_ADD:
//////					//w->set_net_wm_state(e->data.l[i]);
//////					break;
//////				case _NET_WM_STATE_TOGGLE:
//////					//w->toggle_net_wm_state(e->data.l[i]);
//////					break;
//////				}
//////			}
//////		}
////	} else if (e->type == A(WM_CHANGE_STATE)) {
////
////		/** When window want to become iconic, just bind them **/
////		if (mw != nullptr) {
////			if (mw->is(MANAGED_FLOATING) and e->data.data32[0] == IconicState) {
////				bind_window(mw, false);
////			} else if (mw->is(
////					MANAGED_NOTEBOOK) and e->data.data32[0] == IconicState) {
////				auto n = dynamic_pointer_cast<notebook_t>(mw->parent()->shared_from_this());
////				n->iconify_client(mw);
////			}
////		}
////
////	} else if (e->type == A(PAGE_QUIT)) {
////		_mainloop.stop();
////	} else if (e->type == A(WM_PROTOCOLS)) {
////
////	} else if (e->type == A(_NET_CLOSE_WINDOW)) {
////		if(mw != nullptr) {
////			mw->delete_window(e->data.data32[0]);
////		}
////	} else if (e->type == A(_NET_REQUEST_FRAME_EXTENTS)) {
////
////	} else if (e->type == A(_NET_WM_MOVERESIZE)) {
////		if (mw != nullptr) {
////			if (mw->is(MANAGED_FLOATING) and _grab_handler == nullptr) {
////
////				int root_x = e->data.data32[0];
////				int root_y = e->data.data32[1];
////				int direction = e->data.data32[2];
////				xcb_button_t button = static_cast<xcb_button_t>(e->data.data32[3]);
////				int source = e->data.data32[4];
////
////				if (direction == _NET_WM_MOVERESIZE_MOVE) {
////					grab_start(new grab_floating_move_t{this, mw, button, root_x, root_y});
////				} else {
////
////					if (direction == _NET_WM_MOVERESIZE_SIZE_TOP) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_TOP});
////					} else if (direction == _NET_WM_MOVERESIZE_SIZE_BOTTOM) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_BOTTOM});
////					} else if (direction == _NET_WM_MOVERESIZE_SIZE_LEFT) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_LEFT});
////					} else if (direction == _NET_WM_MOVERESIZE_SIZE_RIGHT) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_RIGHT});
////					} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPLEFT) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_TOP_LEFT});
////					} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPRIGHT) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_TOP_RIGHT});
////					} else if (direction
////							== _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_BOTTOM_LEFT});
////					} else if (direction
////							== _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT) {
////						grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_BOTTOM_RIGHT});
////					} else {
////						grab_start(new grab_floating_move_t{this, mw, button, root_x, root_y});
////					}
////				}
////
////				if (_grab_handler != nullptr) {
////					xcb_grab_pointer(_dpy->xcb(), false, _dpy->root(),
////							XCB_EVENT_MASK_BUTTON_PRESS
////									| XCB_EVENT_MASK_BUTTON_RELEASE
////									| XCB_EVENT_MASK_BUTTON_MOTION,
////							XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
////							XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
////				}
////
////			}
////		}
////	} else if (e->type == A(_NET_CURRENT_DESKTOP)) {
////		if(e->data.data32[0] >= 0 and e->data.data32[0] < _root->_desktop_list.size() and e->data.data32[0] != _root->_current_desktop) {
////			switch_to_desktop(e->data.data32[0]);
////			shared_ptr<client_managed_t> mw;
////			if (get_current_workspace()->client_focus_history_front(mw)) {
////				set_focus(mw, e->data.data32[1]);
////			} else {
////				set_focus(nullptr, e->data.data32[1]);
////			}
////		}
////	}
//}
//
//void page_t::process_damage_notify_event(xcb_generic_event_t const * e) {
//
//}

//void page_t::render() {
//
//	/* TODO: remove */
//
////	// ask to update everything to draw the time64_t::now() frame
////	_root->broadcast_update_layout(time64_t::now());
////	// ask to flush all pending drawing
////	_root->broadcast_trigger_redraw();
////	// render on screen if we need too.
////	if (_compositor != nullptr) {
////		// TODO or remove.
////		//_compositor->render(_root.get());
////	}
////	xcb_flush(_dpy->xcb());
////	_root->broadcast_render_finished();
//}

//void page_t::fullscreen(shared_ptr<xdg_surface_toplevel_t> mw) {
//
//	if(mw->is(MANAGED_FULLSCREEN))
//		return;
//
//	shared_ptr<viewport_t> v;
//	if(mw->is(MANAGED_NOTEBOOK)) {
//		v = find_viewport_of(mw);
//	} else if (mw->is(MANAGED_FLOATING)) {
//		v = get_current_workspace()->get_any_viewport();
//	} else {
//		cout << "WARNING: a dock trying to become fullscreen" << endl;
//		return;
//	}
//
//	fullscreen(mw, v);
//}
//
//void page_t::fullscreen(shared_ptr<xdg_surface_toplevel_t> mw, shared_ptr<viewport_t> v) {
//	assert(v != nullptr);
//
//	if(mw->is(MANAGED_FULLSCREEN))
//		return;
//
//	/* WARNING: Call order is important, change it with caution */
//
//	fullscreen_data_t data;
//
//	if(mw->is(MANAGED_NOTEBOOK)) {
//		/**
//		 * if the current window is managed in notebook:
//		 *
//		 * 1. search for the current notebook,
//		 * 2. search the viewport for this notebook, and use it as default
//		 *    fullscreen host or use the first available viewport.
//		 **/
//		data.revert_type = MANAGED_NOTEBOOK;
//		data.revert_notebook = find_parent_notebook_for(mw);
//	} else if (mw->is(MANAGED_FLOATING)) {
//		data.revert_type = MANAGED_FLOATING;
//		data.revert_notebook.reset();
//	} else {
//		cout << "WARNING: a dock trying to become fullscreen" << endl;
//		return;
//	}
//
//	auto workspace = find_desktop_of(v);
//
//	detach(mw);
//
//	// unfullscreen client that already use this screen
//	for (auto &x : _fullscreen_client_to_viewport) {
//		if (x.second.viewport.lock() == v) {
//			unfullscreen(x.second.client.lock());
//			break;
//		}
//	}
//
//	data.client = mw;
//	data.workspace = workspace;
//	data.viewport = v;
//
//	_fullscreen_client_to_viewport[mw.get()] = data;
//
//	//mw->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);
//	mw->set_managed_type(MANAGED_FULLSCREEN);
//	workspace->attach(mw);
//
//	/* it's a trick */
//	mw->set_notebook_wished_position(v->raw_area());
//	mw->reconfigure();
//	mw->normalize();
//	mw->show();
//
//	/* hide the viewport because he is covered by a fullscreen client */
//	v->hide();
//	_need_restack = true;
//}
//
//void page_t::unfullscreen(shared_ptr<xdg_surface_toplevel_t> mw) {
//	/* WARNING: Call order is important, change it with caution */
//
//	/** just in case **/
//	//mw->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
//
//	if(!has_key(_fullscreen_client_to_viewport, mw.get()))
//		return;
//
//	detach(mw);
//
//	fullscreen_data_t data = _fullscreen_client_to_viewport[mw.get()];
//	_fullscreen_client_to_viewport.erase(mw.get());
//
//	shared_ptr<workspace_t> d;
//
//	if(data.workspace.expired()) {
//		d = get_current_workspace();
//	} else {
//		d = data.workspace.lock();
//	}
//
//	shared_ptr<viewport_t> v;
//
//	if(data.viewport.expired()) {
//		v = d->get_any_viewport();
//	} else {
//		v = data.viewport.lock();
//	}
//
//	if (data.revert_type == MANAGED_NOTEBOOK) {
//		shared_ptr<notebook_t> n;
//		if(data.revert_notebook.expired()) {
//			n = d->default_pop();
//		} else {
//			n = data.revert_notebook.lock();
//		}
//		mw->set_managed_type(MANAGED_NOTEBOOK);
//		n->add_client(mw, true);
//		mw->reconfigure();
//	} else {
//		mw->set_managed_type(MANAGED_FLOATING);
//		insert_in_tree_using_transient_for(mw);
//		mw->reconfigure();
//	}
//
//	if(d->is_visible() and not v->is_visible()) {
//		v->show();
//	}
//
//	update_workarea();
//
//	_need_restack = true;
//
//}
//
//void page_t::toggle_fullscreen(shared_ptr<xdg_surface_toplevel_t> c) {
//	if(c->is(MANAGED_FULLSCREEN))
//		unfullscreen(c);
//	else
//		fullscreen(c);
//}


//void page_t::process_event(xcb_generic_event_t const * e) {
//	auto x = _event_handlers.find(e->response_type);
//	if(x != _event_handlers.end()) {
//		if(x->second != nullptr) {
//			(this->*(x->second))(e);
//		}
//	} else {
//		//std::cout << "not handled event: " << cnx->event_type_name[(e->response_type&(~0x80))] << (e->response_type&(0x80)?" (fake)":"") << std::endl;
//	}
//}

void page_t::insert_window_in_notebook(
		xdg_surface_toplevel_view_p x,
		notebook_p n,
		bool prefer_activate) {
	assert(x != nullptr);
	if (n == nullptr)
		n = get_current_workspace()->default_pop();
	assert(n != nullptr);
	n->add_client(x, prefer_activate);
}

/* update viewport and childs allocation */
//void page_t::update_workarea() {
////	for (auto d : _root->_desktop_list) {
////		for (auto v : d->get_viewports()) {
////			compute_viewport_allocation(d, v);
////		}
////		d->set_workarea(d->primary_viewport()->allocation());
////	}
////
////	std::vector<uint32_t> workarea_data(_root->_desktop_list.size()*4);
////	for(unsigned k = 0; k < _root->_desktop_list.size(); ++k) {
////		workarea_data[k*4+0] = _root->_desktop_list[k]->workarea().x;
////		workarea_data[k*4+1] = _root->_desktop_list[k]->workarea().y;
////		workarea_data[k*4+2] = _root->_desktop_list[k]->workarea().w;
////		workarea_data[k*4+3] = _root->_desktop_list[k]->workarea().h;
////	}
////
////	_dpy->change_property(_dpy->root(), _NET_WORKAREA, CARDINAL, 32,
////			&workarea_data[0], workarea_data.size());
//
//}

void page_t::set_focus(weston_pointer * pointer,
		shared_ptr<xdg_surface_toplevel_view_t> new_focus) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	assert(new_focus != nullptr);
	assert(new_focus->get_default_view() != nullptr);

	if(!_current_focus.expired()) {
		_current_focus.lock()->set_focus_state(false);
	}

	get_current_workspace()->client_focus_history_move_front(new_focus);
	global_focus_history_move_front(new_focus);
	new_focus->activate();
	new_focus->set_focus_state(true);
	//weston_pointer_set_focus(pointer, new_focus->get_default_view(), 0, 0);

//	auto keyboard = weston_seat_get_keyboard(pointer->seat);
//	if(keyboard)
//		weston_keyboard_set_focus(keyboard, new_focus->surface());
	weston_seat_set_keyboard_focus(pointer->seat, new_focus->xdg_surface()->surface());

	_current_focus = new_focus;

	sync_tree_view();

}

//void page_t::clear_focus(weston_pointer * pointer) {
//	/* if we want to defocus something */
//	if(conf()._auto_refocus) {
//		if (get_current_workspace()->client_focus_history_front(new_focus)) {
//			new_focus->activate();
//			get_current_workspace()->client_focus_history_move_front(new_focus);
//			global_focus_history_move_front(new_focus);
//			new_focus->activate();
//			new_focus->set_focus_state(true);
//			_current_focus = new_focus;
//		} else {
//			_current_focus.reset();
//			weston_pointer_clear_focus()
//		}
//	} else {
//		_net_active_window.reset();
//		_dpy->set_input_focus(identity_window, XCB_INPUT_FOCUS_NONE, tfocus);
//		_dpy->set_net_active_window(XCB_WINDOW_NONE);
//	}
//
//	sync_tree_view();
//
//}

void page_t::split_left(notebook_p nbk, xdg_surface_toplevel_view_p c) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(this);
	auto split = make_shared<split_t>(this, VERTICAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
	split->show();
}

void page_t::split_right(notebook_p nbk, xdg_surface_toplevel_view_p c) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(this);
	auto split = make_shared<split_t>(this, VERTICAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
	split->show();
}

void page_t::split_top(notebook_p nbk, xdg_surface_toplevel_view_p c) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(this);
	auto split = make_shared<split_t>(this, HORIZONTAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
	split->show();
}

void page_t::split_bottom(notebook_p nbk, xdg_surface_toplevel_view_p c) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(this);
	auto split = make_shared<split_t>(this, HORIZONTAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
	split->show();
}

/*
 * This function will close the given notebook, if possible
 */
void page_t::notebook_close(notebook_p nbk) {
	/**
	 * Closing notebook mean destroying the split base of this
	 * notebook, plus this notebook.
	 **/

	assert(nbk->parent() != nullptr);

	auto splt = dynamic_pointer_cast<split_t>(nbk->parent()->shared_from_this());

	/* if parent is viewport then we cannot close current notebook */
	if(splt == nullptr)
		return;

	assert(nbk == splt->get_pack0() or nbk == splt->get_pack1());

	/* find the sibling branch of note that we want close */
	auto dst = dynamic_pointer_cast<page_component_t>((nbk == splt->get_pack0()) ? splt->get_pack1() : splt->get_pack0());

	assert(dst != nullptr);

	/* remove this split from tree  and replace it by sibling branch */
	detach(dst);
	dynamic_pointer_cast<page_component_t>(splt->parent()->shared_from_this())->replace(splt, dst);

	/**
	 * if notebook that we want destroy was the default_pop, select
	 * a new one.
	 **/
	if (get_current_workspace()->default_pop() == nbk) {
		get_current_workspace()->update_default_pop();
		/* damage the new default pop to show the notebook mark properly */
	}

	/* move all client from destroyed notebook to new default pop */
	auto clients = filter_class<xdg_surface_toplevel_view_t>(nbk->children());
//	bool notebook_has_focus = false;
	for(auto i : clients) {
//		if(i->has_focus())
//			notebook_has_focus = true;
		nbk->remove(i);
		insert_window_in_notebook(i, nullptr, false);
	}

	/**
	 * if a fullscreen client want revert to this notebook,
	 * change it to default_window_pop
	 **/
	for (auto & i : _fullscreen_client_to_viewport) {
		if (i.second.revert_notebook.lock() == nbk) {
			i.second.revert_notebook = _root->_desktop_list[_root->_current_desktop]->default_pop();
		}
	}

//	if(notebook_has_focus) {
//		set_focus(nullptr, XCB_CURRENT_TIME);
//	}

}

///*
// * Compute the usable desktop area and dock allocation.
// */
//void page_t::compute_viewport_allocation(shared_ptr<workspace_t> d, shared_ptr<viewport_t> v) {
//
////	/* Partial struct content definition */
////	enum : uint32_t {
////		PS_LEFT = 0,
////		PS_RIGHT = 1,
////		PS_TOP = 2,
////		PS_BOTTOM = 3,
////		PS_LEFT_START_Y = 4,
////		PS_LEFT_END_Y = 5,
////		PS_RIGHT_START_Y = 6,
////		PS_RIGHT_END_Y = 7,
////		PS_TOP_START_X = 8,
////		PS_TOP_END_X = 9,
////		PS_BOTTOM_START_X = 10,
////		PS_BOTTOM_END_X = 11,
////		PS_LAST = 12
////	};
////
////	rect const raw_area = v->raw_area();
////
////	int margin_left = _root->_root_position.x + raw_area.x;
////	int margin_top = _root->_root_position.y + raw_area.y;
////	int margin_right = _root->_root_position.w - raw_area.x - raw_area.w;
////	int margin_bottom = _root->_root_position.h - raw_area.y - raw_area.h;
////
////	auto children = filter_class<xdg_surface_base_t>(d->get_all_children());
////	for(auto j: children) {
////		int32_t ps[PS_LAST];
////		bool has_strut{false};
////
////		if(j->net_wm_strut_partial() != nullptr) {
////			if(j->net_wm_strut_partial()->size() == 12) {
////				std::copy(j->net_wm_strut_partial()->begin(), j->net_wm_strut_partial()->end(), &ps[0]);
////				has_strut = true;
////			}
////		}
////
////		if (j->net_wm_strut() != nullptr and not has_strut) {
////			if(j->net_wm_strut()->size() == 4) {
////
////				/** if strut is found, fake strut_partial **/
////
////				std::copy(j->net_wm_strut()->begin(), j->net_wm_strut()->end(), &ps[0]);
////
////				if(ps[PS_TOP] > 0) {
////					ps[PS_TOP_START_X] = _root->_root_position.x;
////					ps[PS_TOP_END_X] = _root->_root_position.x + _root->_root_position.w;
////				}
////
////				if(ps[PS_BOTTOM] > 0) {
////					ps[PS_BOTTOM_START_X] = _root->_root_position.x;
////					ps[PS_BOTTOM_END_X] = _root->_root_position.x + _root->_root_position.w;
////				}
////
////				if(ps[PS_LEFT] > 0) {
////					ps[PS_LEFT_START_Y] = _root->_root_position.y;
////					ps[PS_LEFT_END_Y] = _root->_root_position.y + _root->_root_position.h;
////				}
////
////				if(ps[PS_RIGHT] > 0) {
////					ps[PS_RIGHT_START_Y] = _root->_root_position.y;
////					ps[PS_RIGHT_END_Y] = _root->_root_position.y + _root->_root_position.h;
////				}
////
////				has_strut = true;
////			}
////		}
////
////		if (has_strut) {
////
////			if (ps[PS_LEFT] > 0) {
////				/* check if raw area intersect current viewport */
////				rect b(0, ps[PS_LEFT_START_Y], ps[PS_LEFT],
////						ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1);
////				rect x = raw_area & b;
////				if (!x.is_null()) {
////					margin_left = std::max(margin_left, ps[PS_LEFT]);
////				}
////			}
////
////			if (ps[PS_RIGHT] > 0) {
////				/* check if raw area intersect current viewport */
////				rect b(_root->_root_position.w - ps[PS_RIGHT],
////						ps[PS_RIGHT_START_Y], ps[PS_RIGHT],
////						ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1);
////				rect x = raw_area & b;
////				if (!x.is_null()) {
////					margin_right = std::max(margin_right, ps[PS_RIGHT]);
////				}
////			}
////
////			if (ps[PS_TOP] > 0) {
////				/* check if raw area intersect current viewport */
////				rect b(ps[PS_TOP_START_X], 0,
////						ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1, ps[PS_TOP]);
////				rect x = raw_area & b;
////				if (!x.is_null()) {
////					margin_top = std::max(margin_top, ps[PS_TOP]);
////				}
////			}
////
////			if (ps[PS_BOTTOM] > 0) {
////				/* check if raw area intersect current viewport */
////				rect b(ps[PS_BOTTOM_START_X],
////						_root->_root_position.h - ps[PS_BOTTOM],
////						ps[PS_BOTTOM_END_X] - ps[PS_BOTTOM_START_X] + 1,
////						ps[PS_BOTTOM]);
////				rect x = raw_area & b;
////				if (!x.is_null()) {
////					margin_bottom = std::max(margin_bottom, ps[PS_BOTTOM]);
////				}
////			}
////		}
////	}
////
////	rect final_size;
////
////	final_size.x = margin_left;
////	final_size.w = _root->_root_position.w - margin_right - margin_left;
////	final_size.y = margin_top;
////	final_size.h = _root->_root_position.h - margin_bottom - margin_top;
////
////	v->set_allocation(final_size);
//
//}
//
///*
// * Reconfigure docks.
// */
//void page_t::reconfigure_docks(shared_ptr<workspace_t> const & d) {
//
////	/* Partial struct content definition */
////	enum {
////		PS_LEFT = 0,
////		PS_RIGHT = 1,
////		PS_TOP = 2,
////		PS_BOTTOM = 3,
////		PS_LEFT_START_Y = 4,
////		PS_LEFT_END_Y = 5,
////		PS_RIGHT_START_Y = 6,
////		PS_RIGHT_END_Y = 7,
////		PS_TOP_START_X = 8,
////		PS_TOP_END_X = 9,
////		PS_BOTTOM_START_X = 10,
////		PS_BOTTOM_END_X = 11,
////	};
////
////	auto children = filter_class<xdg_surface_toplevel_t>(d->get_all_children());
////	for(auto j: children) {
////
////		if(not j->is(MANAGED_DOCK))
////			continue;
////
////		int32_t ps[12] = { 0 };
////		bool has_strut{false};
////
////		if(j->net_wm_strut_partial() != nullptr) {
////			if(j->net_wm_strut_partial()->size() == 12) {
////				std::copy(j->net_wm_strut_partial()->begin(), j->net_wm_strut_partial()->end(), &ps[0]);
////				has_strut = true;
////			}
////		}
////
////		if (j->net_wm_strut() != nullptr and not has_strut) {
////			if(j->net_wm_strut()->size() == 4) {
////
////				/** if strut is found, fake strut_partial **/
////
////				std::copy(j->net_wm_strut()->begin(), j->net_wm_strut()->end(), &ps[0]);
////
////				if(ps[PS_TOP] > 0) {
////					ps[PS_TOP_START_X] = _root->_root_position.x;
////					ps[PS_TOP_END_X] = _root->_root_position.x + _root->_root_position.w;
////				}
////
////				if(ps[PS_BOTTOM] > 0) {
////					ps[PS_BOTTOM_START_X] = _root->_root_position.x;
////					ps[PS_BOTTOM_END_X] = _root->_root_position.x + _root->_root_position.w;
////				}
////
////				if(ps[PS_LEFT] > 0) {
////					ps[PS_LEFT_START_Y] = _root->_root_position.y;
////					ps[PS_LEFT_END_Y] = _root->_root_position.y + _root->_root_position.h;
////				}
////
////				if(ps[PS_RIGHT] > 0) {
////					ps[PS_RIGHT_START_Y] = _root->_root_position.y;
////					ps[PS_RIGHT_END_Y] = _root->_root_position.y + _root->_root_position.h;
////				}
////
////				has_strut = true;
////			}
////		}
////
////		if (has_strut) {
////
////			if (ps[PS_LEFT] > 0) {
////				rect pos;
////				pos.x = 0;
////				pos.y = ps[PS_LEFT_START_Y];
////				pos.w = ps[PS_LEFT];
////				pos.h = ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1;
////				j->set_floating_wished_position(pos);
////				j->normalize();
////				j->show();
////				continue;
////			}
////
////			if (ps[PS_RIGHT] > 0) {
////				rect pos;
////				pos.x = _root->_root_position.w - ps[PS_RIGHT];
////				pos.y = ps[PS_RIGHT_START_Y];
////				pos.w = ps[PS_RIGHT];
////				pos.h = ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1;
////				j->set_floating_wished_position(pos);
////				j->normalize();
////				j->show();
////				continue;
////			}
////
////			if (ps[PS_TOP] > 0) {
////				rect pos;
////				pos.x = ps[PS_TOP_START_X];
////				pos.y = 0;
////				pos.w = ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1;
////				pos.h = ps[PS_TOP];
////				j->set_floating_wished_position(pos);
////				j->normalize();
////				j->show();
////				continue;
////			}
////
////			if (ps[PS_BOTTOM] > 0) {
////				rect pos;
////				pos.x = ps[PS_BOTTOM_START_X];
////				pos.y = _root->_root_position.h - ps[PS_BOTTOM];
////				pos.w = ps[PS_BOTTOM_END_X] - ps[PS_BOTTOM_START_X] + 1;
////				pos.h = ps[PS_BOTTOM];
////				j->set_floating_wished_position(pos);
////				j->normalize();
////				j->show();
////				continue;
////			}
////		}
////	}
//}

//void page_t::process_net_vm_state_client_message(xcb_window_t c, long type, xcb_atom_t state_properties) {
//	if(state_properties == XCB_ATOM_NONE)
//		return;
//
//	/* debug print */
////	if(true) {
////		char const * action;
////		switch (type) {
////		case _NET_WM_STATE_REMOVE:
////			action = "remove";
////			break;
////		case _NET_WM_STATE_ADD:
////			action = "add";
////			break;
////		case _NET_WM_STATE_TOGGLE:
////			action = "toggle";
////			break;
////		default:
////			action = "invalid";
////			break;
////		}
////		std::cout << "_NET_WM_STATE: " << action << " "
////				<< cnx->get_atom_name(state_properties) << std::endl;
////	}
//
//	auto mw = find_managed_window_with(c);
//	if(mw == nullptr)
//		return;
//
//	if (mw->is(MANAGED_NOTEBOOK)) {
//
//		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				break;
//			case _NET_WM_STATE_ADD:
//				fullscreen(mw);
//				update_desktop_visibility();
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				toggle_fullscreen(mw);
//				update_desktop_visibility();
//				break;
//			}
//			update_workarea();
//		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE: {
//				auto n = dynamic_pointer_cast<notebook_t>(mw->parent()->shared_from_this());
//				if (n != nullptr) {
//					mw->activate();
//					set_focus(mw, XCB_CURRENT_TIME);
//				}
//			}
//
//				break;
//			case _NET_WM_STATE_ADD:
//				mw->iconify();
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				/** IWMH say ignore it ? **/
//			default:
//				break;
//			}
//		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				mw->set_demands_attention(false);
//				mw->queue_redraw();
//				break;
//			case _NET_WM_STATE_ADD:
//				mw->set_demands_attention(true);
//				mw->queue_redraw();
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				mw->set_demands_attention(not mw->demands_attention());
//				mw->queue_redraw();
//				break;
//			default:
//				break;
//			}
//		}
//	} else if (mw->is(MANAGED_FLOATING)) {
//
//		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				break;
//			case _NET_WM_STATE_ADD:
//				fullscreen(mw);
//				update_desktop_visibility();
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				toggle_fullscreen(mw);
//				update_desktop_visibility();
//				break;
//			}
//			update_workarea();
//		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				mw->activate();
//				set_focus(mw, XCB_CURRENT_TIME);
//				break;
//			case _NET_WM_STATE_ADD:
//				/** I ignore it **/
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				/** IWMH say ignore it ? **/
//			default:
//				break;
//			}
//		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				mw->set_demands_attention(false);
//				mw->queue_redraw();
//				break;
//			case _NET_WM_STATE_ADD:
//				mw->set_demands_attention(true);
//				mw->queue_redraw();
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				mw->set_demands_attention(not mw->demands_attention());
//				mw->queue_redraw();
//				break;
//			default:
//				break;
//			}
//		}
//	} else if (mw->is(MANAGED_DOCK)) {
//
//		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				break;
//			case _NET_WM_STATE_ADD:
//				//fullscreen(mw);
//				//update_desktop_visibility();
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				//toggle_fullscreen(mw);
//				//update_desktop_visibility();
//				break;
//			}
//			update_workarea();
//		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				mw->activate();
//				break;
//			case _NET_WM_STATE_ADD:
//				/** I ignore it **/
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				/** IWMH say ignore it ? **/
//			default:
//				break;
//			}
//		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				mw->set_demands_attention(false);
//				mw->queue_redraw();
//				break;
//			case _NET_WM_STATE_ADD:
//				mw->set_demands_attention(true);
//				mw->queue_redraw();
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				mw->set_demands_attention(not mw->demands_attention());
//				mw->queue_redraw();
//				break;
//			default:
//				break;
//			}
//		}
//	} else if (mw->is(MANAGED_FULLSCREEN)) {
//		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				unfullscreen(mw);
//				update_desktop_visibility();
//				break;
//			case _NET_WM_STATE_ADD:
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				toggle_fullscreen(mw);
//				update_desktop_visibility();
//				break;
//			}
//			update_workarea();
//		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				break;
//			case _NET_WM_STATE_ADD:
//				break;
//			case _NET_WM_STATE_TOGGLE:
//			default:
//				break;
//			}
//		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
//			switch (type) {
//			case _NET_WM_STATE_REMOVE:
//				mw->set_demands_attention(false);
//				break;
//			case _NET_WM_STATE_ADD:
//				mw->set_demands_attention(true);
//				break;
//			case _NET_WM_STATE_TOGGLE:
//				mw->set_demands_attention(not mw->demands_attention());
//				break;
//			default:
//				break;
//			}
//		}
//	}
//}
//
void page_t::insert_in_tree_using_transient_for(xdg_surface_toplevel_view_p c) {

//	auto transient_for = c->transient_for();
//	if(transient_for != nullptr and not transient_for->master_view().expired()) {
//		transient_for->master_view().lock()->add_transient_child(c);
//	} else {
//		bind_window(c, true);
//
////		auto mw = dynamic_pointer_cast<xdg_surface_toplevel_t>(c);
////
////		if (mw != nullptr) {
////			int workspace = find_current_desktop(c);
////			if(workspace >= 0 and workspace < get_workspace_count()) {
////				get_workspace(workspace)->attach(mw);
////			} else {
////				get_current_workspace()->attach(mw);
////			}
////		} else {
////			_root->root_subclients->push_back(c);
////		}
//	}

	c->set_managed_type(MANAGED_FLOATING);
	_root->root_subclients->push_back(c);
	c->update_view();
	sync_tree_view();

}

//shared_ptr<xdg_surface_base_t> page_t::get_transient_for(
//		shared_ptr<xdg_surface_base_t> c) {
////	assert(c != nullptr);
////	shared_ptr<xdg_surface_base_t> transient_for = nullptr;
////	if (c->wm_transient_for() != nullptr) {
////		transient_for = find_client_with(*(c->wm_transient_for()));
////		if (transient_for == nullptr)
////			printf("Warning transient for an unknown client\n");
////	}
////	return transient_for;
//}

void page_t::detach(shared_ptr<tree_t> t) {
	assert(t != nullptr);

	/** detach a tree_t will cause it to be restacked, at less **/
	//add_global_damage(t->get_visible_region());
	if(t->parent() != nullptr) {

		/**
		 * Keeping a client in the focus history of wrong workspace
		 * trouble the workspace switching, because when a switch occur,
		 * page look within the workspace focus history to choose the
		 * proper client to re-focus and may re-focus that is not belong the
		 * new workspace.
		 **/
		if(typeid(*t.get()) == typeid(xdg_surface_toplevel_view_t)) {
			auto x = dynamic_pointer_cast<xdg_surface_toplevel_view_t>(t);
			for(auto w: _root->_desktop_list)
				w->client_focus_history_remove(x);
		}

		t->parent()->remove(t);
		if(t->parent() != nullptr) {
			printf("XXX %s\n", typeid(*t->parent()).name());
			assert(false);
		}
	}
}

void page_t::fullscreen_client_to_viewport(xdg_surface_toplevel_view_p c, viewport_p v) {
//	if (has_key(_fullscreen_client_to_viewport, c.get())) {
//		fullscreen_data_t & data = _fullscreen_client_to_viewport[c.get()];
//		if (v != data.viewport.lock()) {
//			if(not data.viewport.expired()) {
//				data.viewport.lock()->show();
//				data.viewport.lock()->queue_redraw();
//				//add_global_damage(data.viewport.lock()->raw_area());
//			}
//			v->hide();
//			//add_global_damage(v->raw_area());
//			data.viewport = v;
//			//data.workspace = find_desktop_of(v);
//			c->set_notebook_wished_position(v->raw_area());
//			c->reconfigure();
//			//update_desktop_visibility();
//		}
//	}
}

void page_t::bind_window(xdg_surface_toplevel_view_p mw, bool activate) {
	detach(mw);
	insert_window_in_notebook(mw, nullptr, activate);
}

void page_t::unbind_window(xdg_surface_toplevel_view_p mw) {
	detach(mw);
	mw->set_managed_type(MANAGED_FLOATING);
	insert_in_tree_using_transient_for(mw);
	mw->queue_redraw();
	//mw->normalize();
	mw->show();
	mw->activate();
}

/* look for a notebook in tree base, that is deferent from nbk */
shared_ptr<notebook_t> page_t::get_another_notebook(shared_ptr<tree_t> base, shared_ptr<tree_t> nbk) {
	vector<shared_ptr<notebook_t>> l;

	if (base == nullptr) {
		l = filter_class<notebook_t>(_root->get_all_children());
	} else {
		l = filter_class<notebook_t>(base->get_all_children());;
	}

	if (!l.empty()) {
		if (l.front() != nbk)
			return l.front();
		if (l.back() != nbk)
			return l.back();
	}

	return nullptr;

}

shared_ptr<notebook_t> page_t::find_parent_notebook_for(shared_ptr<xdg_surface_toplevel_view_t> mw) {
	return dynamic_pointer_cast<notebook_t>(mw->parent()->shared_from_this());
}

shared_ptr<workspace_t> page_t::find_desktop_of(shared_ptr<tree_t> n) {
	shared_ptr<tree_t> x = n;
	while (x != nullptr) {
		auto ret = dynamic_pointer_cast<workspace_t>(x);
		if (ret != nullptr)
			return ret;

		if (x->parent() != nullptr)
			x = (x->parent()->shared_from_this());
		else
			return nullptr;
	}
	return nullptr;
}

void page_t::update_windows_stack() {
	sync_tree_view();
}

/**
 * This function will update viewport layout on xrandr events.
 *
 * It cut the visible outputs area in rectangle, where viewport will cover. The
 * rule is that the first output get the area first, the last one is cut in
 * sub-rectangle that do not overlap previous allocated area.
 **/
void page_t::update_viewport_layout() {

	/* TODO: based on output list */
//
//	_left_most_border = std::numeric_limits<int>::max();
//	_top_most_border = std::numeric_limits<int>::max();


	/* compute the extends of all outputs */
	rect outputs_extends{numeric_limits<int>::max(), numeric_limits<int>::max(),
		numeric_limits<int>::min(), numeric_limits<int>::min()};

	for(auto o: _outputs) {
		outputs_extends.x = std::min(outputs_extends.x, o->x);
		outputs_extends.y = std::min(outputs_extends.y, o->y);
		outputs_extends.w = std::max(outputs_extends.w, o->x+o->width);
		outputs_extends.h = std::max(outputs_extends.h, o->y+o->height);
	}

	outputs_extends.w -= outputs_extends.x;
	outputs_extends.h -= outputs_extends.y;

	_root->_root_position = outputs_extends;

//	map<xcb_randr_crtc_t, xcb_randr_get_crtc_info_reply_t *> crtc_info;
//
//	vector<xcb_randr_get_crtc_info_cookie_t> ckx(xcb_randr_get_screen_resources_crtcs_length(randr_resources));
//	xcb_randr_crtc_t * crtc_list = xcb_randr_get_screen_resources_crtcs(randr_resources);
//	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
//		ckx[k] = xcb_randr_get_crtc_info(_dpy->xcb(), crtc_list[k], XCB_CURRENT_TIME);
//	}
//
//	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
//		xcb_randr_get_crtc_info_reply_t * r = xcb_randr_get_crtc_info_reply(_dpy->xcb(), ckx[k], 0);
//		if(r != nullptr) {
//			crtc_info[crtc_list[k]] = r;
//		}
//
//		// keep left more screen to move iconnified window there
//		if(r->x < _left_most_border) {
//			_left_most_border = r->x;
//		}
//
//		if(r->y < _top_most_border) {
//			_top_most_border = r->y;
//		}
//
//	}

	/* compute all viewport  that does not overlap and cover the full area of
	 * outputs */

	/* list of future viewport locations */
	vector<pair<weston_output *, rect>> viewport_allocation;

	/* start with not allocated area */
	region already_allocated;
	for(auto o: _outputs) {
		/* the location of outputs */
		region location{o->x, o->y, o->width, o->height};
		/* remove overlapped areas */
		location -= already_allocated;
		/* for remaining rectangles, allocate a viewport */
		for(auto & b: location.rects()) {
			viewport_allocation.push_back(make_pair(o, b));
		}
		already_allocated += location;
	}

	/* for each desktop we update the list of viewports, without destroying
	 * existing is not nessesary */
	for(auto d: _root->_desktop_list) {
		//d->set_allocation(_root->_root_position);
		/** get old layout to recycle old viewport, and keep unchanged outputs **/
		vector<shared_ptr<viewport_t>> old_layout = d->get_viewport_map();
		/** store the newer layout, to be able to cleanup obsolete viewports **/
		vector<shared_ptr<viewport_t>> new_layout;
		/** for each not overlaped rectangle **/
		for(unsigned i = 0; i < viewport_allocation.size(); ++i) {
			printf("%d: found viewport (%d,%d,%d,%d)\n", d->id(),
					viewport_allocation[i].second.x, viewport_allocation[i].second.y,
					viewport_allocation[i].second.w, viewport_allocation[i].second.h);
			shared_ptr<viewport_t> vp;
			if(i < old_layout.size()) {
				vp = old_layout[i];
				vp->set_raw_area(viewport_allocation[i].second);
			} else {
				vp = make_shared<viewport_t>(this, viewport_allocation[i].second, viewport_allocation[i].first);
			}
			new_layout.push_back(vp);
		}

		d->set_layout(new_layout);
		d->update_default_pop();

		/** clean up obsolete layout **/
		for (unsigned i = new_layout.size(); i < old_layout.size(); ++i) {
			/** destroy this viewport **/
			remove_viewport(d, old_layout[i]);
			old_layout[i] = nullptr;
		}

		/* TODO: ensure floating client to be visible after output
		 * reconfiguration */

//		if(new_layout.size() > 0) {
//			// update position of floating managed clients to avoid offscreen
//			// floating window
//			for(auto x: net_client_list()) {
//				if(x->is(MANAGED_FLOATING)) {
//					auto r = x->position();
//					r.x = new_layout[0]->allocation().x
//							+ _theme->floating.margin.left;
//					r.y = new_layout[0]->allocation().y
//							+ _theme->floating.margin.top;
//					x->set_floating_wished_position(r);
//					x->reconfigure();
//				}
//			}
//		}

	}

	_root->broadcast_update_layout(time64_t::now());
	sync_tree_view();

}

void page_t::remove_viewport(workspace_p d, viewport_p v) {

	/* TODO: on output */

	/* remove fullscreened clients if needed */
//	for (auto &x : _fullscreen_client_to_viewport) {
//		if (x.second.viewport.lock() == v) {
//			unfullscreen(x.second.client.lock());
//			break;
//		}
//	}

	/* Transfer clients to a valid notebook */
	for (auto nbk : filter_class<notebook_t>(v->get_all_children())) {
		for (auto c : filter_class<xdg_surface_toplevel_view_t>(nbk->children())) {
			d->default_pop()->add_client(c, false);
		}
	}

}



//void page_t::create_managed_window(xcb_window_t w, xcb_atom_t type) {
//	auto mw = make_shared<xdg_surface_toplevel_t>(this, w, type);
//	_net_client_list.push_back(mw);
//	manage_client(mw, type);
//
////	if (mw->net_wm_strut() != nullptr
////			or mw->net_wm_strut_partial() != nullptr) {
////		update_workarea();
////	}
//}

void page_t::manage_client(xdg_surface_toplevel_view_p mw) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

//	if (mw->_pending.fullscreen) {
//		/**
//		 * Here client that default to fullscreen
//		 **/
//
//		fullscreen(mw);
//		update_desktop_visibility();
//		mw->show();
//		mw->activate();
//		set_focus(mw, XCB_CURRENT_TIME);
//
//	} else if (mw->_pending.transient_for == nullptr) {
//		/**
//		 * Here client that is NOTEBOOK
//		 **/
//
//		bind_window(mw, true);
//		mw->reconfigure();
//
//	} else {
//		/**
//		 * Here client that default to floating
//		 **/
//
//		mw->normalize();
//		mw->show();
//		mw->activate();
//		//set_focus(mw, XCB_CURRENT_TIME);
//		//if(mw->is(MANAGED_DOCK)) {
//		//	update_workarea();
//		//}
//
//		mw->reconfigure();
//		//_need_restack = true;
//	}

//	mw->show();

//	auto view = xdg_surface->create_view();
//	weston_view_set_position(view, 0, 0);
//	surface->timeline.force_refresh = 1;
//
//	wl_array array;
//	wl_array_init(&array);
//	wl_array_add(&array, sizeof(uint32_t)*2);
//	((uint32_t*)array.data)[0] = XDG_SURFACE_STATE_MAXIMIZED;
//	((uint32_t*)array.data)[1] = XDG_SURFACE_STATE_ACTIVATED;
//	xdg_surface_send_configure(resource, 800, 800, &array, 10);
//	wl_array_release(&array);
//
//	weston_view_geometry_dirty(view);

	/** case is notebook window **/
	//bind_window(mw, true);

	/** case is floating window **/
	insert_in_tree_using_transient_for(mw);

}

void page_t::create_unmanaged_window(xcb_window_t w, xcb_atom_t type) {
//	auto uw = make_shared<client_not_managed_t>(this, w, type);
//	_dpy->map(uw->orig());
//	uw->show();
//	safe_update_transient_for(uw);
}

shared_ptr<viewport_t> page_t::find_mouse_viewport(int x, int y) const {
	auto viewports = get_current_workspace()->get_viewports();
	for (auto v: viewports) {
		if (v->raw_area().is_inside(x, y))
			return v;
	}
	return shared_ptr<viewport_t>{};
}

/**
 * Read user time with proper fallback
 * @return: true if successfully find usertime, otherwise false.
 * @output time: if time is found time is set to the found value.
 * @input c: a window client handler.
 **/
//bool page_t::get_safe_net_wm_user_time(shared_ptr<xdg_surface_base_t> c, xcb_timestamp_t & time) {
//
//	/* TODO: Serial if apply */
//}
//
//shared_ptr<xdg_surface_toplevel_t> page_t::find_managed_window_with(xcb_window_t w) {
//	auto c = find_client_with(w);
//	if (c != nullptr) {
//		return dynamic_pointer_cast<xdg_surface_toplevel_t>(c);
//	} else {
//		return nullptr;
//	}
//}

/**
 * This will remove a client from tree and destroy related data. This
 * function do not send any X11 request.
 **/
//void page_t::cleanup_not_managed_client(shared_ptr<xdg_surface_popup_t> c) {
//	remove_client(c);
//}

//void page_t::safe_update_transient_for(shared_ptr<xdg_surface_base_t> c) {
//
//	/* TODO ? */
//
////	if (typeid(*c.get()) == typeid(xdg_surface_toplevel_t)) {
////		auto mw = dynamic_pointer_cast<xdg_surface_toplevel_t>(c);
////		if (mw->is(MANAGED_FLOATING)) {
////			detach(mw);
////			insert_in_tree_using_transient_for(mw);
////		} else if (mw->is(MANAGED_NOTEBOOK)) {
////			/* DO NOTHING */
////		} else if (mw->is(MANAGED_FULLSCREEN)) {
////			/* DO NOTHING */
////		} else if (mw->is(MANAGED_DOCK)) {
////			detach(mw);
////			insert_in_tree_using_transient_for(mw);
////		}
////	} else if (typeid(*c.get()) == typeid(xdg_surface_popup_t)) {
////		auto uw = dynamic_pointer_cast<xdg_surface_popup_t>(c);
////		detach(uw);
////		if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
////			_root->tooltips->push_back(uw);
////		} else if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
////			_root->notifications->push_back(uw);
////		} else if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_DOCK)) {
////			insert_in_tree_using_transient_for(uw);
////		} else if (uw->net_wm_state() != nullptr
////				and has_key<xcb_atom_t>(*(uw->net_wm_state()), A(_NET_WM_STATE_ABOVE))) {
////			_root->above->push_back(uw);
////		} else if (uw->net_wm_state() != nullptr
////				and has_key(*(uw->net_wm_state()), static_cast<xcb_atom_t>(A(_NET_WM_STATE_BELOW)))) {
////			_root->below->push_back(uw);
////		} else {
////			insert_in_tree_using_transient_for(uw);
////		}
////	}
//}

//void page_t::set_desktop_geometry(long width, long height) {
//	/* DO NOT APPLY ANYMORE */
//
//	/* define desktop geometry */
////	uint32_t desktop_geometry[2];
////	desktop_geometry[0] = width;
////	desktop_geometry[1] = height;
////	_dpy->change_property(_dpy->root(), _NET_DESKTOP_GEOMETRY,
////			CARDINAL, 32, desktop_geometry, 2);
//}
//
//shared_ptr<xdg_surface_toplevel_t> page_t::find_client_managed_with(xcb_window_t w) {
//	_net_client_list.remove_if([](weak_ptr<tree_t> const & w) { return w.expired(); });
//	for(auto & i: _net_client_list) {
//		if(i.lock()->has_window(w)) {
//			return i.lock();
//		}
//	}
//	return nullptr;
//}
//
//shared_ptr<xdg_surface_base_t> page_t::find_client_with(xcb_window_t w) {
//	for(auto & i: filter_class<xdg_surface_base_t>(_root->get_all_children())) {
//		if(i->has_window(w)) {
//			return i;
//		}
//	}
//	return nullptr;
//}
//
//shared_ptr<xdg_surface_base_t> page_t::find_client(xcb_window_t w) {
//	for(auto i: filter_class<xdg_surface_base_t>(_root->get_all_children())) {
//		if(i->orig() == w) {
//			return i;
//		}
//	}
//	return nullptr;
//}

void page_t::remove_client(xdg_surface_base_view_p c) {
	auto parent = c->parent()->shared_from_this();
	detach(c);
	for(auto i: c->children()) {
		auto c = dynamic_pointer_cast<xdg_surface_toplevel_view_t>(i);
		if(c != nullptr) {
			insert_in_tree_using_transient_for(c);
		}
	}
}

void replace(shared_ptr<page_component_t> const & src, shared_ptr<page_component_t> by) {
	throw exception_t{"Unexpectected use of page::replace function\n"};
}

inline void grab_key(xcb_connection_t * xcb, xcb_window_t w, key_desc_t & key, keymap_t * _keymap) {
//	int kc = 0;
//	if ((kc = _keymap->find_keysim(key.ks))) {
//		xcb_grab_key(xcb, true, w, key.mod, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
//		if(_keymap->numlock_mod_mask() != 0) {
//			xcb_grab_key(xcb, true, w, key.mod|_keymap->numlock_mod_mask(), kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
//		}
//	}

	/** TODO **/

}

/**
 * Update grab keys aware of current _keymap
 */
//void page_t::update_grabkey() {
//	/** TODO **/
//
//
////
////	assert(_keymap != nullptr);
////
////	/** ungrab all previews key **/
////	xcb_ungrab_key(_dpy->xcb(), XCB_GRAB_ANY, _dpy->root(), XCB_MOD_MASK_ANY);
////
////	int kc = 0;
////
////	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_1, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_2, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_3, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_4, _keymap);
////
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[0].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[1].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[2].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[3].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[4].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[5].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[6].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[7].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[8].key, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[9].key, _keymap);
////
////	grab_key(_dpy->xcb(), _dpy->root(), bind_page_quit, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_close, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_exposay_all, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_toggle_fullscreen, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_toggle_compositor, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_right_desktop, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_left_desktop, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_bind_window, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_fullscreen_window, _keymap);
////	grab_key(_dpy->xcb(), _dpy->root(), bind_float_window, _keymap);
////
////	/* Alt-Tab */
////	if ((kc = _keymap->find_keysim(XK_Tab))) {
////		xcb_grab_key(_dpy->xcb(), true, _dpy->root(), XCB_MOD_MASK_1, kc,
////				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
////		if (_keymap->numlock_mod_mask() != 0) {
////			xcb_grab_key(_dpy->xcb(), true, _dpy->root(),
////					XCB_MOD_MASK_1 | _keymap->numlock_mod_mask(), kc,
////					XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
////		}
////	}
//
//}
//
//void page_t::update_keymap() {
//	/* TODO */
//
////	if(_keymap != nullptr) {
////		delete _keymap;
////	}
////	_keymap = new keymap_t{_dpy->xcb()};
//}

///** debug function that try to print the state of page in stdout **/
//void page_t::print_state() const {
//	_root->print_tree(0);
//	cout << "_current_desktop = " << _root->_current_desktop << endl;
//
//	cout << "clients list:" << endl;
//	for(auto c: filter_class<xdg_surface_base_t>(_root->get_all_children())) {
//		cout << "client " << c->get_node_name() << " id = " << c->orig() << " ptr = " << c << " parent = " << c->parent() << endl;
//	}
//	cout << "end" << endl;;
//
//}
//
//void page_t::update_current_desktop() const {
//	/* OBSOLETE */
//}
//
//void page_t::switch_to_desktop(unsigned int desktop) {
//	assert(desktop < _root->_desktop_list.size());
//	if (desktop != _root->_current_desktop and desktop != ALL_DESKTOP) {
//		std::cout << "switch to desktop #" << desktop << std::endl;
//
//		if(desktop<_root->_current_desktop) {
//			if((_root->_current_desktop-desktop) <= (_root->_desktop_list.size()/2))
//				_root->_desktop_list[desktop]->start_switch(WORKSPACE_SWITCH_LEFT);
//			else
//				_root->_desktop_list[desktop]->start_switch(WORKSPACE_SWITCH_RIGHT);
//
//		} else {
//			if((desktop-_root->_current_desktop) <= (_root->_desktop_list.size()/2))
//				_root->_desktop_list[desktop]->start_switch(WORKSPACE_SWITCH_RIGHT);
//			else
//				_root->_desktop_list[desktop]->start_switch(WORKSPACE_SWITCH_LEFT);
//		}
//
//		auto stiky_list = get_sticky_client_managed(_root->_desktop_list[_root->_current_desktop]);
//
//		/* hide the current desktop */
//		_root->_desktop_list[_root->_current_desktop]->hide();
//		_root->_current_desktop = desktop;
//
//		/* put the new desktop ontop of others and show it */
//		_root->_desktop_stack->remove(_root->_desktop_list[_root->_current_desktop]);
//		_root->_desktop_stack->push_back(_root->_desktop_list[_root->_current_desktop]);
//		_root->_desktop_list[_root->_current_desktop]->show();
//
//		/** move sticky to current desktop **/
//		for(auto s : stiky_list) {
//			detach(s);
//			insert_in_tree_using_transient_for(s);
//		}
//		update_viewport_layout();
//		update_current_desktop();
//		update_desktop_visibility();
//	}
//}

//void page_t::update_fullscreen_clients_position() {
//	for(auto i: _fullscreen_client_to_viewport) {
//		i.second.client.lock()->set_notebook_wished_position(i.second.viewport.lock()->raw_area());
//	}
//}

//void page_t::update_desktop_visibility() {
//	/** hide only desktop that must be hidden first **/
//	for(unsigned k = 0; k < _root->_desktop_list.size(); ++k) {
//		if(k != _root->_current_desktop) {
//			_root->_desktop_list[k]->hide();
//		}
//	}
//
//	/** and show the desktop that have to be show **/
//	_root->_desktop_list[_root->_current_desktop]->show();
//
//	for(auto & i: _fullscreen_client_to_viewport) {
//		if(not i.second.workspace.lock()->is_visible()) {
//			if(not i.second.viewport.expired()) {
//				i.second.viewport.lock()->hide();
//			}
//		}
//	}
//}
//
//void page_t::_event_handler_bind(int type, callback_event_t f) {
//	_event_handlers[type] = f;
//}
//
//void page_t::_bind_all_default_event() {
//
//	/* TODO: event will be changed */
//
////	_event_handlers.clear();
////
////	_event_handler_bind(XCB_BUTTON_PRESS, &page_t::process_button_press_event);
////	_event_handler_bind(XCB_BUTTON_RELEASE, &page_t::process_button_release);
////	_event_handler_bind(XCB_MOTION_NOTIFY, &page_t::process_motion_notify);
////	_event_handler_bind(XCB_KEY_PRESS, &page_t::process_key_press_event);
////	_event_handler_bind(XCB_KEY_RELEASE, &page_t::process_key_release_event);
////	_event_handler_bind(XCB_CONFIGURE_NOTIFY, &page_t::process_configure_notify_event);
////	_event_handler_bind(XCB_CREATE_NOTIFY, &page_t::process_create_notify_event);
////	_event_handler_bind(XCB_DESTROY_NOTIFY, &page_t::process_destroy_notify_event);
////	_event_handler_bind(XCB_GRAVITY_NOTIFY, &page_t::process_gravity_notify_event);
////	_event_handler_bind(XCB_MAP_NOTIFY, &page_t::process_map_notify_event);
////	_event_handler_bind(XCB_REPARENT_NOTIFY, &page_t::process_reparent_notify_event);
////	_event_handler_bind(XCB_UNMAP_NOTIFY, &page_t::process_unmap_notify_event);
////	//_event_handler_bind(XCB_CIRCULATE_NOTIFY, &page_t::process_circulate_notify_event);
////	_event_handler_bind(XCB_CONFIGURE_REQUEST, &page_t::process_configure_request_event);
////	_event_handler_bind(XCB_MAP_REQUEST, &page_t::process_map_request_event);
////	_event_handler_bind(XCB_MAPPING_NOTIFY, &page_t::process_mapping_notify_event);
////	_event_handler_bind(XCB_SELECTION_CLEAR, &page_t::process_selection_clear_event);
////	_event_handler_bind(XCB_PROPERTY_NOTIFY, &page_t::process_property_notify_event);
////	_event_handler_bind(XCB_EXPOSE, &page_t::process_expose_event);
////	_event_handler_bind(XCB_FOCUS_IN, &page_t::process_focus_in_event);
////	_event_handler_bind(XCB_FOCUS_OUT, &page_t::process_focus_out_event);
////	_event_handler_bind(XCB_ENTER_NOTIFY, &page_t::process_enter_window_event);
////	_event_handler_bind(XCB_LEAVE_NOTIFY, &page_t::process_leave_window_event);
////
////
////	_event_handler_bind(0, &page_t::process_error);
////
////	_event_handler_bind(XCB_UNMAP_NOTIFY|0x80, &page_t::process_fake_unmap_notify_event);
////	_event_handler_bind(XCB_CLIENT_MESSAGE|0x80, &page_t::process_fake_client_message_event);
////
////	/** Extension **/
////	_event_handler_bind(_dpy->damage_event + XCB_DAMAGE_NOTIFY, &page_t::process_damage_notify_event);
////	_event_handler_bind(_dpy->randr_event + XCB_RANDR_NOTIFY, &page_t::process_randr_notify_event);
////	_event_handler_bind(_dpy->shape_event + XCB_SHAPE_NOTIFY, &page_t::process_shape_notify_event);
//
//}

//
//void page_t::process_mapping_notify_event(xcb_generic_event_t const * e) {
//	update_keymap();
//	update_grabkey();
//}
//
//void page_t::process_selection_clear_event(xcb_generic_event_t const * _e) {
//	/** OBSOLETE **/
//
////	auto e = reinterpret_cast<xcb_selection_clear_event_t const *>(_e);
////	if(e->selection == _dpy->wm_sn_atom)
////		_mainloop.stop();
////	if(e->selection == _dpy->cm_sn_atom)
////		stop_compositor();
//}
//
//void page_t::process_focus_in_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_focus_in_event_t const *>(_e);
//
//	/* TODO */
//
////	cout << focus_in_to_string(e) << endl;
////
////	/**
////	 * Since we can detect the client_id the focus rules is based on current
////	 * focussed client. i.e. as soon as the a client has the focus, he is
////	 * allowed to give the focus to any window he own.
////	 **/
////
////	if (e->event == _dpy->root() and e->detail == XCB_NOTIFY_DETAIL_NONE) {
////		_dpy->set_input_focus(identity_window, XCB_INPUT_FOCUS_NONE, XCB_CURRENT_TIME);
////		return;
////	}
////
////	shared_ptr<client_managed_t> focused;
////	if (get_current_workspace()->client_focus_history_front(focused)) {
////		// client are only allowed to focus their own windows
////		// NOTE: client_id() is based on Xorg client XID allocation and may be
////		//   invalid for other X11 server implementation.
////		if(client_id(focused->orig()) != client_id(e->event)) {
////			focused->focus(XCB_CURRENT_TIME);
////		}
////	} else {
////		/**
////		 * if no client should be focussed and the event does not belong
////		 * identity window, refocus identity window.
////		 **/
////		if(e->event != identity_window) {
////			_dpy->set_input_focus(identity_window, XCB_INPUT_FOCUS_NONE,
////					XCB_CURRENT_TIME);
////		}
////	}
////
////	{
////		auto c = find_client_managed_with(e->event);
////		if(c != nullptr) {
////			switch(e->detail) {
////			case XCB_NOTIFY_DETAIL_INFERIOR:
////			case XCB_NOTIFY_DETAIL_ANCESTOR:
////			case XCB_NOTIFY_DETAIL_VIRTUAL:
////			case XCB_NOTIFY_DETAIL_NONLINEAR:
////			case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
////				c->grab_button_focused_unsafe();
////				c->set_focus_state(true);
////			default:
////				break;
////			}
////		}
////	}
//
//}
//
//void page_t::process_focus_out_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_focus_in_event_t const *>(_e);
//
//	/* TODO */
//
////	cout << focus_in_to_string(e) << endl;
////
////	/**
////	 * if the root window loose the focus, give the focus back to the
////	 * proper window.
////	 **/
////	if(e->event == _dpy->root()) {
////		/* ignore all focus due to grabs */
////		if(e->mode != XCB_NOTIFY_MODE_NORMAL)
////			return;
////
////		/* ignore all focus event related to the pointer */
////		if(e->detail == XCB_NOTIFY_DETAIL_POINTER)
////			return;
////
////		shared_ptr<client_managed_t> focused;
////		if (get_current_workspace()->client_focus_history_front(focused)) {
////			focused->focus(XCB_CURRENT_TIME);
////		} else {
////			if (e->event == identity_window or e->event == _dpy->root()) {
////				_dpy->set_input_focus(identity_window, XCB_INPUT_FOCUS_NONE,
////						XCB_CURRENT_TIME);
////			}
////		}
////	} else {
////		auto c = find_client_managed_with(e->event);
////		if(c != nullptr) {
////			switch(e->detail) {
////			case XCB_NOTIFY_DETAIL_ANCESTOR:
////			case XCB_NOTIFY_DETAIL_NONLINEAR:
////			case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
////				c->grab_button_unfocused_unsafe();
////				c->set_focus_state(false);
////				break;
////			case XCB_NOTIFY_DETAIL_INFERIOR:
////				c->grab_button_focused_unsafe();
////				c->set_focus_state(true);
////				break;
////			default:
////				break;
////			}
////		}
////	}
//}
//
//void page_t::process_enter_window_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_enter_notify_event_t const *>(_e);
//	_root->broadcast_enter(e);
//
//	if(not configuration._mouse_focus)
//		return;
//
//	auto mw = find_managed_window_with(e->event);
//	if(mw != nullptr) {
//		set_focus(mw, e->time);
//	}
//}
//
//void page_t::process_leave_window_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_leave_notify_event_t const *>(_e);
//	_root->broadcast_leave(e);
//}
//
//void page_t::process_randr_notify_event(xcb_generic_event_t const * e) {
//	//auto ev = reinterpret_cast<xcb_randr_notify_event_t const *>(e);
//
//	//		char const * s_subtype = "Unknown";
//	//
//	//		switch(ev->subCode) {
//	//		case XCB_RANDR_NOTIFY_CRTC_CHANGE:
//	//			s_subtype = "RRNotify_CrtcChange";
//	//			break;
//	//		case XCB_RANDR_NOTIFY_OUTPUT_CHANGE:
//	//			s_subtype = "RRNotify_OutputChange";
//	//			break;
//	//		case XCB_RANDR_NOTIFY_OUTPUT_PROPERTY:
//	//			s_subtype = "RRNotify_OutputProperty";
//	//			break;
//	//		case XCB_RANDR_NOTIFY_PROVIDER_CHANGE:
//	//			s_subtype = "RRNotify_ProviderChange";
//	//			break;
//	//		case XCB_RANDR_NOTIFY_PROVIDER_PROPERTY:
//	//			s_subtype = "RRNotify_ProviderProperty";
//	//			break;
//	//		case XCB_RANDR_NOTIFY_RESOURCE_CHANGE:
//	//			s_subtype = "RRNotify_ResourceChange";
//	//			break;
//	//		default:
//	//			break;
//	//		}
//
////	if (ev->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
////		update_viewport_layout();
////		_dpy->update_layout();
////		_theme->update();
////	}
//
//	_need_restack = true;
//
//}
//
//void page_t::process_shape_notify_event(xcb_generic_event_t const * e) {
////	auto se = reinterpret_cast<xcb_shape_notify_event_t const *>(e);
////	if (se->shape_kind == XCB_SHAPE_SK_BOUNDING) {
////		xcb_window_t w = se->affected_window;
////		shared_ptr<xdg_surface_base_t> c = find_client(w);
////		if (c != nullptr) {
////			c->update_shape();
////		}
////
////		auto mw = dynamic_pointer_cast<xdg_surface_toplevel_t>(c);
////		if(mw != nullptr) {
////			mw->reconfigure();
////		}
////
////	}
//}
//
//void page_t::process_motion_notify(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_motion_notify_event_t const *>(_e);
//	if(_grab_handler != nullptr) {
//		_grab_handler->button_motion(e);
//		return;
//	} else {
//		_root->broadcast_button_motion(e);
//	}
//}
//
//void page_t::process_button_release(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_button_release_event_t const *>(_e);
//	if(_grab_handler != nullptr) {
//		_grab_handler->button_release(e);
//	} else {
//		_root->broadcast_button_release(e);
//	}
//}

//void page_t::start_compositor() {
//	/* OBSOLETE */
//}
//
//void page_t::stop_compositor() {
//	/* OBSOLETE */
//}

//void page_t::process_expose_event(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_expose_event_t const *>(_e);
//	_root->broadcast_expose(e);
//}
//
//void page_t::process_error(xcb_generic_event_t const * _e) {
//	auto e = reinterpret_cast<xcb_generic_error_t const *>(_e);
//	// TODO: _dpy->print_error(e);
//}


/* Inspired from openbox */
void page_t::run_cmd(std::string const & cmd_with_args)
{
	printf("executing %s\n", cmd_with_args.c_str());

    GError *e;
    gchar **argv = NULL;
    gchar *cmd;

    if (cmd_with_args == "null")
    	return;

    cmd = g_filename_from_utf8(cmd_with_args.c_str(), -1, NULL, NULL, NULL);
    if (!cmd) {
        printf("Failed to convert the path \"%s\" from utf8\n", cmd_with_args.c_str());
        return;
    }

    e = NULL;
    if (!g_shell_parse_argv(cmd, NULL, &argv, &e)) {
        printf("%s\n", e->message);
        g_error_free(e);
    } else {
        gchar *program = NULL;
        gboolean ok;

        e = NULL;
        ok = g_spawn_async(NULL, argv, NULL,
                           (GSpawnFlags)(G_SPAWN_SEARCH_PATH |
                           G_SPAWN_DO_NOT_REAP_CHILD),
                           NULL, NULL, NULL, &e);
        if (!ok) {
            printf("%s\n", e->message);
            g_error_free(e);
        }

        g_free(program);
        g_strfreev(argv);
    }

    g_free(cmd);

    return;
}

//vector<shared_ptr<xdg_surface_toplevel_t>> page_t::get_sticky_client_managed(shared_ptr<tree_t> base) {
//	vector<shared_ptr<xdg_surface_toplevel_t>> ret;
////	for(auto c: base->get_all_children()) {
////		auto mw = dynamic_pointer_cast<xdg_surface_toplevel_t>(c);
////		if(mw != nullptr) {
////			if(mw->is_stiky() and (not mw->is(MANAGED_NOTEBOOK)) and (not mw->is(MANAGED_FULLSCREEN)))
////				ret.push_back(mw);
////		}
////	}
//
//	return ret;
//}

shared_ptr<viewport_t> page_t::find_viewport_of(shared_ptr<tree_t> t) {
	while(t != nullptr) {
		auto ret = dynamic_pointer_cast<viewport_t>(t);
		if(ret != nullptr)
			return ret;
		t = t->parent()->shared_from_this();
	}

	return nullptr;
}

unsigned int page_t::find_current_desktop(shared_ptr<xdg_surface_base_t> c) {
//	if(c->net_wm_desktop() != nullptr)
//		return *(c->net_wm_desktop());
//	auto d = find_desktop_of(c);
//	if(d != nullptr)
//		return d->id();
	return _root->_current_desktop;
}
//
//void page_t::process_pending_events() {
//
//	/* TODO: wayland mainloop */
//
////	/* on connection error, terminate */
////	if(xcb_connection_has_error(_dpy->xcb()) != 0) {
////		_mainloop.stop();
////		return;
////	}
////
////	_dpy->fetch_pending_events();
//
//}

theme_t const * page_t::theme() const {
	return _theme;
}

//display_compositor_t * page_t::dpy() const {
//	return const_cast<page_t*>(this);
//}
//
//display_compositor_t * page_t::cmp() const {
//	return const_cast<page_t*>(this);
//}

void page_t::grab_start(weston_pointer * pointer, pointer_grab_handler_t * handler) {
	assert(_grab_handler == nullptr);
	_grab_handler = handler;
	pointer_start_grab(pointer, handler);
}

void page_t::grab_stop(weston_pointer * pointer) {
	pointer_end_grab(pointer);
	delete _grab_handler;
	_grab_handler = nullptr;
}

//void page_t::overlay_add(shared_ptr<tree_t> x) {
//	_root->_overlays->push_back(x);
//}

shared_ptr<workspace_t> const & page_t::get_current_workspace() const {
	return _root->_desktop_list[_root->_current_desktop];
}

shared_ptr<workspace_t> const & page_t::get_workspace(int id) const {
	return _root->_desktop_list[id];
}

int page_t::get_workspace_count() const {
	return _root->_desktop_list.size();
}

int page_t::create_workspace() {
	auto d = make_shared<workspace_t>(this, _root->_desktop_list.size());
	_root->_desktop_list.push_back(d);
	_root->_desktop_stack->push_front(d);
	d->hide();

//	update_viewport_layout();
//	update_current_desktop();
//	update_desktop_visibility();
	return d->id();
}
//
//int page_t::left_most_border() {
//	return _left_most_border;
//}
//int page_t::top_most_border() {
//	return _top_most_border;
//}
//
//keymap_t const * page_t::keymap() const {
//	return _keymap;
//}

list<xdg_surface_toplevel_view_w> page_t::global_client_focus_history() {
	return _global_focus_history;
}

bool page_t::global_focus_history_front(shared_ptr<xdg_surface_toplevel_view_t> & out) {
	if(not global_focus_history_is_empty()) {
		out = _global_focus_history.front().lock();
		return true;
	}
	return false;
}

void page_t::global_focus_history_remove(shared_ptr<xdg_surface_toplevel_view_t> in) {
	_global_focus_history.remove_if([in](weak_ptr<tree_t> const & w) { return w.expired() or w.lock() == in; });
}

void page_t::global_focus_history_move_front(shared_ptr<xdg_surface_toplevel_view_t> in) {
	move_front(_global_focus_history, in);
}

bool page_t::global_focus_history_is_empty() {
	_global_focus_history.remove_if([](weak_ptr<tree_t> const & w) { return w.expired(); });
	return _global_focus_history.empty();
}

//void page_t::start_alt_tab(xcb_timestamp_t time) {
//
//	/* TODO */
//
////	auto _x = net_client_list();
////	list<shared_ptr<client_managed_t>> managed_window{_x.begin(), _x.end()};
////
////	auto focus_history = global_client_focus_history();
////	/* reorder client to follow focused order */
////	for (auto i = focus_history.rbegin();
////			i != focus_history.rend();
////			++i) {
////		if (not i->expired()) {
////			auto x = std::find_if(managed_window.begin(), managed_window.end(), [i](shared_ptr<client_managed_t> const & x) -> bool { return x == i->lock(); });
////			if(x != managed_window.end()) {
////				managed_window.splice(managed_window.begin(), managed_window, x);
////			}
////		}
////	}
////
////	managed_window.remove_if([](shared_ptr<client_managed_t> const & c) { return c->skip_task_bar() or c->is(MANAGED_DOCK); });
////
////	if(managed_window.size() > 1) {
////		/* Grab keyboard */
////		auto ck = xcb_grab_keyboard(_dpy->xcb(),
////				false, _dpy->root(), time,
////				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
////
////		xcb_generic_error_t * err;
////		auto reply = xcb_grab_keyboard_reply(_dpy->xcb(), ck, &err);
////
////		if(err != nullptr) {
////			cout << "page_t::start_alt_tab error while trying to grab : " << xcb_event_get_error_label(err->error_code) << endl;
////			xcb_discard_reply(_dpy->xcb(), ck.sequence);
////		} else {
////			if(reply->status == XCB_GRAB_STATUS_SUCCESS) {
////				grab_start(new grab_alt_tab_t{this, managed_window, time});
////			} else {
////				cout << "page_t::start_alt_tab error while trying to grab" << endl;
////			}
////			free(reply);
////		}
////	}
//}
//
//void page_t::on_visibility_change_handler(xcb_window_t xid, bool visible) {
//	auto client = find_client_managed_with(xid);
//	if(client != nullptr) {
//		if(visible) {
//			//client->net_wm_state_remove(_NET_WM_STATE_HIDDEN);
//		} else {
//			//client->net_wm_state_add(_NET_WM_STATE_HIDDEN);
//		}
//	}
//}
//
//void page_t::on_block_mainloop_handler() {
//
//	/** OBSOLETE **/
//
////	while (_dpy->has_pending_events()) {
////		while (_dpy->has_pending_events()) {
////			process_event(_dpy->front_event());
////			_dpy->pop_event();
////		}
////
////		if (_need_restack) {
////			_need_restack = false;
////			update_windows_stack();
////			_need_update_client_list = true;
////		}
////
////		if(_need_update_client_list) {
////			_need_update_client_list = false;
////			update_client_list();
////			update_client_list_stacking();
////		}
////		xcb_flush(_dpy->xcb());
////	}
////
////	render();
//}

auto page_t::conf() const -> page_configuration_t const & {
	return configuration;
}
//
//auto page_t::create_view(xcb_window_t w) -> shared_ptr<client_view_t> {
//	// TODO: use weston view
//
//	//return _dpy->create_view(w);
//}
//
//void page_t::make_surface_stats(int & size, int & count) {
//	//_dpy->make_surface_stats(size, count);
//}
//
//auto page_t::net_client_list() -> list<client_managed_p> {
//	return lock(_net_client_list);
//}
//
//auto page_t::mainloop() -> mainloop_t * {
//	return &_mainloop;
//}


using backend_init_func =
		int (*)(struct weston_compositor *c,
	    struct weston_backend_config *config_base);

void page_t::load_x11_backend(weston_compositor* ec) {
	weston_x11_backend_config config = {{ 0, }};
	weston_x11_backend_output_config default_output = { 0, };

	default_output.height = 1600;
	default_output.width = 1600;
	default_output.name = strdup("Wayland output");
	default_output.scale = 1;
	default_output.transform = WL_OUTPUT_TRANSFORM_NORMAL;

	config.base.struct_size = sizeof(weston_x11_backend_config);
	config.base.struct_version = WESTON_X11_BACKEND_CONFIG_VERSION;

	config.fullscreen = 0;
	config.no_input = 0;
	config.num_outputs = 1;
	config.outputs = &default_output;
	config.use_pixman = 0;

	auto backend_init = reinterpret_cast<backend_init_func>(
			weston_load_module("x11-backend.so", "backend_init"));
	if (!backend_init)
		return;

	backend_init(ec, &config.base);

	free(default_output.name);

}

void page_t::connect_all() {

	destroy.notify = [](wl_listener *l, void *data) { weston_log("compositor::destroy\n"); };
    create_surface.notify = [](wl_listener *l, void *data) { weston_log("compositor::create_surface\n"); };
    activate.notify = [](wl_listener *l, void *data) { weston_log("compositor::activate\n"); };
    transform.notify = [](wl_listener *l, void *data) { weston_log("compositor::transform\n"); };
    kill.notify = [](wl_listener *l, void *data) { weston_log("compositor::kill\n"); };
    idle.notify = [](wl_listener *l, void *data) { weston_log("compositor::idle\n"); };
    wake.notify = [](wl_listener *l, void *data) { weston_log("compositor::wake\n"); };
    show_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::show_input_panel\n"); };
    hide_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::hide_input_panel\n"); };
    update_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::update_input_panel\n"); };
    seat_created.notify = [](wl_listener *l, void *data) { weston_log("compositor::seat_created\n"); };

    output_created.connect(&ec->output_created_signal, [this](weston_output * o) { this->on_output_created(o);});

    output_destroyed.notify = [](wl_listener *l, void *data) { weston_log("compositor::output_destroyed\n"); };
    output_moved.notify = [](wl_listener *l, void *data) { weston_log("compositor::output_moved\n"); };
    session.notify = [](wl_listener *l, void *data) { weston_log("compositor::session\n"); };


    wl_signal_add(&ec->destroy_signal, &destroy);
    wl_signal_add(&ec->create_surface_signal, &create_surface);
    wl_signal_add(&ec->activate_signal, &activate);
    wl_signal_add(&ec->transform_signal, &transform);
    wl_signal_add(&ec->kill_signal, &kill);
    wl_signal_add(&ec->idle_signal, &idle);
    wl_signal_add(&ec->wake_signal, &wake);
    wl_signal_add(&ec->show_input_panel_signal, &show_input_panel);
    wl_signal_add(&ec->hide_input_panel_signal, &hide_input_panel);
    wl_signal_add(&ec->update_input_panel_signal, &update_input_panel);
    wl_signal_add(&ec->seat_created_signal, &seat_created);

    wl_signal_add(&ec->output_destroyed_signal, &output_destroyed);
    wl_signal_add(&ec->output_moved_signal, &output_moved);
    wl_signal_add(&ec->session_signal, &session);
}

void page_t::on_output_created(weston_output * output) {
	weston_log("compositor::output_created\n");

//	auto stest = weston_surface_create(ec);
//	weston_surface_set_size(stest, output->width, output->height);
//    pixman_region32_fini(&stest->opaque);
//    pixman_region32_init_rect(&stest->opaque, 0, 0, output->width,
//    		output->height);
//    weston_surface_damage(stest);
//
//    auto surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, output->width,
//    		output->height);
//
//    auto cr = cairo_create(surf);
//    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
//    cairo_paint(cr);
//    cairo_destroy(cr);

//    auto sbuffer = weston_buffer_from_memory(output->width, output->height,
//    		cairo_image_surface_get_stride(surf), WL_SHM_FORMAT_ARGB8888, cairo_image_surface_get_data(surf));
//    weston_surface_attach(stest, sbuffer);
//    weston_surface_damage(stest);
//
//    auto sview = weston_view_create(stest);
//	weston_view_set_position(sview, output->x, output->y);
//	stest->timeline.force_refresh = 1;
//	weston_layer_entry_insert(&default_layer.view_list, &sview->layer_link);
//    weston_surface_commit(stest);


	/** create the solid color background **/
//	auto background = weston_surface_create(ec);
//	weston_surface_set_color(background, 0.5, 0.5, 0.5, 1.0);
//    weston_surface_set_size(background, output->width, output->height);
//    pixman_region32_fini(&background->opaque);
//    pixman_region32_init_rect(&background->opaque, output->x, output->y, output->width, output->width);
//    weston_surface_damage(background);
//    //pixman_region32_fini(&s->input);
//    //pixman_region32_init_rect(&s->input, 0, 0, w, h);
//
//    auto bview = weston_view_create(background);
//	weston_view_set_position(bview, 0, 0);
//	background->timeline.force_refresh = 1;
//	weston_layer_entry_insert(&default_layer.view_list, &bview->layer_link);

	repaint_functions[output] = output->repaint;
	output->repaint = &page_t::page_repaint;
	_outputs.push_back(output);
	update_viewport_layout();

}
//
//void page_t::xdg_shell_destroy(wl_client * client,
//		  wl_resource * resource) {
//	/* TODO */
//}
//
//void page_t::xdg_shell_use_unstable_version(wl_client * client, wl_resource * resource, int32_t version) {
//	/* TODO */
//}
//
//void page_t::xdg_shell_get_xdg_surface(wl_client * client,
//		    wl_resource * resource,
//		    uint32_t id,
//		    wl_resource * surface_resource) {
//
//	auto surface =
//		reinterpret_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));
//	auto c = client_shell_t::get(resource);
//
//	auto xdg_surface = make_shared<xdg_surface_toplevel_t>(dynamic_cast<page_context_t*>(this), client, surface, id);
//	c->xdg_shell_surfaces.push_back(xdg_surface);
//
//	wl_resource_set_implementation(xdg_surface->resource(),
//			&display_compositor_t::xdg_surface_implementation,
//			xdg_surface.get(), &xdg_surface_toplevel_t::xdg_surface_delete);
//
//	printf("create (%p)\n", xdg_surface.get());
//
//	/* tell weston how to use this data */
//	if (weston_surface_set_role(surface, "xdg_surface",
//				    resource, XDG_SHELL_ERROR_ROLE) < 0)
//		throw "TODO";
//
//	auto s = xdg_surface->on_configure.connect(this, &page_t::configure_surface);
//	_slots.push_back(s);
//
//	/* the first output */
////	weston_output* output = wl_container_of(ec->output_list.next,
////		    output, link);
////	surface->output = output;
//
//
//	weston_log("exit %s\n", __PRETTY_FUNCTION__);
//
//}
//
//void
//page_t::xdg_shell_get_xdg_popup(wl_client * client,
//		  wl_resource * resource,
//		  uint32_t id,
//		  wl_resource * surface_resource,
//		  wl_resource * parent_resource,
//		  wl_resource * seat_resource,
//		  uint32_t serial,
//		  int32_t x, int32_t y) {
//	weston_log("call %s\n", __PRETTY_FUNCTION__);
////	/* In our case nullptr */
////	auto surface =
////		reinterpret_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));
////	auto shell = xdg_shell_t::get(resource);
////
////	weston_log("p=%p, x=%d, y=%d\n", surface, x, y);
////
////	auto xdg_popup = new xdg_popup_t(client, id, surface, x, y);
//
//}
//
//void
//page_t::xdg_shell_pong(struct wl_client *client,
//	 struct wl_resource *resource, uint32_t serial)
//{
//	weston_log("call %s\n", __PRETTY_FUNCTION__);
//}


void page_t::configure_surface(xdg_surface_toplevel_view_p xdg_surface,
			int32_t sx, int32_t sy) {

	weston_log("ccc %p\n", xdg_surface.get());

	if(xdg_surface->is(MANAGED_UNCONFIGURED)) {
		manage_client(xdg_surface);
	} else {
		/* TODO: update the state if necessary */
	}

}

///**
// * the xdg-surface
// **/
//
//void page_t::xdg_surface_destroy(struct wl_client *client,
//		struct wl_resource *resource)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	wl_resource_destroy(resource);
//}
//
//void page_t::xdg_surface_set_parent(wl_client * client,
//		wl_resource * resource, wl_resource * parent_resource)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//
//	if(parent_resource) {
//		auto parent = reinterpret_cast<xdg_surface_toplevel_t*>(wl_resource_get_user_data(resource));
//		xdg_surface->set_transient_for(parent);
//	} else {
//		xdg_surface->set_transient_for(nullptr);
//	}
//
//}
//
//void
//page_t::xdg_surface_set_app_id(struct wl_client *client,
//		       struct wl_resource *resource,
//		       const char *app_id)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//
//}
//
//void
//page_t::xdg_surface_show_window_menu(wl_client *client,
//			     wl_resource *surface_resource,
//			     wl_resource *seat_resource,
//			     uint32_t serial,
//			     int32_t x,
//			     int32_t y)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(surface_resource);
//
//}
//
//void
//page_t::xdg_surface_set_title(wl_client *client,
//			wl_resource *resource, const char *title)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	xdg_surface->set_title(title);
//}
//
//void
//page_t::xdg_surface_move(struct wl_client *client, struct wl_resource *resource,
//		 struct wl_resource *seat_resource, uint32_t serial)
//{
//	weston_log("call %s\n", __PRETTY_FUNCTION__);
//
////	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
////
////	auto seat = reinterpret_cast<weston_seat*>(wl_resource_get_user_data(seat_resource));
////	auto xdg_surface = xdg_surface_t::get(resource);
////	auto pointer = weston_seat_get_pointer(seat);
////
////	weston_pointer_grab_move_t * grab_data =
////			reinterpret_cast<weston_pointer_grab_move_t *>(malloc(sizeof *grab_data));
////	/** TODO: memory error **/
////
////	grab_data->base.interface = &move_grab_interface;
////	grab_data->base.pointer = nullptr;
////
////	/* relative client position from the cursor */
////	grab_data->origin_x = xdg_surface->view->geometry.x - wl_fixed_to_double(pointer->grab_x);
////	grab_data->origin_y = xdg_surface->view->geometry.y - wl_fixed_to_double(pointer->grab_y);
////
////	wl_list_remove(&(xdg_surface->view->layer_link.link));
////	wl_list_insert(&(cmp->default_layer.view_list.link),
////			&(xdg_surface->view->layer_link.link));
////
////	weston_pointer_start_grab(seat->pointer_state, &grab_data->base);
//
//}
//
//void
//page_t::xdg_surface_resize(struct wl_client *client, struct wl_resource *resource,
//		   struct wl_resource *seat_resource, uint32_t serial,
//		   uint32_t edges)
//{
//	weston_log("call %s\n", __PRETTY_FUNCTION__);
//}
//
//void page_t::xdg_surface_ack_configure(wl_client *client,
//		wl_resource * resource,
//		uint32_t serial)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//
//	//weston_layer_entry_insert(&cmp->default_layer.view_list, &xdg_surface->view->layer_link);
//
//}
//
//void
//page_t::xdg_surface_set_window_geometry(struct wl_client *client,
//				struct wl_resource *resource,
//				int32_t x,
//				int32_t y,
//				int32_t width,
//				int32_t height)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	xdg_surface->set_window_geometry(x, y, width, height);
//}
//
//void
//page_t::xdg_surface_set_maximized(struct wl_client *client,
//			  struct wl_resource *resource)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	xdg_surface->set_maximized();
//}
//
//void
//page_t::xdg_surface_unset_maximized(struct wl_client *client,
//			    struct wl_resource *resource)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	xdg_surface->unset_maximized();
//}
//
//void
//page_t::xdg_surface_set_fullscreen(struct wl_client *client,
//			   struct wl_resource *resource,
//			   struct wl_resource *output_resource)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	xdg_surface->set_fullscreen();
//}
//
//void
//page_t::xdg_surface_unset_fullscreen(struct wl_client *client,
//			     struct wl_resource *resource)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	xdg_surface->unset_fullscreen();
//}
//
//void
//page_t::xdg_surface_set_minimized(struct wl_client *client,
//			    struct wl_resource *resource)
//{
//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//	xdg_surface->set_minimized();
//}

/**
 * This function synchronize the page tree with the weston scene graph. The side
 * effects are damage all outputs and schedule repaint for all outputs.
 **/
void page_t::sync_tree_view() {

	list<weston_view *> lv;
	auto children = _root->get_all_children();
	weston_log("found %lu children\n", children.size());
	for(auto x: children) {
		auto v = x->get_default_view();
		if(v)
			lv.push_back(v);
	}

	weston_log("found %lu views\n", lv.size());

	/* remove all existing views */
	weston_layer_entry * nxt;
	weston_layer_entry * cur;
	wl_list_for_each_safe(cur, nxt, &default_layer.view_list.link, link) {
		weston_view * v = wl_container_of(cur, v, layer_link);
		weston_layer_entry_remove(&v->layer_link);
	}

	for(auto v: lv) {
		weston_layer_entry_insert(&default_layer.view_list, &v->layer_link);
		weston_view_geometry_dirty(v);
		weston_view_update_transform(v);
	}

	wl_list_for_each_safe(cur, nxt, &default_layer.view_list.link, link) {
		weston_view * v = wl_container_of(cur, v, layer_link);
		weston_log("aaaa %p\n", v->output);
	}

	weston_compositor_damage_all(ec);

}

void page_t::xdg_popup_destroy(wl_client * client, wl_resource * resource) {

}

auto page_t::create_pixmap(uint32_t width, uint32_t height) -> pixmap_p {
	auto p = make_shared<pixmap_t>(this, PIXMAP_RGBA, width, height);
	pixmap_list.push_back(p);
	return p;
}

void page_t::process_focus(weston_pointer_grab * grab) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	weston_compositor_set_default_pointer_grab(ec, NULL);
	(*grab->pointer->default_grab.interface->focus)(grab);
	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);
}

void page_t::process_motion(weston_pointer_grab * grab, uint32_t time, weston_pointer_motion_event *event) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);

	_root->broadcast_motion(grab, time, event);

	weston_compositor_set_default_pointer_grab(ec, NULL);
	(*grab->pointer->default_grab.interface->motion)(grab, time, event);
	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);

}

void page_t::process_button(weston_pointer_grab * grab, uint32_t time,
		uint32_t button, uint32_t state) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);
//	weston_compositor_set_default_pointer_grab(ec, NULL);
//	(*grab->pointer->default_grab.interface->button)(grab, time, button, state);
//	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);

	struct weston_pointer *pointer = grab->pointer;
	struct weston_compositor *compositor = pointer->seat->compositor;
	struct weston_view *view;
	struct wl_resource *resource;
	uint32_t serial;
	struct wl_display *display = compositor->wl_display;
	wl_fixed_t sx, sy;
	struct wl_list *resource_list = NULL;

	if (pointer->focus_client)
		resource_list = &pointer->focus_client->pointer_resources;
	if (resource_list && !wl_list_empty(resource_list)) {
		resource_list = &pointer->focus_client->pointer_resources;
		serial = wl_display_next_serial(display);
		wl_resource_for_each(resource, resource_list)
			wl_pointer_send_button(resource,
					       serial,
					       time,
					       button,
					       state);
	}

	view = weston_compositor_pick_view(compositor,
					   pointer->x, pointer->y,
					   &sx, &sy);
	if(view) {
		if(strcmp("page_viewport", view->surface->role_name) == 0) {
			_root->broadcast_button(grab, time, button, state);
		}
		if (pointer->button_count == 0 &&
				 state == WL_POINTER_BUTTON_STATE_RELEASED) {

			weston_pointer_set_focus(pointer, view, sx, sy);

			if(strcmp("xdg_toplevel", view->surface->role_name) == 0) {
				auto xdg_window = reinterpret_cast<xdg_surface_toplevel_t*>(view->surface->configure_private);
				if(not xdg_window->master_view().expired()) {
					set_focus(pointer, xdg_window->master_view().lock());
				}
			}
		}
	}

}

void page_t::process_axis(weston_pointer_grab * grab, uint32_t time, weston_pointer_axis_event *event) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);

	weston_compositor_set_default_pointer_grab(ec, NULL);
	(*grab->pointer->default_grab.interface->axis)(grab, time, event);
	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);

}

void page_t::process_axis_source(weston_pointer_grab * grab, uint32_t source) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);

	weston_compositor_set_default_pointer_grab(ec, NULL);
	(*grab->pointer->default_grab.interface->axis_source)(grab, source);
	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);

}

void page_t::process_frame(weston_pointer_grab * grab) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);

	weston_compositor_set_default_pointer_grab(ec, NULL);
	(*grab->pointer->default_grab.interface->frame)(grab);
	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);

}

void page_t::process_cancel(weston_pointer_grab * grab) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);

	weston_compositor_set_default_pointer_grab(ec, NULL);
	(*grab->pointer->default_grab.interface->cancel)(grab);
	weston_compositor_set_default_pointer_grab(ec, &default_grab_pod.grab_interface);

}

void page_t::client_destroy(xdg_shell_client_t * c) {
	//_clients.remove_if([c] (xdg_shell_client_t const & x) -> bool { return x.client == c; });
}

void page_t::client_create_popup(xdg_shell_client_t * c, xdg_surface_popup_t * s) {
	/* TODO: find parent view */
}

void page_t::client_create_toplevel(xdg_shell_client_t * c, xdg_surface_toplevel_t * s) {
	/* TODO: manage */
}


}

