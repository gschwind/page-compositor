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
#include <compositor.h>
#include <compositor-x11.h>
#include <compositor-drm.h>
#include <windowed-output-api.h>
#include <wayland-client-protocol.h>
#include <xdg-shell-v5-shell.hxx>
#include <xdg-shell-v5-surface-base.hxx>
#include <xdg-shell-v5-surface-popup.hxx>
#include <xdg-shell-v5-surface-toplevel.hxx>

#include "xdg-shell-unstable-v5-server-protocol.h"
#include "xdg-shell-unstable-v6-server-protocol.h"

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
#include "view.hxx"

/* ICCCM definition */
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

namespace page {

static void _default_grab_focus(weston_pointer_grab * grab) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_focus(grab);
}

static void _default_grab_motion(weston_pointer_grab * grab, uint32_t time, weston_pointer_motion_event *event) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_motion(grab, time, event);
}

static void _default_grab_button(weston_pointer_grab * grab, uint32_t time, uint32_t button, uint32_t state) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_button(grab, time, button, state);
}

static void _default_grab_axis(weston_pointer_grab * grab, uint32_t time, weston_pointer_axis_event *event) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_axis(grab, time, event);
}

static void _default_grab_axis_source(weston_pointer_grab * grab, uint32_t source) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_axis_source(grab, source);
}

static void _default_grab_frame(weston_pointer_grab * grab) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_frame(grab);
}

static void _default_grab_cancel(weston_pointer_grab * grab) {
	page_t::_default_grab_interface_t * pod = wl_container_of(grab->interface, pod, grab_interface);
	pod->ths->process_cancel(grab);
}

void page_t::page_repaint_idle() {
	_root->broadcast_trigger_redraw();
	weston_compositor_schedule_repaint(ec);
	repaint_scheduled = false;
}

void page_t::schedule_repaint() {
	if(repaint_scheduled)
		return;
	repaint_scheduled = true;
	auto loop = wl_display_get_event_loop(ec->wl_display);
	wl_event_loop_add_idle(loop, [](void * data){
		reinterpret_cast<page_t*>(data)->page_repaint_idle();
	}, this);

}

void page_t::destroy_surface(surface_t * s) {
	if(s->_master_view.expired())
		return;
	detach(s->_master_view.lock());
	assert(s->_master_view.expired());
	sync_tree_view();
}

void page_t::start_move(surface_t * s, struct weston_seat * seat, uint32_t serial) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);
	if(s->_master_view.expired())
		return;

	auto pointer = weston_seat_get_pointer(seat);
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	auto master_view = s->_master_view.lock();
	if(master_view->is(MANAGED_NOTEBOOK)) {
		grab_start(pointer, new grab_bind_client_t{this, master_view,
			BTN_LEFT, rect(x, y, 1, 1)});
	} else if(master_view->is(MANAGED_FLOATING)) {
		grab_start(pointer, new grab_floating_move_t(this, master_view,
			BTN_LEFT, x, y));
	}
}

