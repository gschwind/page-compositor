/*
 * client.hxx
 *
 *  Created on: 10 juin 2016
 *      Author: gschwind
 */

#ifndef SRC_XDG_SHELL_CLIENT_HXX_
#define SRC_XDG_SHELL_CLIENT_HXX_

#include <list>

#include "xdg-shell-interface.hxx"
#include "page_context.hxx"
#include "xdg-surface-base.hxx"
#include "xdg-surface-popup.hxx"
#include "xdg-surface-toplevel.hxx"

namespace page {

using namespace std;

struct xdg_shell_client_t : public xdg_shell_vtable {
	page_context_t * _ctx;

	wl_client * client;

	/* resource created for xdg_shell */
	wl_resource * xdg_shell_resource;

	signal_t<xdg_shell_client_t *> destroy;
//	signal_t<xdg_shell_client_t *, xdg_surface_toplevel_t *> create_toplevel;
//	signal_t<xdg_shell_client_t *, xdg_surface_popup_t *> create_popup;


	map<uint32_t, xdg_surface_toplevel_t *> xdg_surface_toplevel_map;
	map<uint32_t, xdg_surface_popup_t *> xdg_surface_popup_map;

	xdg_shell_client_t(page_context_t * ctx, wl_client * client, uint32_t id);

	void remove_all_transient(xdg_surface_toplevel_t * s);
	void remove_all_popup(xdg_surface_popup_t * s);

	void destroy_toplevel(xdg_surface_toplevel_t * s);
	void destroy_popup(xdg_surface_popup_t * s);


	static xdg_shell_client_t * get(wl_resource * resource);

	static void xdg_shell_delete(wl_resource * resource);

	virtual void xdg_shell_destroy(wl_client * client, wl_resource * resource)
			override;
	virtual void xdg_shell_use_unstable_version(wl_client * client,
			wl_resource * resource, int32_t version) override;
	virtual void xdg_shell_get_xdg_surface(wl_client * client,
			wl_resource * resource, uint32_t id, wl_resource * surface_resource)
					override;
	virtual void xdg_shell_get_xdg_popup(wl_client * client,
			wl_resource * resource, uint32_t id, wl_resource * surface,
			wl_resource * parent, wl_resource* seat_resource, uint32_t serial,
			int32_t x, int32_t y) override;
	virtual void xdg_shell_pong(wl_client * client, wl_resource * resource,
			uint32_t serial) override;

};

}

#endif /* SRC_XDG_SHELL_CLIENT_HXX_ */
