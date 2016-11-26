/*
 * xdg_surface_popup_view.hxx
 *
 *  Created on: 9 juil. 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_SURFACE_POPUP_VIEW_HXX_
#define SRC_XDG_SURFACE_POPUP_VIEW_HXX_

#include "tree-types.hxx"
#include "view-base.hxx"
#include "page-surface-interface.hxx"

namespace page {

class view_popup_t : public view_base_t {
private:

	weston_view * _default_view;

	page_surface_interface * _page_surface;

	/* avoid copy */
	view_popup_t(view_popup_t const &) = delete;
	view_popup_t & operator=(view_popup_t const &) = delete;

	static void _weston_configure(weston_surface * es, int32_t sx, int32_t sy);

public:

	signal_t<view_popup_t*> destroy;

	void xdg_popup_destroy(wl_client * client, wl_resource * resource);

	/** called on surface commit */
	void weston_configure(weston_surface * es, int32_t sx, int32_t sy);

	view_popup_t(page_context_t * ctx, page_surface_interface * p);
	virtual ~view_popup_t();

	virtual void add_popup_child(view_popup_p child, int x, int y);
	void destroy_popup_child(view_popup_t * c);

	virtual auto get_default_view() const -> weston_view *;

	void signal_destroy();

};

}


#endif /* SRC_XDG_SURFACE_POPUP_VIEW_HXX_ */
