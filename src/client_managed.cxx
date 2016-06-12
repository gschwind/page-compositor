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

static struct ::xdg_surface_interface _xdg_surface_implementation = {
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
	/* TODO */
}

static void _weston_configure(struct weston_surface * es,
		int32_t sx, int32_t sy)
{
	auto ths = reinterpret_cast<xdg_surface_toplevel_t *>(es->configure_private);
	ths->weston_configure(es, sx, sy);
}

xdg_surface_toplevel_t::xdg_surface_toplevel_t(
		page_context_t * ctx, wl_client * client,
		weston_surface * surface, uint32_t id) :
	xdg_surface_base_t{ctx, client, surface, id},
	_floating_wished_position{},
	_notebook_wished_position{},
	_wished_position{},
	_orig_position{},
	_base_position{},
	_surf{nullptr},
	_icon(nullptr),
	_has_focus{false},
	_is_iconic{true},
	_demands_attention{false},
	_default_view{nullptr},
	_pending{},
	_ack_serial{0}
{

	rect pos{0,0,surface->width, surface->height};

	printf("window default position = %s\n", pos.to_string().c_str());

	_managed_type = MANAGED_UNDEFINED;

	_floating_wished_position = pos;
	_notebook_wished_position = pos;
	_base_position = pos;
	_orig_position = pos;

	_xdg_surface_resource = wl_resource_create(client, &::xdg_surface_interface, 1, id);

	wl_resource_set_implementation(_xdg_surface_resource,
			&_xdg_surface_implementation,
			this, &xdg_surface_delete);

	surface->configure = &_weston_configure;
	surface->configure_private = this;

	weston_log("bbbb %p\n", surface->configure_private);
	weston_log("bbbX %p\n", _ctx);

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

	if(_default_view) {
		weston_layer_entry_remove(&_default_view->layer_link);
		weston_view_destroy(_default_view);
		_default_view = nullptr;
	}

	on_destroy.signal(this);

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

		/* floating window without borders */
		_base_position = _wished_position;

		_orig_position.x = 0;
		_orig_position.y = 0;
		_orig_position.w = _wished_position.w;
		_orig_position.h = _wished_position.h;

		/* avoid to hide title bar of floating windows */
		if(_base_position.y < 0) {
			_base_position.y = 0;
		}

		//cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);

	} else {
		_wished_position = _notebook_wished_position;
		_base_position = _notebook_wished_position;
		_orig_position = rect(0, 0, _base_position.w, _base_position.h);
	}

	_is_resized = true;

	if(_default_view) {
		weston_view_set_position(_default_view, _base_position.x, _base_position.y);
		weston_view_geometry_dirty(_default_view);
	}

	_ack_serial = weston_compositor_get_time();

	wl_array array;
	wl_array_init(&array);
	wl_array_add(&array, sizeof(uint32_t)*2);
	((uint32_t*)array.data)[0] = XDG_SURFACE_STATE_MAXIMIZED;
	((uint32_t*)array.data)[1] = XDG_SURFACE_STATE_ACTIVATED;
	xdg_surface_send_configure(_xdg_surface_resource, _base_position.w,
			_base_position.h, &array, _ack_serial);
	wl_array_release(&array);


}


void xdg_surface_toplevel_t::set_managed_type(managed_window_type_e type)
{
	_managed_type = type;
	reconfigure();
}

