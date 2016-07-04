/*
 * managed_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "leak_checker.hxx"

#include <typeinfo>

#include <cairo.h>
#include <linux/input.h>

#include "client_managed.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"
#include "xdg-shell-server-protocol.h"

namespace page {

using namespace std;

static void _xdg_surface_destroy(wl_client * client, wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_destroy(client, resource);
}

static void _xdg_surface_set_parent(wl_client * client, wl_resource * resource,
		wl_resource * parent_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_set_parent(client, resource, parent_resource);
}

static void _xdg_surface_set_app_id(wl_client * client, wl_resource * resource,
		const char* app_id)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_set_app_id(client, resource, app_id);
}

static void _xdg_surface_show_window_menu(wl_client * client,
		wl_resource * surface_resource, wl_resource * seat_resource,
		uint32_t serial, int32_t x, int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(surface_resource)->xdg_surface_show_window_menu(client, surface_resource, seat_resource,
			serial, x, y);
}

static void _xdg_surface_set_title(wl_client * client, wl_resource * resource,
		char const * title)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_set_title(client, resource, title);
}

static void _xdg_surface_move(wl_client * client, wl_resource * resource,
		wl_resource * seat_resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_move(client, resource, seat_resource, serial);
}

static void _xdg_surface_resize(wl_client * client, wl_resource * resource,
		wl_resource * seat_resource, uint32_t serial, uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_resize(client, resource, seat_resource, serial, edges);
}

static void _xdg_surface_ack_configure(wl_client * client,
		wl_resource * resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_ack_configure(client, resource, serial);
}

static void _xdg_surface_set_window_geometry(wl_client * client,
		wl_resource * resource, int32_t x, int32_t y, int32_t width,
		int32_t height)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_set_window_geometry(client, resource, x, y, width, height);
}

static void _xdg_surface_set_maximized(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_set_maximized(client, resource);
}

static void _xdg_surface_unset_maximized(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_unset_maximized(client, resource);
}

static void _xdg_surface_set_fullscreen(wl_client * client,
		wl_resource * resource, wl_resource * output_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_set_fullscreen(client, resource, output_resource);
}

static void _xdg_surface_unset_fullscreen(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_unset_fullscreen(client, resource);
}

static void _xdg_surface_set_minimized(wl_client * client,
		wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	xdg_surface_toplevel_t::get(resource)->xdg_surface_set_minimized(client, resource);
}

static struct xdg_surface_interface _xdg_surface_implementation = {
	page::_xdg_surface_destroy,
	page::_xdg_surface_set_parent,
	page::_xdg_surface_set_title,
	page::_xdg_surface_set_app_id,
	page::_xdg_surface_show_window_menu,
	page::_xdg_surface_move,
	page::_xdg_surface_resize,
	page::_xdg_surface_ack_configure,
	page::_xdg_surface_set_window_geometry,
	page::_xdg_surface_set_maximized,
	page::_xdg_surface_unset_maximized,
	page::_xdg_surface_set_fullscreen,
	page::_xdg_surface_unset_fullscreen,
	page::_xdg_surface_set_minimized
};



void xdg_surface_toplevel_t::xdg_surface_delete(struct wl_resource *resource) {
	auto ths = xdg_surface_toplevel_t::get(resource);

	weston_log("call %s\n", __PRETTY_FUNCTION__);
	ths->on_destroy.signal(ths);

}

void xdg_surface_toplevel_t::weston_surface_destroy() {
	on_destroy.signal(this);
}

xdg_surface_toplevel_t::xdg_surface_toplevel_t(
		page_context_t * ctx, wl_client * client,
		weston_surface * surface, uint32_t id) :
	xdg_surface_base_t{ctx, client, surface, id},
	_floating_wished_position{},
	_notebook_wished_position{},
	_wished_position{},
	_has_focus{false},
	_pending{},
	_ack_serial{0},
	_is_activated{false}
{

	rect pos{0,0,surface->width, surface->height};

	weston_log("window default position = %s\n", pos.to_string().c_str());

	_managed_type = MANAGED_UNCONFIGURED;

	_floating_wished_position = pos;
	_notebook_wished_position = pos;

	_transient_childdren = make_shared<tree_t>();
	push_back(_transient_childdren);

	_xdg_surface_resource = wl_resource_create(client, &xdg_surface_interface, 1, id);

	wl_resource_set_implementation(_xdg_surface_resource,
			&_xdg_surface_implementation,
			this, &xdg_surface_toplevel_t::xdg_surface_delete);

	surface->configure = &xdg_surface_base_t::_weston_configure;

	/* /!\ use xdg_surface_base, leaving the choie of the virutal
	 * weston_configure */
	surface->configure_private = dynamic_cast<xdg_surface_base_t*>(this);

	surface_destroy.notify = [] (wl_listener *l, void *data) {
		weston_surface * surface = reinterpret_cast<weston_surface*>(data);
		auto ths = dynamic_cast<xdg_surface_toplevel_t*>(reinterpret_cast<xdg_surface_base_t*>(surface->configure_private));
		ths->weston_surface_destroy();
	};

	wl_signal_add(&surface->destroy_signal, &surface_destroy);

	/* tell weston how to use this data */
	if (weston_surface_set_role(surface, "xdg_surface",
			_xdg_surface_resource, XDG_SHELL_ERROR_ROLE) < 0)
		throw "TODO";

