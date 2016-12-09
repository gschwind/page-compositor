/*

 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef XDG_SURFACE_POPUP_HXX_
#define XDG_SURFACE_POPUP_HXX_

#include <memory>
#include <compositor.h>

#include "xdg-shell-unstable-v5-interface.hxx"

#include "renderable.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"
#include "renderable_pixmap.hxx"

#include "xdg-surface-base.hxx"
#include "page-surface-interface.hxx"

namespace page {

using namespace wcxx;
using namespace std;

struct xdg_surface_popup_t :
		public xdg_surface_base_t,
		public xdg_popup_vtable,
		public page_surface_interface {

	wl_client * client;
	wl_resource * resource;
	uint32_t id;
	weston_surface * _surface;
	xdg_surface_base_t * parent;
	weston_seat * seat;
	uint32_t serial;
	int32_t x;
	int32_t y;

	signal_t<xdg_surface_popup_t *> destroy;

	auto create_view() -> view_p;
	auto master_view() -> view_w;

	/* avoid copy */
	xdg_surface_popup_t(xdg_surface_popup_t const &) = delete;
	xdg_surface_popup_t & operator=(xdg_surface_popup_t const &) = delete;

	/** called on surface commit */
	virtual void surface_commited(weston_surface * es) override;
	virtual void surface_destroyed(weston_surface * es) override;

	xdg_surface_popup_t(page_context_t * ctx,
			  wl_client * client,
			  wl_resource * resource,
			  uint32_t id,
			  weston_surface * surface,
			  xdg_surface_base_t * parent,
			  weston_seat * seat,
			  uint32_t serial,
			  int32_t x, int32_t y);
	~xdg_surface_popup_t();

	static auto get(wl_resource * r) -> xdg_surface_popup_t *;

	virtual view_p base_master_view();

	void destroy_all_views();

	/* xdg_popup_vtable */
	virtual void xdg_popup_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void xdg_popup_delete_resource(struct wl_resource * resource) override;

	/* page_surface_interface */
	virtual weston_surface * surface() const override;
	virtual weston_view * create_weston_view() override;
	virtual int32_t width() const override;
	virtual int32_t height() const override;
	virtual string const & title() const override;
	virtual void send_configure(int32_t width, int32_t height, set<uint32_t> const & states) override;
	virtual void send_close() override;

};

}

#endif /* XDG_SURFACE_POPUP_HXX_ */
