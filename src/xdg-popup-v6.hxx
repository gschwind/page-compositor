/*
 * xdg-toplevel-v6.hxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_POPUP_V6_HXX_
#define SRC_XDG_POPUP_V6_HXX_

#include "xdg-shell-unstable-v6-interface.hxx"
#include "xdg-surface-v6.hxx"
#include "page-surface-interface.hxx"

namespace page {

using namespace std;
using namespace wcxx;

struct xdg_popup_v6_t : public zxdg_popup_v6_vtable, public page_surface_interface {

	page_context_t *       _ctx;
	wl_client *            _client;
	xdg_surface_v6_t *     _surface;
	uint32_t               _id;
	struct wl_resource *   _parent;
	struct wl_resource *   _positioner;

	struct wl_resource *   self_resource;

	view_w _master_view;

	signal_t<xdg_popup_v6_t *> destroy;

	xdg_popup_v6_t(xdg_popup_v6_t const &) = delete;
	xdg_popup_v6_t & operator=(xdg_popup_v6_t const &) = delete;

	xdg_popup_v6_t(
			page_context_t * ctx,
			wl_client * client,
			xdg_surface_v6_t * surface,
			uint32_t id,
			struct wl_resource * parent,
			struct wl_resource * positioner);

	void surface_commited(weston_surface * s);
	auto create_view() -> view_p;
	void destroy_all_views();

	virtual ~xdg_popup_v6_t() = default;

	/* zxdg_popup_v6_vtable */
	virtual void zxdg_popup_v6_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_popup_v6_grab(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial) override;
	virtual void zxdg_popup_v6_delete_resource(struct wl_resource * resource) override;

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

#else

namespace page {
struct xdg_popup_v6_t;
}

#endif /* SRC_XDG_POPUP_V6_HXX_ */
