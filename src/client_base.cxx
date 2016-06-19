/*
 * client_base.cxx
 *
 *  Created on: 5 août 2015
 *      Author: gschwind
 */

#include "client_base.hxx"

namespace page {

using namespace std;


xdg_surface_base_t::xdg_surface_base_t(page_context_t * ctx, wl_client * client,
		weston_surface * surface, uint32_t id) :
	tree_t{},
	_ctx{ctx},
	_xdg_surface_resource{nullptr},
	_client{client},
	_surface{surface},
	_default_view{nullptr}
{

}

xdg_surface_base_t::~xdg_surface_base_t() {
	if(_xdg_surface_resource)
		wl_resource_destroy(_xdg_surface_resource);
}

bool xdg_surface_base_t::has_motif_border() {
	return false;
}

void xdg_surface_base_t::add_subclient(shared_ptr<xdg_surface_base_t> s) {
	assert(s != nullptr);
	_children.push_back(s);
	s->set_parent(this);
	if(_is_visible) {
		s->show();
	} else {
		s->hide();
	}
}

bool xdg_surface_base_t::is_window(wl_client * client, uint32_t w) {
	return _client == client and wl_resource_get_id(_xdg_surface_resource) == w;
}

string xdg_surface_base_t::get_node_name() const {
	return _get_node_name<'c'>();
}

uint32_t xdg_surface_base_t::wm_type() {
	return 0;
}

void xdg_surface_base_t::print_window_attributes() {
	/* TODO */
}

void xdg_surface_base_t::print_properties() {
	/* TODO */
}

void xdg_surface_base_t::remove(shared_ptr<tree_t> t) {
	if(t == nullptr)
		return;
	auto c = dynamic_pointer_cast<xdg_surface_base_t>(t);
	if(c == nullptr)
		return;
	_children.remove(c);
	c->clear_parent();
}

void xdg_surface_base_t::append_children(vector<shared_ptr<tree_t>> & out) const {
	out.insert(out.end(), _children.begin(), _children.end());
}

void xdg_surface_base_t::activate(shared_ptr<tree_t> t) {
	assert(t != nullptr);
	assert(has_key(_children, t));
	tree_t::activate();
	move_back(_children, t);
}

/* find the bigger window that is smaller than w and h */
dimention_t<unsigned> xdg_surface_base_t::compute_size_with_constrain(unsigned w, unsigned h) {

	/* currently not available */

	return dimention_t<unsigned>{w, h};

//	/* has no constrain */
//	if (wm_normal_hints() == nullptr)
//		return dimention_t<unsigned> { w, h };
//
//	XSizeHints const * sh = wm_normal_hints();
//
//	if (sh->flags & PMaxSize) {
//		if ((int) w > sh->max_width)
//			w = sh->max_width;
//		if ((int) h > sh->max_height)
//			h = sh->max_height;
//	}
//
//	if (sh->flags & PBaseSize) {
//		if ((int) w < sh->base_width)
//			w = sh->base_width;
//		if ((int) h < sh->base_height)
//			h = sh->base_height;
//	} else if (sh->flags & PMinSize) {
//		if ((int) w < sh->min_width)
//			w = sh->min_width;
//		if ((int) h < sh->min_height)
//			h = sh->min_height;
//	}
//
//	if (sh->flags & PAspect) {
//		if (sh->flags & PBaseSize) {
//			/**
//			 * ICCCM say if base is set subtract base before aspect checking
//			 * reference: ICCCM
//			 **/
//			if ((w - sh->base_width) * sh->min_aspect.y
//					< (h - sh->base_height) * sh->min_aspect.x) {
//				/* reduce h */
//				h = sh->base_height
//						+ ((w - sh->base_width) * sh->min_aspect.y)
//								/ sh->min_aspect.x;
//
//			} else if ((w - sh->base_width) * sh->max_aspect.y
//					> (h - sh->base_height) * sh->max_aspect.x) {
//				/* reduce w */
//				w = sh->base_width
//						+ ((h - sh->base_height) * sh->max_aspect.x)
//								/ sh->max_aspect.y;
//			}
//		} else {
//			if (w * sh->min_aspect.y < h * sh->min_aspect.x) {
//				/* reduce h */
//				h = (w * sh->min_aspect.y) / sh->min_aspect.x;
//
//			} else if (w * sh->max_aspect.y > h * sh->max_aspect.x) {
//				/* reduce w */
//				w = (h * sh->max_aspect.x) / sh->max_aspect.y;
//			}
//		}
//
//	}
//
//	if (sh->flags & PResizeInc) {
//		w -= ((w - sh->base_width) % sh->width_inc);
//		h -= ((h - sh->base_height) % sh->height_inc);
//	}
//
//	return dimention_t<unsigned> { w, h };

}

//auto xdg_surface_base_t::get_xid() const -> xcb_window_t {
//	return base();
//}
//
//auto xdg_surface_base_t::get_parent_xid() const -> xcb_window_t {
//	return base();
//}

weston_view * xdg_surface_base_t::create_view() {
	auto view = weston_view_create(_surface);
	_views.push_back(view);
	return view;
}

void xdg_surface_base_t::set_output(weston_output * output) {
	_surface->output = output;
}

void xdg_surface_base_t::_weston_configure(weston_surface * es, int32_t sx,
		int32_t sy)
{
	auto ths = reinterpret_cast<xdg_surface_base_t *>(es->configure_private);
	ths->weston_configure(es, sx, sy);
}

auto xdg_surface_base_t::get_default_view() const -> weston_view * {
	return _default_view;
}

}


