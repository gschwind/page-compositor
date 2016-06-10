/*
 * client.hxx
 *
 *  Created on: 10 juin 2016
 *      Author: gschwind
 */

#ifndef SRC_CLIENT_HXX_
#define SRC_CLIENT_HXX_

#include <list>

#include "client_base.hxx"

namespace page {

using namespace std;

struct client_t {
	wl_client * client;

	/* resource created for xdg_shell */
	wl_resource * xdg_shell_resource;

	/* list of surfaces belon the client */
	list<shared_ptr<xdg_surface_base_t>> xdg_shell_surfaces;

	client_t(wl_client * client, uint32_t id);

	static client_t * get(wl_resource * resource);

	static void xdg_shell_delete(wl_resource * resource);

};

}

#endif /* SRC_CLIENT_HXX_ */