void page_t::start_resize(surface_t * s, struct weston_seat * seat, uint32_t serial, edge_e edges) {
	if(s->_master_view.expired())
		return;

	auto pointer = weston_seat_get_pointer(seat);
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	auto master_view = s->_master_view.lock();
	if(master_view->is(MANAGED_FLOATING)) {
		grab_start(pointer, new grab_floating_resize_t(this, master_view,
			BTN_LEFT, x, y, edges));
	}
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

static const struct zzz_buffer_manager_interface _zzz_buffer_manager_implementation = {
		ack_buffer
};

//time64_t const page_t::default_wait{1000000000L / 120L};

void page_t::bind_xdg_shell_v5(struct wl_client * client, void * data,
				      uint32_t version, uint32_t id) {
	page_t * ths = reinterpret_cast<page_t *>(data);

	auto c = new xdg_shell_client_t{ths, client, id};
	ths->connect(c->destroy, ths, &page_t::xdg_shell_v5_client_destroy);
	ths->_xdg_shell_v5_clients.push_back(c);

}

void page_t::bind_xdg_shell_v6(struct wl_client * client, void * data,
				      uint32_t version, uint32_t id) {
	page_t * ths = reinterpret_cast<page_t *>(data);

	auto c = new xdg_shell_v6_client_t{ths, client, id};
	ths->connect(c->destroy, ths, &page_t::xdg_shell_v6_client_destroy);
	ths->_xdg_shell_v6_clients.push_back(c);

}

void page_t::bind_wl_shell(struct wl_client * client, void * data,
				      uint32_t version, uint32_t id) {
	page_t * ths = reinterpret_cast<page_t *>(data);

	auto c = new wl_shell_client_t{ths, client, id};
	ths->connect(c->destroy, ths, &page_t::wl_shell_client_destroy);
	ths->_wl_shell_clients.push_back(c);

}

void page_t::bind_zzz_buffer_manager(struct wl_client * client, void * data,
	      uint32_t version, uint32_t id) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	page_t * ths = reinterpret_cast<page_t *>(data);

	if(ths->_buffer_manager_resource)
		throw exception_t{"only one buffer manager is allowed"};

	/* ONLY one those client */
	ths->_buffer_manager_resource = wl_resource_create(client,
			&::zzz_buffer_manager_interface, 1, id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	wl_resource_set_implementation(ths->_buffer_manager_resource,
			&_zzz_buffer_manager_implementation, ths, &xx_buffer_delete);


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

page_t::page_t(int argc, char ** argv) :
		repaint_scheduled{false}
{

	char const * conf_file_name = 0;

	use_x11_backend = false;
	use_pixman = false;
	_global_wl_shell = nullptr;
	_global_xdg_shell_v5 = nullptr;
	_global_xdg_shell_v6 = nullptr;
	_global_buffer_manager = nullptr;
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

		if(strcmp("--use-pixman", argv[k]) == 0) {
			use_pixman = true;
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

	bind_page_quit           = _conf.get_string("default", "bind_page_quit");
	bind_close               = _conf.get_string("default", "bind_close");
	bind_toggle_fullscreen   = _conf.get_string("default", "bind_toggle_fullscreen");
	bind_toggle_compositor   = _conf.get_string("default", "bind_toggle_compositor");
	bind_right_desktop       = _conf.get_string("default", "bind_right_desktop");
	bind_left_desktop        = _conf.get_string("default", "bind_left_desktop");

	bind_bind_window         = _conf.get_string("default", "bind_bind_window");
	bind_fullscreen_window   = _conf.get_string("default", "bind_fullscreen_window");
	bind_float_window        = _conf.get_string("default", "bind_float_window");

	bind_cmd[0].key = _conf.get_string("default", "bind_cmd_0");
	bind_cmd[1].key = _conf.get_string("default", "bind_cmd_1");
	bind_cmd[2].key = _conf.get_string("default", "bind_cmd_2");
	bind_cmd[3].key = _conf.get_string("default", "bind_cmd_3");
	bind_cmd[4].key = _conf.get_string("default", "bind_cmd_4");
	bind_cmd[5].key = _conf.get_string("default", "bind_cmd_5");
	bind_cmd[6].key = _conf.get_string("default", "bind_cmd_6");
	bind_cmd[7].key = _conf.get_string("default", "bind_cmd_7");
	bind_cmd[8].key = _conf.get_string("default", "bind_cmd_8");
	bind_cmd[9].key = _conf.get_string("default", "bind_cmd_9");

	bind_cmd[0].cmd = _conf.get_string("default", "exec_cmd_0");
	bind_cmd[1].cmd = _conf.get_string("default", "exec_cmd_1");
	bind_cmd[2].cmd = _conf.get_string("default", "exec_cmd_2");
	bind_cmd[3].cmd = _conf.get_string("default", "exec_cmd_3");
	bind_cmd[4].cmd = _conf.get_string("default", "exec_cmd_4");
	bind_cmd[5].cmd = _conf.get_string("default", "exec_cmd_5");
	bind_cmd[6].cmd = _conf.get_string("default", "exec_cmd_6");
	bind_cmd[7].cmd = _conf.get_string("default", "exec_cmd_7");
	bind_cmd[8].cmd = _conf.get_string("default", "exec_cmd_8");
	bind_cmd[9].cmd = _conf.get_string("default", "exec_cmd_9");

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


	default_grab_pod.grab_interface.focus = &_default_grab_focus;
	default_grab_pod.grab_interface.motion = &_default_grab_motion;
	default_grab_pod.grab_interface.button = &_default_grab_button;
	default_grab_pod.grab_interface.axis = &_default_grab_axis;
	default_grab_pod.grab_interface.axis_source = &_default_grab_axis_source;
	default_grab_pod.grab_interface.frame = &_default_grab_frame;
	default_grab_pod.grab_interface.cancel = &_default_grab_cancel;
	default_grab_pod.ths = this;

}

page_t::~page_t() {
	// cleanup cairo, for valgrind happiness.
	//cairo_debug_reset_static_data();
}

static int xxvprintf(const char * fmt, va_list args) {
	static FILE * log = 0;
	if(not log) {
		log = fopen("/tmp/page-compositor.log", "w");
	}

	int ret = vfprintf(log, fmt, args);
	fflush(log);
	return ret;

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


    weston_log_set_handler(&xxvprintf, &xxvprintf);

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
	ec->vt_switching = 1;

	weston_layer_init(&default_layer, &ec->cursor_layer.link);

	_global_wl_shell = wl_global_create(_dpy, &wl_shell_interface, 1, this,
			&page_t::bind_wl_shell);
	_global_xdg_shell_v5 = wl_global_create(_dpy, &xdg_shell_interface, 1, this,
			&page_t::bind_xdg_shell_v5);
	_global_xdg_shell_v6 = wl_global_create(_dpy, &zxdg_shell_v6_interface, 1, this,
			&page_t::bind_xdg_shell_v6);
	_global_buffer_manager = wl_global_create(_dpy,
			&zzz_buffer_manager_interface, 1, this,
			&page_t::bind_zzz_buffer_manager);


	connect_all();


	/* setup the keyboard layout (MANDATORY) */
	xkb_rule_names names = {
			/* weston steal those pointers ... */
			strdup(_conf.get_string("default", "xkb_rules").c_str()),					/*rules*/
			strdup(_conf.get_string("default", "xkb_model").c_str()),			/*model*/
			strdup(_conf.get_string("default", "xkb_layout").c_str()),				/*layout*/
			strdup(_conf.get_string("default", "xkb_variant").c_str()),					/*variant*/
			strdup(_conf.get_string("default", "xkb_options").c_str())					/*option*/
	};

	weston_compositor_set_xkb_rule_names(ec, &names);

	char * display = getenv("DISPLAY");
	if(display) {
		use_x11_backend = true;
		load_x11_backend(ec);
	} else {
		use_x11_backend = false;
		load_drm_backend(ec);
	}

	update_viewport_layout();

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

void page_t::handle_quit_page(weston_keyboard * wk, uint32_t time, uint32_t key) {
	wl_display_terminate(_dpy);
}

void page_t::handle_toggle_fullscreen(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	if(_current_focus.expired())
		return;
	auto v = _current_focus.lock();
	if(v->is(MANAGED_FULLSCREEN)) {
		unfullscreen(v);
	} else if (v->is(MANAGED_FLOATING) or v->is(MANAGED_NOTEBOOK)) {
		fullscreen(v);
	}
}

void page_t::handle_close_window(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	if(_current_focus.expired())
		return;
	auto v = _current_focus.lock();
	v->send_close();
}

void page_t::handle_goto_desktop_at_right(weston_keyboard * wk, uint32_t time, uint32_t key) {

}

void page_t::handle_goto_desktop_at_left(weston_keyboard * wk, uint32_t time, uint32_t key) {

}

void page_t::handle_bind_window(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	if(_current_focus.expired())
		return;
	auto v = _current_focus.lock();
	if(v->is(MANAGED_FULLSCREEN)) {
		unfullscreen(v);
		bind_window(v, true);
	} else if (v->is(MANAGED_FLOATING)) {
		bind_window(v, true);
	}
}

void page_t::handle_set_fullscreen_window(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	if(_current_focus.expired())
		return;
	auto v = _current_focus.lock();
	if(v->is(MANAGED_FLOATING) or v->is(MANAGED_NOTEBOOK)) {
		fullscreen(v);
	}
}

void page_t::handle_set_floating_window(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	if(_current_focus.expired())
		return;
	auto v = _current_focus.lock();
	if(v->is(MANAGED_FULLSCREEN)) {
		unfullscreen(v);
	}

	if(v->is(MANAGED_NOTEBOOK)) {
		unbind_window(v);
	}

}

void page_t::handle_bind_cmd_0(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[0].cmd);
}

void page_t::handle_bind_cmd_1(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[1].cmd);
}

void page_t::handle_bind_cmd_2(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[2].cmd);
}

void page_t::handle_bind_cmd_3(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[3].cmd);
}