//	/** if x == 0 then place window at center of the screen **/
//	if (_floating_wished_position.x == 0 and not is(MANAGED_DOCK)) {
//		_floating_wished_position.x =
//				(_client_proxy->geometry().width - _floating_wished_position.w) / 2;
//	}
//
//	if(_floating_wished_position.x - _ctx->theme()->floating.margin.left < 0) {
//		_floating_wished_position.x = _ctx->theme()->floating.margin.left;
//	}
//
//	/**
//	 * if y == 0 then place window at center of the screen
//	 **/
//	if (_floating_wished_position.y == 0 and not is(MANAGED_DOCK)) {
//		_floating_wished_position.y = (_client_proxy->geometry().height - _floating_wished_position.h) / 2;
//	}
//
//	if(_floating_wished_position.y - _ctx->theme()->floating.margin.top < 0) {
//		_floating_wished_position.y = _ctx->theme()->floating.margin.top;
//	}

	/**
	 * Create the base window, window that will content managed window
	 **/

	rect b = _floating_wished_position;

	//update_floating_areas();

	uint32_t cursor;

	//cursor = cnx()->xc_top_side;

	//select_inputs_unsafe();

	//update_icon();

}

xdg_surface_toplevel_t::~xdg_surface_toplevel_t() {

	//_ctx->add_global_damage(get_visible_region());

//	if(_default_view) {
//		weston_layer_entry_remove(&_default_view->layer_link);
//		weston_view_destroy(_default_view);
//		_default_view = nullptr;
//	}
//
//	on_destroy.signal(this);

//	unselect_inputs_unsafe();
//
//	if (_surf != nullptr) {
//		warn(cairo_surface_get_reference_count(_surf) == 1);
//		cairo_surface_destroy(_surf);
//		_surf = nullptr;
//	}
//
//	destroy_back_buffer();

}

auto xdg_surface_toplevel_t::shared_from_this() -> shared_ptr<xdg_surface_toplevel_t> {
	return dynamic_pointer_cast<xdg_surface_toplevel_t>(tree_t::shared_from_this());
}

