/*
 * xdg_surface_popup_view.hxx
 *
 *  Created on: 9 juil. 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_SURFACE_POPUP_VIEW_HXX_
#define SRC_XDG_SURFACE_POPUP_VIEW_HXX_

#include "tree-types.hxx"
#include "xdg-surface-base-view.hxx"
#include "page-surface-interface.hxx"

namespace page {

class xdg_surface_popup_view_t : public xdg_surface_base_view_t {
private:

	weston_view * _default_view;

	__attribute__ ((deprecated)) page_surface_interface * _xdg_surface_popup;

	/* avoid copy */
	xdg_surface_popup_view_t(xdg_surface_popup_view_t const &) = delete;
	xdg_surface_popup_view_t & operator=(xdg_surface_popup_view_t const &) = delete;

	static void _weston_configure(weston_surface * es, int32_t sx, int32_t sy);

public:

	signal_t<xdg_surface_popup_view_t*> destroy;

	void xdg_popup_destroy(wl_client * client, wl_resource * resource);

	/** called on surface commit */
	void weston_configure(weston_surface * es, int32_t sx, int32_t sy);

	xdg_surface_popup_view_t(page_context_t * ctx, page_surface_interface * p);
	virtual ~xdg_surface_popup_view_t();

	virtual void add_popup_child(xdg_surface_popup_view_p child, int x, int y);
	void destroy_popup_child(xdg_surface_popup_view_t * c);

	virtual auto get_default_view() const -> weston_view *;

	void signal_destroy();

};

}


#endif /* SRC_XDG_SURFACE_POPUP_VIEW_HXX_ */
