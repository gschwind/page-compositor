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

struct xdg_popup_v6_t : public connectable_t, public zxdg_popup_v6_vtable, public page_surface_interface {
	xdg_surface_v6_t *     _base;

	page_context_t *       _ctx;
	wl_client *            _client;
	uint32_t               _id;

	struct wl_resource *   self_resource;

	signal_t<xdg_popup_v6_t *> destroy;

	bool _is_configured;

	xdg_popup_v6_t(xdg_popup_v6_t const &) = delete;
	xdg_popup_v6_t & operator=(xdg_popup_v6_t const &) = delete;

	xdg_popup_v6_t(
			page_context_t * ctx,
			wl_client * client,
			xdg_surface_v6_t * surface,
			uint32_t id,
			struct wl_resource * parent,
			struct wl_resource * positioner);

	void surface_destroyed(xdg_surface_v6_t * s);
	void surface_commited(xdg_surface_v6_t * s);

	virtual ~xdg_popup_v6_t();

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
	virtual void send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) override;

};

}

#else

namespace page {
struct xdg_popup_v6_t;
}

#endif /* SRC_XDG_POPUP_V6_HXX_ */
