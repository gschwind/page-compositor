/*
 * client.cxx
 *
 *  Created on: 10 juin 2016
 *      Author: gschwind
 */

#include "client.hxx"

namespace page {

using namespace std;

client_t * client_t::get(wl_resource * resource) {
	return reinterpret_cast<client_t *>(wl_resource_get_user_data(resource));
}


void client_t::xdg_shell_delete(struct wl_resource *resource) {
	client_t::get(resource)->xdg_shell_resource = nullptr;

	/* TODO */
}


client_t::client_t(wl_client * client, uint32_t id) :
		xdg_shell_resource { nullptr }, client { client }
{

	/* allocate a wayland resource for the provided 'id' */
	xdg_shell_resource = wl_resource_create(client, &xdg_shell_interface, 1,
			id);



}

}



