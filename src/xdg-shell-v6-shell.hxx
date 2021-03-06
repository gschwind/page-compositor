/*
 *
 * copyright (2016) Benoit Gschwind
 *
 */

#ifndef SRC_XDG_SHELL_V6_SHELL_HXX_
#define SRC_XDG_SHELL_V6_SHELL_HXX_

#include <list>
#include <map>

#include "xdg-shell-unstable-v6-interface.hxx"

#include "page_context.hxx"

#include "xdg-shell-v5-surface-base.hxx"
#include "xdg-shell-v5-surface-popup.hxx"
#include "xdg-shell-v5-surface-toplevel.hxx"
#include "xdg-shell-v6-positioner.hxx"
#include "xdg-shell-v6-surface.hxx"


namespace page {

using namespace std;

struct xdg_shell_v6_client_t : protected connectable_t, public zxdg_shell_v6_vtable {
	page_context_t * _ctx;

	wl_client * client;
	wl_resource * self_resource;

	signal_t<xdg_shell_v6_client_t *> destroy;

	map<uint32_t, xdg_surface_v6_t *> xdg_surface_map;
	map<uint32_t, xdg_positioner_v6_t *> xdg_positioner_v6_map;

	void destroy_surface(xdg_surface_v6_t * s);
	void destroy_positionner(xdg_positioner_v6_t * p);

	xdg_shell_v6_client_t(page_context_t * ctx, wl_client * client, uint32_t id);

	virtual ~xdg_shell_v6_client_t() = default;

	/* zxdg_shell_v6_vtable */
	virtual void zxdg_shell_v6_destroy(struct wl_client * client, struct wl_resource * resource) override;
	virtual void zxdg_shell_v6_create_positioner(struct wl_client * client, struct wl_resource * resource, uint32_t id) override;
	virtual void zxdg_shell_v6_get_xdg_surface(struct wl_client * client, struct wl_resource * resource, uint32_t id, struct wl_resource * surface) override;
	virtual void zxdg_shell_v6_pong(struct wl_client * client, struct wl_resource * resource, uint32_t serial) override;
	virtual void zxdg_shell_v6_delete_resource(struct wl_resource * resource) override;


};

}

#endif /* SRC_XDG_SHELL_CLIENT_HXX_ */