void xdg_surface_toplevel_t::reconfigure() {
	if(not _default_view)
		return;

	if (is(MANAGED_FLOATING)) {
		_wished_position = _floating_wished_position;
	} else {
		_wished_position = _notebook_wished_position;
	}

	if(_default_view) {
		weston_view_set_position(_default_view, _wished_position.x,
				_wished_position.y);
		weston_view_geometry_dirty(_default_view);
	}

	_ack_serial = weston_compositor_get_time();

	wl_array array;
	wl_array_init(&array);
	wl_array_add(&array, sizeof(uint32_t));
	((uint32_t*)array.data)[0] = XDG_SURFACE_STATE_MAXIMIZED;
	if(_has_focus) {
		wl_array_add(&array, sizeof(uint32_t));
		((uint32_t*)array.data)[1] = XDG_SURFACE_STATE_ACTIVATED;
	}
	xdg_surface_send_configure(_xdg_surface_resource, _wished_position.w,
			_wished_position.h, &array, _ack_serial);
	wl_array_release(&array);
	wl_client_flush(_client);


}


void xdg_surface_toplevel_t::set_managed_type(managed_window_type_e type)
{
	_managed_type = type;
	reconfigure();
}

rect xdg_surface_toplevel_t::get_base_position() const {
	return _wished_position;
}

managed_window_type_e xdg_surface_toplevel_t::get_type() {
	return _managed_type;
}

bool xdg_surface_toplevel_t::is(managed_window_type_e type) {
	return _managed_type == type;
}


/**
 * set usual passive button grab for a focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
//void xdg_surface_toplevel_t::grab_button_focused_unsafe() {
//
//
//}

/**
 * set usual passive button grab for a not focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
//void xdg_surface_toplevel_t::grab_button_unfocused_unsafe() {
//
//}

bool xdg_surface_toplevel_t::is_fullscreen() {
	return _managed_type == MANAGED_FULLSCREEN;
}

//bool xdg_surface_toplevel_t::skip_task_bar() {
//	return false;
//}
//
//void xdg_surface_toplevel_t::net_wm_state_delete() {
//
//}

//void xdg_surface_toplevel_t::normalize() {
//	if(not _is_iconic)
//		return;
//	_is_iconic = false;
//}
//
//void xdg_surface_toplevel_t::iconify() {
//	if(_is_iconic)
//		return;
//	_is_iconic = true;
//	_is_activated = false;
//}

//void xdg_surface_toplevel_t::wm_state_delete() {
//	/**
//	 * This one is for removing the window manager tag, thus only check if the window
//	 * still exist. (don't need lock);
//	 **/
//
//	//_client_proxy->delete_wm_state();
//}

void xdg_surface_toplevel_t::set_floating_wished_position(rect const & pos) {
	_floating_wished_position = pos;
}

void xdg_surface_toplevel_t::set_notebook_wished_position(rect const & pos) {
	_notebook_wished_position = pos;
}

rect const & xdg_surface_toplevel_t::get_wished_position() {
	return _wished_position;
}

rect const & xdg_surface_toplevel_t::get_floating_wished_position() {
	return _floating_wished_position;
}
//
//bool xdg_surface_toplevel_t::has_window(xcb_window_t w) const {
//	return false;
//}

string xdg_surface_toplevel_t::get_node_name() const {
	string s = _get_node_name<'M'>();
	ostringstream oss;
	oss << s << " " << (void*)nullptr << " " << title();
	return oss.str();
}

//rect const & xdg_surface_toplevel_t::base_position() const {
//	return _wished_position;
//}
//
//rect const & xdg_surface_toplevel_t::orig_position() const {
//	return _wished_position;
//}

void xdg_surface_toplevel_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

}

void xdg_surface_toplevel_t::render_finished() {

}

void xdg_surface_toplevel_t::set_focus_state(bool is_focused) {
	_has_change = true;
	_has_focus = is_focused;
	on_focus_change.signal(shared_from_this(), is_focused);
	queue_redraw();
}

void xdg_surface_toplevel_t::hide() {

	for(auto x: _children) {
		x->hide();
	}

	if(_default_view) {
		weston_view_unmap(_default_view);
		weston_view_destroy(_default_view);
		_default_view = nullptr;
		_ctx->sync_tree_view();
	}

	_is_visible = false;
}