rect xdg_surface_toplevel_t::get_base_position() const {
	return _base_position;
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
void xdg_surface_toplevel_t::grab_button_focused_unsafe() {


}

/**
 * set usual passive button grab for a not focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void xdg_surface_toplevel_t::grab_button_unfocused_unsafe() {

}

bool xdg_surface_toplevel_t::is_fullscreen() {
	return _managed_type == MANAGED_FULLSCREEN;
}

bool xdg_surface_toplevel_t::skip_task_bar() {
	return false;
}

void xdg_surface_toplevel_t::net_wm_state_delete() {

}

void xdg_surface_toplevel_t::normalize() {
	if(not _is_iconic)
		return;
	_is_iconic = false;
}

void xdg_surface_toplevel_t::iconify() {
	if(_is_iconic)
		return;
	_is_iconic = true;
}

void xdg_surface_toplevel_t::wm_state_delete() {
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/

	//_client_proxy->delete_wm_state();
}

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

rect const & xdg_surface_toplevel_t::base_position() const {
	return _base_position;
}

rect const & xdg_surface_toplevel_t::orig_position() const {
	return _orig_position;
}

void xdg_surface_toplevel_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

}

void xdg_surface_toplevel_t::render_finished() {

}

void xdg_surface_toplevel_t::set_focus_state(bool is_focused) {
	_has_change = true;
	_has_focus = is_focused;
	on_focus_change.signal(shared_from_this());
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

bool xdg_surface_toplevel_t::is_iconic() {
	return _is_iconic;
}

bool xdg_surface_toplevel_t::is_stiky() {
	return false;
}

bool xdg_surface_toplevel_t::is_modal() {
	return false;
}

void xdg_surface_toplevel_t::activate() {

	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}

	if(is_iconic()) {
		normalize();
		queue_redraw();
	}

}

void xdg_surface_toplevel_t::queue_redraw() {

	if(_default_view) {
		weston_view_schedule_repaint(_default_view);
	}

	if(is(MANAGED_FLOATING)) {
		_is_resized = true;
	} else {
		tree_t::queue_redraw();
	}
}

void xdg_surface_toplevel_t::trigger_redraw() {

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

shared_ptr<icon16> xdg_surface_toplevel_t::icon() const {
	return _icon;
}

bool xdg_surface_toplevel_t::has_focus() const {
	return _has_focus;
}

void xdg_surface_toplevel_t::set_current_desktop(unsigned int n) {

}

void xdg_surface_toplevel_t::set_demands_attention(bool x) {
	_demands_attention = x;
}

bool xdg_surface_toplevel_t::demands_attention() {
	return _demands_attention;
}

string const & xdg_surface_toplevel_t::title() const {
	return _title;
}



void xdg_surface_toplevel_t::weston_configure(struct weston_surface * es,
		int32_t sx, int32_t sy)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	weston_log("typeInfo %p\n", es->configure_private);
	weston_log("typeInfo %s\n", typeid(es->configure_private).name());

	/* configuration is invalid */
	if(_ack_serial != 0)
		return;

	auto ptr = shared_from_this();

	if(is(MANAGED_UNDEFINED)) {
		_ctx->manage_client(ptr);
	} else {
		/* TODO: update the state if necessary */
		weston_view_schedule_repaint(_default_view);
	}

	/* once configure is finished apply pending states */
	_title = _pending.title;
	_transient_for = _pending.transient_for;

}

void xdg_surface_toplevel_t::set_transient_for(xdg_surface_toplevel_t * s) {
	_pending.transient_for = s;
}

auto xdg_surface_toplevel_t::transient_for() const -> xdg_surface_toplevel_t * {
	return _transient_for;
}

auto xdg_surface_toplevel_t::get(wl_resource * r) -> xdg_surface_toplevel_t * {
	return reinterpret_cast<xdg_surface_toplevel_t*>(wl_resource_get_user_data(r));
}

void xdg_surface_toplevel_t::set_maximized() {
	_pending.maximize = true;
}

void xdg_surface_toplevel_t::unset_maximized() {
	_pending.maximize = false;
}

void xdg_surface_toplevel_t::set_fullscreen() {
	_pending.fullscreen = true;
}

void xdg_surface_toplevel_t::unset_fullscreen() {
	_pending.fullscreen = false;
}

void xdg_surface_toplevel_t::set_minimized() {
	_pending.minimize = true;
}

void xdg_surface_toplevel_t::set_window_geometry(int32_t x, int32_t y, int32_t w, int32_t h) {
	_pending.geometry = rect(x, y, w, h);
}

auto xdg_surface_toplevel_t::resource() const -> wl_resource * {
	return _xdg_surface_resource;
}

auto xdg_surface_toplevel_t::get_default_view() const -> weston_view * {
	return _default_view;
}

void xdg_surface_toplevel_t::xdg_surface_destroy(struct wl_client *client,
		struct wl_resource *resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
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
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->set_window_geometry(x, y, width, height);
}

void xdg_surface_toplevel_t::xdg_surface_set_maximized(struct wl_client *client,
			  struct wl_resource *resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->set_maximized();
}

void xdg_surface_toplevel_t::xdg_surface_unset_maximized(struct wl_client *client,
			    struct wl_resource *resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->unset_maximized();
}

void xdg_surface_toplevel_t::xdg_surface_set_fullscreen(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *output_resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->set_fullscreen();
}

void xdg_surface_toplevel_t::xdg_surface_unset_fullscreen(struct wl_client *client,
			     struct wl_resource *resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->unset_fullscreen();
}

void xdg_surface_toplevel_t::xdg_surface_set_minimized(struct wl_client *client,
			    struct wl_resource *resource)
{
	auto xdg_surface = xdg_surface_toplevel_t::get(resource);
	xdg_surface->set_minimized();
}

}

