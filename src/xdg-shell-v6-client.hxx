/*
 *
 * copyright (2016) Benoit Gschwind
 *
 */

#ifndef SRC_XDG_SHELL_V6_CLIENT_HXX_
#define SRC_XDG_SHELL_V6_CLIENT_HXX_

#include <list>

#include "xdg-shell-unstable-v6-interface.hxx"
#include "page_context.hxx"

#include "xdg-surface-base.hxx"
#include "xdg-surface-popup.hxx"
#include "xdg-surface-toplevel.hxx"

namespace page {

using namespace std;

struct xdg_shell_v6_client_t : protected connectable_t, public zxdg_shell_v6_vtable {
	page_context_t * _ctx;

	wl_client * client;
	wl_resource * self_resource;

	signal_t<xdg_shell_v6_client_t *> destroy;

	map<uint32_t, xdg_surface_toplevel_t *> xdg_surface_toplevel_map;
	map<uint32_t, xdg_surface_popup_t *> xdg_surface_popup_map;

	xdg_shell_v6_client_t(page_context_t * ctx, wl_client * client, uint32_t id);
	virtual ~xdg_shell_v6_client_t() { }

	void remove_all_transient(xdg_surface_toplevel_t * s);
	void remove_all_popup(xdg_surface_popup_t * s);

	void destroy_toplevel(xdg_surface_toplevel_t * s);
	void destroy_popup(xdg_surface_popup_t * s);

	/* zxdg_shell_v6_vtable */
	virtual void zxdg_shell_v6_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_shell_v6_create_positioner(struct wl_client * client, struct wl_resource * resource, uint32_t id) override;
	virtual void zxdg_shell_v6_get_xdg_surface(struct wl_client * client, struct wl_resource * resource, uint32_t id, struct wl_resource * surface) override;
	virtual void zxdg_shell_v6_pong(struct wl_client * client, struct wl_resource * resource, uint32_t serial) override;
	virtual void zxdg_shell_v6_delete_resource(struct wl_resource * resource) override;


};

}

#endif /* SRC_XDG_SHELL_CLIENT_HXX_ */