void xdg_surface_toplevel_t::show() {
	_is_visible = true;

	if(not _default_view) {
		_default_view = weston_view_create(_surface);
		reconfigure();
		weston_log("bbXX %p\n", _surface->compositor);
		_ctx->sync_tree_view();
	}

	for(auto x: _children) {
		x->show();
	}

}

//bool xdg_surface_toplevel_t::is_iconic() {
//	return _is_iconic;
//}

//bool xdg_surface_toplevel_t::is_stiky() {
//	return false;
//}
//
//bool xdg_surface_toplevel_t::is_modal() {
//	return false;
//}

void xdg_surface_toplevel_t::activate() {

	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}

	_is_activated = true;
	reconfigure();

	queue_redraw();

}

void xdg_surface_toplevel_t::queue_redraw() {

	if(_default_view) {
		weston_view_schedule_repaint(_default_view);
	}

	if(is(MANAGED_FLOATING)) {
		//_is_resized = true;
	} else {
		tree_t::queue_redraw();
	}
}

void xdg_surface_toplevel_t::trigger_redraw() {

}

void xdg_surface_toplevel_t::send_close() {
	xdg_surface_send_close(_xdg_surface_resource);
}

void xdg_surface_toplevel_t::set_title(char const * title) {
	_has_change = true;
	weston_log("%p title: %s\n", this, title);
	if(title) {
		_pending.title = title;
	} else {
		_pending.title = "";
	}
}

//shared_ptr<icon16> xdg_surface_toplevel_t::icon() const {
//	return _icon;
//}
//
//bool xdg_surface_toplevel_t::has_focus() const {
//	return _has_focus;
//}

//void xdg_surface_toplevel_t::set_current_desktop(unsigned int n) {
//
//}
//
//void xdg_surface_toplevel_t::set_demands_attention(bool x) {
//	_demands_attention = x;
//}
//
//bool xdg_surface_toplevel_t::demands_attention() {
//	return _demands_attention;
//}

string const & xdg_surface_toplevel_t::title() const {
	return _current.title;
}



void xdg_surface_toplevel_t::weston_configure(struct weston_surface * es,
		int32_t sx, int32_t sy)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	/* configuration is invalid */
	if(_ack_serial != 0)
		return;

	auto ptr = shared_from_this();

	if(_pending.maximized != _current.maximized) {
		if(_pending.maximized) {
			/* on maximize */
		} else {
			/* on unmaximize */
		}
	}

	if(_pending.minimized != _current.minimized) {
		if(_pending.minimized) {
			/* on minimize */
		} else {
			/* on unminimize */
		}
	}

	_current = _pending;

	if(is(MANAGED_UNCONFIGURED)) {
		_ctx->manage_client(ptr);
	} else {
		/* TODO: update the state if necessary */
		if(_default_view)
			weston_view_schedule_repaint(_default_view);
	}

}

void xdg_surface_toplevel_t::set_transient_for(xdg_surface_toplevel_t * s) {
	_pending.transient_for = s;
}

auto xdg_surface_toplevel_t::transient_for() const -> xdg_surface_toplevel_t * {
	return _current.transient_for;
}

auto xdg_surface_toplevel_t::get(wl_resource * r) -> xdg_surface_toplevel_t * {
	return reinterpret_cast<xdg_surface_toplevel_t*>(wl_resource_get_user_data(r));
}

auto xdg_surface_toplevel_t::resource() const -> wl_resource * {
	return _xdg_surface_resource;
}

void xdg_surface_toplevel_t::xdg_surface_destroy(struct wl_client *client,
		struct wl_resource *resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	on_destroy.signal(this);
	_ctx->sync_tree_view();
	wl_resource_destroy(resource);

}

void xdg_surface_toplevel_t::xdg_surface_set_parent(wl_client * client,
		wl_resource * resource, wl_resource * parent_resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);

	if(parent_resource) {
		auto parent = reinterpret_cast<xdg_surface_toplevel_t*>(wl_resource_get_user_data(resource));
		xdg_surface->set_transient_for(parent);
	} else {
		xdg_surface->set_transient_for(nullptr);
	}

}

