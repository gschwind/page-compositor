/*
 * client.hxx
 *
 *  Created on: 10 juin 2016
 *      Author: gschwind
 */

#ifndef SRC_CLIENT_HXX_
#define SRC_CLIENT_HXX_

#include <list>

#include "xdg-shell-interface.hxx"
#include "page_context.hxx"
#include "client_base.hxx"
#include "client_managed.hxx"

namespace page {

using namespace std;

struct client_shell_t : public xdg_shell_vtable {
	page_context_t * _ctx;

	wl_client * client;

	/* resource created for xdg_shell */
	wl_resource * xdg_shell_resource;

	/* list of surfaces belon the client */
	list<shared_ptr<xdg_surface_toplevel_t>> xdg_surface_toplevel_list;

	client_shell_t(page_context_t * ctx, wl_client * client, uint32_t id);

	static client_shell_t * get(wl_resource * resource);

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

#endif /* SRC_CLIENT_HXX_ */