void page_t::handle_bind_cmd_4(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[4].cmd);
}

void page_t::handle_bind_cmd_5(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[5].cmd);
}

void page_t::handle_bind_cmd_6(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[6].cmd);
}

void page_t::handle_bind_cmd_7(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[7].cmd);
}

void page_t::handle_bind_cmd_8(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[8].cmd);
}

void page_t::handle_bind_cmd_9(weston_keyboard * wk, uint32_t time, uint32_t key) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	run_cmd(bind_cmd[9].cmd);
}

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

void page_t::fullscreen(view_p mw) {

	if(mw->is(MANAGED_FULLSCREEN))
		return;

	shared_ptr<viewport_t> v;
	if(mw->is(MANAGED_NOTEBOOK)) {
		v = find_viewport_of(mw);
	} else if (mw->is(MANAGED_FLOATING)) {
		v = get_current_workspace()->get_any_viewport();
	} else {
		cout << "WARNING: a dock trying to become fullscreen" << endl;
		return;
	}

	fullscreen(mw, v);
}

void page_t::fullscreen(view_p mw, shared_ptr<viewport_t> v) {
	assert(v != nullptr);

	if(mw->is(MANAGED_FULLSCREEN))
		return;

	/* WARNING: Call order is important, change it with caution */

	fullscreen_data_t data;

	if(mw->is(MANAGED_NOTEBOOK)) {
		/**
		 * if the current window is managed in notebook:
		 *
		 * 1. search for the current notebook,
		 * 2. search the viewport for this notebook, and use it as default
		 *    fullscreen host or use the first available viewport.
		 **/
		data.revert_type = MANAGED_NOTEBOOK;
		data.revert_notebook = find_parent_notebook_for(mw);
	} else if (mw->is(MANAGED_FLOATING)) {
		data.revert_type = MANAGED_FLOATING;
		data.revert_notebook.reset();
	} else {
		cout << "WARNING: a dock trying to become fullscreen" << endl;
		return;
	}

	auto workspace = find_desktop_of(v);

	detach(mw);

	// unfullscreen client that already use this screen
	for (auto &x : _fullscreen_client_to_viewport) {
		if (x.second.viewport.lock() == v) {
			unfullscreen(x.second.client.lock());
			break;
		}
	}

	data.client = mw;
	data.workspace = workspace;
	data.viewport = v;

	_fullscreen_client_to_viewport[mw.get()] = data;

	mw->set_managed_type(MANAGED_FULLSCREEN);
	workspace->attach(mw);

	/* it's a trick */
	mw->set_notebook_wished_position(v->raw_area());
	mw->reconfigure();

	sync_tree_view();

}