void xdg_surface_toplevel_t::xdg_surface_set_app_id(struct wl_client *client,
		       struct wl_resource *resource,
		       const char *app_id)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);

}

void xdg_surface_toplevel_t::xdg_surface_show_window_menu(wl_client *client,
			     wl_resource *surface_resource,
			     wl_resource *seat_resource,
			     uint32_t serial,
			     int32_t x,
			     int32_t y)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(surface_resource);

}

void xdg_surface_toplevel_t::xdg_surface_set_title(wl_client *client,
			wl_resource *resource, const char *title)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->set_title(title);
}

void xdg_surface_toplevel_t::xdg_surface_move(struct wl_client *client, struct wl_resource *resource,
		 struct wl_resource *seat_resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto seat = reinterpret_cast<weston_seat*>(
			wl_resource_get_user_data(seat_resource));

	auto pointer = weston_seat_get_pointer(seat);
	double x = wl_fixed_to_double(pointer->x);
	double y = wl_fixed_to_double(pointer->y);

	_ctx->grab_start(pointer, new grab_bind_client_t{_ctx, shared_from_this(), BTN_LEFT, rect(x, y, 1, 1)});


//	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
//
//	auto seat = reinterpret_cast<weston_seat*>(wl_resource_get_user_data(seat_resource));
//	auto xdg_surface = xdg_surface_t::get(resource);
//	auto pointer = weston_seat_get_pointer(seat);
//
//	weston_pointer_grab_move_t * grab_data =
//			reinterpret_cast<weston_pointer_grab_move_t *>(malloc(sizeof *grab_data));
//	/** TODO: memory error **/
//
//	grab_data->base.interface = &move_grab_interface;
//	grab_data->base.pointer = nullptr;
//
//	/* relative client position from the cursor */
//	grab_data->origin_x = xdg_surface->view->geometry.x - wl_fixed_to_double(pointer->grab_x);
//	grab_data->origin_y = xdg_surface->view->geometry.y - wl_fixed_to_double(pointer->grab_y);
//
//	wl_list_remove(&(xdg_surface->view->layer_link.link));
//	wl_list_insert(&(cmp->default_layer.view_list.link),
//			&(xdg_surface->view->layer_link.link));
//
//	weston_pointer_start_grab(seat->pointer_state, &grab_data->base);

}

void xdg_surface_toplevel_t::xdg_surface_resize(struct wl_client *client, struct wl_resource *resource,
		   struct wl_resource *seat_resource, uint32_t serial,
		   uint32_t edges)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

void xdg_surface_toplevel_t::xdg_surface_ack_configure(wl_client *client,
		wl_resource * resource,
		uint32_t serial)
{

	if(serial == _ack_serial)
		_ack_serial = 0;

}

void xdg_surface_toplevel_t::xdg_surface_set_window_geometry(struct wl_client *client,
				struct wl_resource *resource,
				int32_t x,
				int32_t y,
				int32_t width,
				int32_t height)
{
	_pending.geometry = rect(x, y, width, height);
}

void xdg_surface_toplevel_t::xdg_surface_set_maximized(struct wl_client *client,
			  struct wl_resource *resource)
{
	_pending.maximized = true;
}

void xdg_surface_toplevel_t::xdg_surface_unset_maximized(struct wl_client *client,
			    struct wl_resource *resource)
{
	_pending.maximized = false;
}

void xdg_surface_toplevel_t::xdg_surface_set_fullscreen(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *output_resource)
{
	_pending.fullscreen = true;
}

void xdg_surface_toplevel_t::xdg_surface_unset_fullscreen(struct wl_client *client,
			     struct wl_resource *resource)
{
	_pending.fullscreen = false;
}

void xdg_surface_toplevel_t::xdg_surface_set_minimized(struct wl_client *client,
			    struct wl_resource *resource)
{
	_pending.minimized = true;
}

}

