/*
 * unmanaged_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "client_not_managed.hxx"

namespace page {

using namespace std;

void xdg_surface_popup_t::weston_configure(weston_surface * es, int32_t sx,
		int32_t sy)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static void xdg_popup_destroy(wl_client * client, wl_resource * resource) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

struct xdg_popup_interface xx_xdg_popup_interface = {
		xdg_popup_destroy
};

xdg_surface_popup_t * xdg_surface_popup_t::get(wl_resource *resource) {
	return reinterpret_cast<xdg_surface_popup_t*>(
			wl_resource_get_user_data(resource));
}

void xdg_surface_popup_t::xdg_popup_delete(struct wl_resource *resource) {
	auto ths = xdg_surface_popup_t::get(resource);

	weston_log("call %s\n", __PRETTY_FUNCTION__);

}

xdg_surface_popup_t::xdg_surface_popup_t(page_context_t * ctx, wl_client * client,
		  wl_resource * resource,
		  uint32_t id,
		  weston_surface * surface,
		  weston_surface * parent,
		  weston_seat * seat,
		  uint32_t serial,
		  int32_t x, int32_t y) :
		xdg_surface_base_t{ctx, client, surface, id}
{
	auto _xparent = reinterpret_cast<xdg_surface_base_t*>(parent->configure_private);

	_xdg_surface_resource = wl_resource_create(client, &xdg_popup_interface, 1, id);

	wl_resource_set_implementation(_xdg_surface_resource,
			&xx_xdg_popup_interface,
			this, &xdg_surface_popup_t::xdg_popup_delete);

	surface->configure = &xdg_surface_base_t::_weston_configure;
	surface->configure_private = dynamic_cast<xdg_surface_base_t*>(this);

	_default_view = create_view();
	weston_view_set_transform_parent(_default_view, _xparent->get_default_view());
	weston_view_set_position(_default_view, x, y);
	weston_view_geometry_dirty(_default_view);
	_ctx->sync_tree_view();
}

xdg_surface_popup_t::~xdg_surface_popup_t() {

}

//bool xdg_surface_popup_t::has_window(xcb_window_t w) const {
//	//return w == _client_proxy->id();
//}
//
//string xdg_surface_popup_t::get_node_name() const {
////	std::string s = _get_node_name<'U'>();
////	std::ostringstream oss;
////	oss << s << " " << orig();
////	oss << " " << cnx()->get_atom_name(_client_proxy->wm_type()) << " ";
////
////	if(_client_proxy->net_wm_name() != nullptr) {
////		oss << " " << *_client_proxy->net_wm_name();
////	}
////
////	oss << " " << _client_proxy->geometry().width << "x" << _client_proxy->geometry().height << "+" << _client_proxy->geometry().x << "+" << _client_proxy->geometry().y;
////
////	return oss.str();
//}
//
//void xdg_surface_popup_t::update_layout(time64_t const time) {
////	if(not _is_visible)
////		return;
////
////	_base_position = _client_proxy->position();
////
////	_update_visible_region();
////	_update_opaque_region();
////
////	region dmg { _client_view->get_damaged() };
////	dmg.translate(_base_position.x, _base_position.y);
////	_damage_cache += dmg;
////	_client_view->clear_damaged();
////
////	rect pos(_client_proxy->geometry().x, _client_proxy->geometry().y,
////			_client_proxy->geometry().width, _client_proxy->geometry().height);
//
//}
//
//region xdg_surface_popup_t::get_visible_region() {
//	return _visible_region_cache;
//}
//
//void xdg_surface_popup_t::_update_visible_region() {
//	/** update visible cache **/
//	_visible_region_cache = region{_base_position};
//}
//
//void xdg_surface_popup_t::_update_opaque_region() {
//
//}
//
//region xdg_surface_popup_t::get_opaque_region() {
//	return _opaque_region_cache;
//}
//
//region xdg_surface_popup_t::get_damaged() {
//	return _damage_cache;
//}
//
//xcb_window_t xdg_surface_popup_t::base() const {
//	//return _client_proxy->id();
//}
//
//xcb_window_t xdg_surface_popup_t::orig() const {
//	//return _client_proxy->id();
//}
//
//rect const & xdg_surface_popup_t::base_position() const {
//	//_base_position = _client_proxy->position();
//	return _base_position;
//}
//
//rect const & xdg_surface_popup_t::orig_position() const {
//	//_base_position = _client_proxy->position();
//	return _base_position;
//}
//
//void xdg_surface_popup_t::render(cairo_t * cr, region const & area) {
////	auto pix = _client_view->get_pixmap();
////
////	if (pix != nullptr) {
////		cairo_save(cr);
////		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
////		cairo_set_source_surface(cr, pix->get_cairo_surface(),
////				_base_position.x, _base_position.y);
////		region r = region{_base_position} & area;
////		for (auto &i : r.rects()) {
////			cairo_clip(cr, i);
////			cairo_mask_surface(cr, pix->get_cairo_surface(),
////					_base_position.x, _base_position.y);
////		}
////		cairo_restore(cr);
////	}
//
//}
//
//void xdg_surface_popup_t::hide() {
//	for(auto x: _children) {
//		x->hide();
//	}
//
//	_ctx->add_global_damage(get_visible_region());
//	_is_visible = false;
//	//unmap();
//}
//
//void xdg_surface_popup_t::show() {
//	_is_visible = true;
//	//map();
//	for(auto x: _children) {
//		x->show();
//	}
//
//}
//
//void xdg_surface_popup_t::render_finished() {
//	//_damage_cache.clear();
//}
//
//void xdg_surface_popup_t::on_property_notify(xcb_property_notify_event_t const * e) {
////	if (e->atom == A(_NET_WM_OPAQUE_REGION)) {
////		_update_opaque_region();
////	}
//}

}