void page_t::unfullscreen(view_p mw) {
	/* WARNING: Call order is important, change it with caution */

	/** just in case **/
	//mw->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);

	if(!has_key(_fullscreen_client_to_viewport, mw.get()))
		return;

	detach(mw);

	fullscreen_data_t data = _fullscreen_client_to_viewport[mw.get()];
	_fullscreen_client_to_viewport.erase(mw.get());

	shared_ptr<workspace_t> d;

	if(data.workspace.expired()) {
		d = get_current_workspace();
	} else {
		d = data.workspace.lock();
	}

	shared_ptr<viewport_t> v;

	if(data.viewport.expired()) {
		v = d->get_any_viewport();
	} else {
		v = data.viewport.lock();
	}

	if (data.revert_type == MANAGED_NOTEBOOK) {
		shared_ptr<notebook_t> n;
		if(data.revert_notebook.expired()) {
			n = d->default_pop();
		} else {
			n = data.revert_notebook.lock();
		}
		mw->set_managed_type(MANAGED_NOTEBOOK);
		n->add_client(mw, true);
		mw->reconfigure();
	} else {
		mw->set_managed_type(MANAGED_FLOATING);
		insert_in_tree_using_transient_for(mw);
		mw->reconfigure();
	}

	if(d->is_visible() and not v->is_visible()) {
		v->show();
	}

	sync_tree_view();

}

void page_t::toggle_fullscreen(view_p c) {
	if(c->is(MANAGED_FULLSCREEN))
		unfullscreen(c);
	else
		fullscreen(c);
}


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
		view_p x,
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

void page_t::set_keyboard_focus(weston_pointer * pointer,
		shared_ptr<view_t> new_focus) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	assert(new_focus != nullptr);
	assert(new_focus->get_default_view() != nullptr);

	if(!_current_focus.expired()) {
		if(_current_focus.lock() == new_focus)
			return;
		_current_focus.lock()->set_focus_state(false);
	}

	weston_seat_set_keyboard_focus(pointer->seat, new_focus->surface());

	_current_focus = new_focus;

	get_current_workspace()->client_focus_history_move_front(new_focus);
	global_focus_history_move_front(new_focus);
	new_focus->activate();
	new_focus->set_focus_state(true);

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

void page_t::split_left(notebook_p nbk, view_p c) {
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

void page_t::split_right(notebook_p nbk, view_p c) {
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

void page_t::split_top(notebook_p nbk, view_p c) {
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

void page_t::split_bottom(notebook_p nbk, view_p c) {
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
	auto clients = filter_class<view_t>(nbk->children());
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

void page_t::insert_in_tree_using_transient_for(view_p c) {

	c->set_managed_type(MANAGED_FLOATING);
	if(c->_page_surface->_transient_for
			and not c->_page_surface->_transient_for->_master_view.expired()) {
		auto parent = c->_page_surface->_transient_for->_master_view.lock();
		parent->add_transient_child(c);
	} else {
		get_current_workspace()->attach(c);
	}
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
		if(typeid(*t.get()) == typeid(view_t)) {
			auto x = dynamic_pointer_cast<view_t>(t);
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

void page_t::fullscreen_client_to_viewport(view_p c, viewport_p v) {
	detach(c);
	if (has_key(_fullscreen_client_to_viewport, c.get())) {
		fullscreen_data_t & data = _fullscreen_client_to_viewport[c.get()];
		if (v != data.viewport.lock()) {
			if(not data.viewport.expired()) {
				//data.viewport.lock()->show();
				//data.viewport.lock()->queue_redraw();
				//add_global_damage(data.viewport.lock()->raw_area());
			}
			//v->hide();
			//add_global_damage(v->raw_area());
			data.viewport = v;
			data.workspace = find_desktop_of(v);
			c->set_notebook_wished_position(v->raw_area());
			c->reconfigure();
			//update_desktop_visibility();
		}
	}
}

void page_t::bind_window(view_p mw, bool activate) {
	detach(mw);
	insert_window_in_notebook(mw, nullptr, activate);
}

void page_t::unbind_window(view_p mw) {
	detach(mw);
	mw->set_managed_type(MANAGED_FLOATING);
	insert_in_tree_using_transient_for(mw);
	mw->queue_redraw();
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

shared_ptr<notebook_t> page_t::find_parent_notebook_for(shared_ptr<view_t> mw) {
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


		if(new_layout.size() > 0) {
			// update position of floating managed clients to avoid offscreen
			// floating window
			for(auto x: filter_class<view_t>(_root->get_all_children())) {
				if(x->is(MANAGED_FLOATING)) {
					auto r = x->get_window_position();
					r.x = new_layout[0]->allocation().x;
					r.y = new_layout[0]->allocation().y;
					x->set_floating_wished_position(r);
					x->reconfigure();
				}
			}
		}
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
		for (auto c : filter_class<view_t>(nbk->children())) {
			d->default_pop()->add_client(c, false);
		}
	}

}

void page_t::manage_client(surface_t * s) {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	auto view = make_shared<view_t>(this, s);
	s->_master_view = view;


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

	if(s->_transient_for == nullptr) {
		bind_window(view, true);
	} else {
		insert_in_tree_using_transient_for(view);
	}

	/** case is floating window **/
	//insert_in_tree_using_transient_for(mw);

}

void page_t::manage_popup(surface_t * s) {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	assert(s->_parent != nullptr);

	auto parent_view = s->_parent->_master_view.lock();

	if(parent_view != nullptr) {
		auto view = make_shared<view_t>(this, s);
		s->_master_view = view;
		weston_log("%s x=%d, y=%d\n", __PRETTY_FUNCTION__, s->x_offset, s->y_offset);
		parent_view->add_popup_child(view, s->x_offset, s->y_offset);
		sync_tree_view();
	}
}

void page_t::configure_popup(surface_t * s) {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);
	s->send_configure_popup(s->x_offset, s->y_offset, s->width(), s->height());
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

void page_t::remove_client(view_p c) {
	auto parent = c->parent()->shared_from_this();
	detach(c);
	for(auto i: c->children()) {
		auto c = dynamic_pointer_cast<view_t>(i);
		if(c != nullptr) {
			insert_in_tree_using_transient_for(c);
		}
	}
}


static bool is_keycode_for_keysym(struct xkb_keymap *keymap,
		xkb_keycode_t keycode, xkb_keysym_t keysym) {
	xkb_layout_index_t num_layouts = xkb_keymap_num_layouts_for_key(keymap,
			keycode);
	for (xkb_layout_index_t i = 0; i < num_layouts; i++) {
		xkb_level_index_t num_levels = xkb_keymap_num_levels_for_key(keymap,
				keycode, i);
		for (xkb_level_index_t j = 0; j < num_levels; j++) {
			const xkb_keysym_t *syms;
			int num_syms = xkb_keymap_key_get_syms_by_level(keymap, keycode, i,
					j, &syms);
			for (int k = 0; k < num_syms; k++) {
				if (syms[k] == keysym)
					return true;
			}
		}
	}
	return false;
}

static void xkb_keymap_key_iter(struct xkb_keymap *keymap, xkb_keycode_t key, void *data) {
	auto input = reinterpret_cast<pair<xkb_keysym_t, xkb_keycode_t>*>(data);
	if(is_keycode_for_keysym(keymap, key, input->first)) {
		input->second = key;
	}
}


static xkb_keycode_t find_keycode_for_keysim(xkb_keymap * keymap, xkb_keysym_t ks) {
	xkb_keymap_key_iter_t iter;
	pair<xkb_keysym_t, xkb_keycode_t> data = {ks, XKB_KEYCODE_INVALID};
	xkb_keymap_key_for_each(keymap, &xkb_keymap_key_iter, &data);
	return data.second;
}

template<void (page_t::*func)(weston_keyboard *, uint32_t, uint32_t)>
void page_t::bind_key(xkb_keymap * keymap, key_desc_t & key) {
	xkb_keysym_t ks = xkb_keysym_from_name(key.keysym_name.c_str(), XKB_KEYSYM_NO_FLAGS);
	if(ks == XKB_KEY_NoSymbol)
		return;
	int kc = find_keycode_for_keysim(keymap, ks);
	weston_log("bind '%s' : %d, %d\n", key.keysym_name.c_str(), kc, ks);

	if (kc != XKB_KEYCODE_INVALID) {
		/* HOWTO lose time: there is a fixes offset (8) between keycode from xkbcommon and
		 * scancode from linux and obbiously weston use scancode... ref: xkbcommon.h */
		weston_compositor_add_key_binding(ec, kc - 8, (enum weston_keyboard_modifier)key.mod,
		[](weston_keyboard * keyboard, uint32_t time, uint32_t key, void *data) {
			auto ths = reinterpret_cast<page_t *>(data);
			(ths->*func)(keyboard, time, key);
		}, this);
	}
}

void page_t::on_seat_created(weston_seat * seat) {
	weston_log("call %s %p\n", __PRETTY_FUNCTION__, this);

	auto xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	auto keymap = xkb_keymap_new_from_names(xkb_ctx, &ec->xkb_names, XKB_KEYMAP_COMPILE_NO_FLAGS);

	bind_key<&page_t::handle_quit_page>(keymap, bind_page_quit);
	bind_key<&page_t::handle_toggle_fullscreen>(keymap, bind_toggle_fullscreen);
	bind_key<&page_t::handle_close_window>(keymap, bind_close);

	bind_key<&page_t::handle_goto_desktop_at_right>(keymap, bind_right_desktop);
	bind_key<&page_t::handle_goto_desktop_at_left>(keymap, bind_left_desktop);

	bind_key<&page_t::handle_bind_window>(keymap, bind_bind_window);
	bind_key<&page_t::handle_set_fullscreen_window>(keymap, bind_fullscreen_window);
	bind_key<&page_t::handle_set_floating_window>(keymap, bind_float_window);

	bind_key<&page_t::handle_bind_cmd_0>(keymap, bind_cmd[0].key);
	bind_key<&page_t::handle_bind_cmd_1>(keymap, bind_cmd[1].key);
	bind_key<&page_t::handle_bind_cmd_2>(keymap, bind_cmd[2].key);
	bind_key<&page_t::handle_bind_cmd_3>(keymap, bind_cmd[3].key);
	bind_key<&page_t::handle_bind_cmd_4>(keymap, bind_cmd[4].key);
	bind_key<&page_t::handle_bind_cmd_5>(keymap, bind_cmd[5].key);
	bind_key<&page_t::handle_bind_cmd_6>(keymap, bind_cmd[6].key);
	bind_key<&page_t::handle_bind_cmd_7>(keymap, bind_cmd[7].key);
	bind_key<&page_t::handle_bind_cmd_8>(keymap, bind_cmd[8].key);
	bind_key<&page_t::handle_bind_cmd_9>(keymap, bind_cmd[9].key);

	xkb_keymap_unref(keymap);
	xkb_context_unref(xkb_ctx);

}

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

list<view_w> page_t::global_client_focus_history() {
	return _global_focus_history;
}

bool page_t::global_focus_history_front(shared_ptr<view_t> & out) {
	if(not global_focus_history_is_empty()) {
		out = _global_focus_history.front().lock();
		return true;
	}
	return false;
}

void page_t::global_focus_history_remove(shared_ptr<view_t> in) {
	_global_focus_history.remove_if([in](weak_ptr<tree_t> const & w) { return w.expired() or w.lock() == in; });
}

void page_t::global_focus_history_move_front(shared_ptr<view_t> in) {
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
	struct weston_windowed_output_api const * api;
//	weston_x11_backend_output_config default_output = { 0, };
//
//	default_output.height = 1600;
//	default_output.width = 1600;
//	default_output.name = strdup("Wayland output");
//	default_output.scale = 1;
//	default_output.transform = WL_OUTPUT_TRANSFORM_NORMAL;

	config.base.struct_size = sizeof(weston_x11_backend_config);
	config.base.struct_version = WESTON_X11_BACKEND_CONFIG_VERSION;

	config.fullscreen = 0;
	config.no_input = 0;
//	config.num_outputs = 1;
//	config.outputs = &default_output;
	config.use_pixman = use_pixman?1:0;

	auto backend_init = reinterpret_cast<backend_init_func>(
			weston_load_module("x11-backend.so", "backend_init"));
	if (!backend_init)
		return;

	backend_init(ec, &config.base);

    output_created.connect(&ec->output_created_signal, this, &page_t::on_output_created);
    output_pending.connect(&ec->output_pending_signal, this, &page_t::on_output_pending);

	api = weston_windowed_output_get_api(ec);
	api->output_create(ec, "Wayland output");
//	free(default_output.name);

}

//static enum weston_drm_backend_output_mode
//drm_configure_output(struct weston_compositor *c,
//		     bool use_current_mode,
//		     const char *name,
//		     struct weston_drm_backend_output_config *config)
//{
//
//	config->base.scale = 1;
//	config->base.transform = WL_OUTPUT_TRANSFORM_NORMAL;
//
//	return WESTON_DRM_BACKEND_OUTPUT_CURRENT;
//}


void page_t::load_drm_backend(weston_compositor* ec) {
	weston_drm_backend_config config = {{ 0, }};

	config.base.struct_size = sizeof(weston_drm_backend_config);
	config.base.struct_version = WESTON_DRM_BACKEND_CONFIG_VERSION;

	config.connector = 0;
	config.tty = 0;
	config.use_pixman = use_pixman?1:0;
	config.seat_id = 0;
	config.gbm_format = 0;
//	config.configure_output = &drm_configure_output;
	config.configure_device = 0;
	config.use_current_mode = 1;

	auto backend_init = reinterpret_cast<backend_init_func>(
			weston_load_module("drm-backend.so", "backend_init"));
	if (!backend_init)
		return;

	backend_init(ec, &config.base);

    output_created.connect(&ec->output_created_signal, this, &page_t::on_output_created);
    output_pending.connect(&ec->output_pending_signal, this, &page_t::on_output_pending);

    weston_pending_output_coldplug(ec);

}

void page_t::connect_all() {

	wl_list_init(&destroy.link);

	wl_list_init(&create_surface.link);
	wl_list_init(&activate.link);
	wl_list_init(&transform.link);

	wl_list_init(&kill.link);
	wl_list_init(&idle.link);
	wl_list_init(&wake.link);

	wl_list_init(&show_input_panel.link);
	wl_list_init(&hide_input_panel.link);
	wl_list_init(&update_input_panel.link);

	seat_created.connect(&ec->seat_created_signal, this, &page_t::on_seat_created);

	wl_list_init(&output_destroyed.link);
	wl_list_init(&output_moved.link);

	wl_list_init(&session.link);

	destroy.notify = [](wl_listener *l, void *data) { weston_log("compositor::destroy\n"); };
    create_surface.notify = [](wl_listener *l, void *data) { weston_log("compositor::create_surface\n"); };
    activate.notify = [](wl_listener *l, void *data) { weston_log("compositor::activate\n"); };
    transform.notify = [](wl_listener *l, void *data) { /*weston_log("compositor::transform\n");*/ };
    kill.notify = [](wl_listener *l, void *data) { weston_log("compositor::kill\n"); };
    idle.notify = [](wl_listener *l, void *data) { weston_log("compositor::idle\n"); };
    wake.notify = [](wl_listener *l, void *data) { weston_log("compositor::wake\n"); };
    show_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::show_input_panel\n"); };
    hide_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::hide_input_panel\n"); };
    update_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::update_input_panel\n"); };

    output_destroyed.notify = [](wl_listener *l, void *data) { weston_log("compositor::output_destroyed\n"); };
    output_moved.notify = [](wl_listener *l, void *data) { weston_log("compositor::output_moved\n"); };
	output_resized.notify =  [](wl_listener *l, void *data) { weston_log("compositor::output_resized\n"); };

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

    wl_signal_add(&ec->output_destroyed_signal, &output_destroyed);
    wl_signal_add(&ec->output_moved_signal, &output_moved);
    wl_signal_add(&ec->output_resized_signal, &output_resized);

    wl_signal_add(&ec->session_signal, &session);
}

void page_t::on_output_created(weston_output * output) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	_outputs.push_back(output);
	update_viewport_layout();
}

void page_t::on_output_pending(weston_output * output) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	if (use_x11_backend) {
		weston_output_set_scale(output, 1);
		weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);

		const struct weston_windowed_output_api *api =
				weston_windowed_output_get_api(ec);
		api->output_set_size(output, 1600, 1600);

		weston_output_enable(output);
	} else {

		struct weston_config_section *section;
		const struct weston_drm_output_api *api = weston_drm_output_get_api(output->compositor);

		if (!api) {
			weston_log("Cannot use weston_drm_output_api.\n");
			return;
		}

		if (api->set_mode(output, WESTON_DRM_BACKEND_OUTPUT_CURRENT, NULL) < 0) {
			weston_log("Cannot configure an output using weston_drm_output_api.\n");
			return;
		}

		weston_output_set_scale(output, 1);
		weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);

		api->set_gbm_format(output, NULL);
		api->set_seat(output, NULL);

		weston_output_enable(output);
	}

}

/**
 * This function synchronize the page tree with the weston scene graph. The side
 * effects are damage all outputs and schedule repaint for all outputs.
 **/
void page_t::sync_tree_view() {

	/* create the list of weston views */
	list<weston_view *> views;
	auto children = _root->get_all_children();
	weston_log("found %lu children\n", children.size());
	for(auto x: children) {
		auto v = x->get_default_view();
		if(v)
			views.push_back(v);
	}

	//_root->print_tree(0);

	weston_log("found %lu views\n", views.size());

	/* remove all existing views */
	weston_layer_entry * nxt;
	weston_layer_entry * cur;
	wl_list_for_each_safe(cur, nxt, &default_layer.view_list.link, link) {
		weston_view * v = wl_container_of(cur, v, layer_link);
		weston_layer_entry_remove(&v->layer_link);
	}

	for(auto v: views) {
		weston_layer_entry_insert(&default_layer.view_list, &v->layer_link);
		weston_view_geometry_dirty(v);
		weston_view_update_transform(v);
	}

	wl_list_for_each_safe(cur, nxt, &default_layer.view_list.link, link) {
		weston_view * v = wl_container_of(cur, v, layer_link);
		weston_log("view=%p,output=%p,role='%s',surface=%p,x=%f,y=%f\n", v, v->output, weston_surface_get_role(v->surface), v->surface, v->geometry.x, v->geometry.y);
	}

	schedule_repaint();

}

auto page_t::create_pixmap(uint32_t width, uint32_t height) -> pixmap_p {
	auto p = make_shared<pixmap_t>(this, PIXMAP_RGBA, width, height);
	pixmap_list.push_back(p);
	return p;
}

/**
 * Called when the cursor enter in the output region or when a refocus maybe
 * needed.
 **/
void page_t::process_focus(weston_pointer_grab * grab) {
	struct weston_pointer *pointer = grab->pointer;
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

//static void
//weston_pointer_send_relative_motion(struct weston_pointer *pointer,
//				    uint32_t time,
//				    struct weston_pointer_motion_event *event)
//{
//	uint64_t time_usec;
//	double dx, dy, dx_unaccel, dy_unaccel;
//	wl_fixed_t dxf, dyf, dxf_unaccel, dyf_unaccel;
//	struct wl_list *resource_list;
//	struct wl_resource *resource;
//
//	if (!pointer->focus_client)
//		return;
//
//	if (!weston_pointer_motion_to_rel(pointer, event,
//					  &dx, &dy,
//					  &dx_unaccel, &dy_unaccel))
//		return;
//
//	resource_list = &pointer->focus_client->relative_pointer_resources;
//	time_usec = event->time_usec;
//	if (time_usec == 0)
//		time_usec = time * 1000ULL;
//
//	dxf = wl_fixed_from_double(dx);
//	dyf = wl_fixed_from_double(dy);
//	dxf_unaccel = wl_fixed_from_double(dx_unaccel);
//	dyf_unaccel = wl_fixed_from_double(dy_unaccel);
//
//	wl_resource_for_each(resource, resource_list) {
//		zwp_relative_pointer_v1_send_relative_motion(
//			resource,
//			(uint32_t) (time_usec >> 32),
//			(uint32_t) time_usec,
//			dxf, dyf,
//			dxf_unaccel, dyf_unaccel);
//	}
//}

static void
weston_pointer_send_motion(struct weston_pointer *pointer, uint32_t time,
			   wl_fixed_t sx, wl_fixed_t sy)
{
	struct wl_list *resource_list;
	struct wl_resource *resource;

	if (!pointer->focus_client)
		return;

	resource_list = &pointer->focus_client->pointer_resources;
	wl_resource_for_each(resource, resource_list)
		wl_pointer_send_motion(resource, time, sx, sy);
}

void page_t::process_motion(weston_pointer_grab * grab, uint32_t time, weston_pointer_motion_event *event) {
	//weston_log("call %s\n", __PRETTY_FUNCTION__);

	_root->broadcast_motion(grab, time, event);

	struct weston_pointer *pointer = grab->pointer;
	wl_fixed_t x, y;
	wl_fixed_t old_sx = pointer->sx;
	wl_fixed_t old_sy = pointer->sy;

	if (pointer->focus) {
		weston_pointer_motion_to_abs(pointer, event, &x, &y);
		weston_view_from_global_fixed(pointer->focus, x, y,
					      &pointer->sx, &pointer->sy);
	}

	weston_pointer_move(pointer, event);

	if (old_sx != pointer->sx || old_sy != pointer->sy) {
		weston_pointer_send_motion(pointer, time,
					   pointer->sx, pointer->sy);
	}

	//weston_pointer_send_relative_motion(pointer, time, event);

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

	_root->broadcast_button(grab, time, button, state);

}

void page_t::process_axis(weston_pointer_grab * grab, uint32_t time, weston_pointer_axis_event *event) {
	weston_pointer_send_axis(grab->pointer, time, event);
}

void page_t::process_axis_source(weston_pointer_grab * grab, uint32_t source) {
	weston_pointer_send_axis_source(grab->pointer, source);
}

void page_t::process_frame(weston_pointer_grab * grab) {
	weston_pointer_send_frame(grab->pointer);
}

void page_t::process_cancel(weston_pointer_grab * grab) {
	/* never cancel default grab ? */
}

void page_t::xdg_shell_v5_client_destroy(xdg_shell_client_t * c) {
	//_clients.remove_if([c] (xdg_shell_client_t const & x) -> bool { return x.client == c; });
}

void page_t::xdg_shell_v6_client_destroy(xdg_shell_v6_client_t * c) {
	//_clients.remove_if([c] (xdg_shell_client_t const & x) -> bool { return x.client == c; });
}

void page_t::wl_shell_client_destroy(wl_shell_client_t * c) {
	//_clients.remove_if([c] (xdg_shell_client_t const & x) -> bool { return x.client == c; });
}

void page_t::client_create_popup(xdg_shell_client_t * c, xdg_surface_popup_t * s) {
	/* TODO: find parent view */
}

void page_t::client_create_toplevel(xdg_shell_client_t * c, xdg_surface_toplevel_t * s) {
	/* TODO: manage */
}

void page_t::switch_focused_to_fullscreen() {
	if(_current_focus.expired())
		return;
	auto current = _current_focus.lock();
	toggle_fullscreen(current);

}

void page_t::switch_focused_to_floating() {
	if(_current_focus.expired())
		return;
	auto current = _current_focus.lock();
	unbind_window(current);
}

void page_t::switch_focused_to_notebook() {
	if(_current_focus.expired())
		return;
	auto current = _current_focus.lock();
	bind_window(current, true);
}


}

