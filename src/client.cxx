/*
 * client.cxx
 *
 *  Created on: 10 juin 2016
 *      Author: gschwind
 */

#include "client.hxx"
#include "xdg-shell-server-protocol.h"

namespace page {

using namespace std;


static void _xdg_shell_destroy(wl_client * client, wl_resource * resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	client_shell_t::get(resource)->xdg_shell_destroy(client, resource);
}

static void _xdg_shell_use_unstable_version(wl_client * client,
		wl_resource * resource, int32_t version)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	client_shell_t::get(resource)->xdg_shell_use_unstable_version(client, resource, version);
}

static void _xdg_shell_get_xdg_surface(wl_client * client,
		wl_resource * resource, uint32_t id, wl_resource* surface_resource)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	client_shell_t::get(resource)->xdg_shell_get_xdg_surface(client, resource, id, surface_resource);
}

static void _xdg_shell_get_xdg_popup(wl_client * client, wl_resource * resource,
		uint32_t id, wl_resource * surface_resource,
		wl_resource * parent_resource, wl_resource * seat_resource,
		uint32_t serial, int32_t x, int32_t y)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	client_shell_t::get(resource)->xdg_shell_get_xdg_popup(client, resource, id, surface_resource,
			parent_resource, seat_resource, serial, x, y);
}

static void _xdg_shell_pong(wl_client * client, wl_resource * resource,
		uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
	client_shell_t::get(resource)->xdg_shell_pong(client, resource, serial);
}

static struct xdg_shell_interface _xdg_shell_implementation = {
	page::_xdg_shell_destroy,
	page::_xdg_shell_use_unstable_version,
	page::_xdg_shell_get_xdg_surface,
	page::_xdg_shell_get_xdg_popup,
	page::_xdg_shell_pong
};


void client_shell_t::xdg_shell_destroy(wl_client * client,
		wl_resource * resource)
{
	/* TODO */
}

void client_shell_t::xdg_shell_use_unstable_version(wl_client * client,
		wl_resource * resource, int32_t version)
{
	/* TODO */
}

void client_shell_t::xdg_shell_get_xdg_surface(wl_client * client,
		wl_resource * resource, uint32_t id, wl_resource * surface_resource)
{

	auto surface = resource_get<weston_surface>(surface_resource);

	auto xdg_surface = make_shared<xdg_surface_toplevel_t>(_ctx, client,
			surface, id);
	xdg_surface_toplevel_list.push_back(xdg_surface);

	printf("create (%p)\n", xdg_surface.get());

	weston_log("exit %s\n", __PRETTY_FUNCTION__);

}

void client_shell_t::xdg_shell_get_xdg_popup(wl_client * client,
		  wl_resource * resource,
		  uint32_t id,
		  wl_resource * surface_resource,
		  wl_resource * parent_resource,
		  wl_resource * seat_resource,
		  uint32_t serial,
		  int32_t x, int32_t y) {
	weston_log("call %s\n", __PRETTY_FUNCTION__);
//	/* In our case nullptr */
//	auto surface =
//		reinterpret_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));
//	auto shell = xdg_shell_t::get(resource);
//
//	weston_log("p=%p, x=%d, y=%d\n", surface, x, y);
//
//	auto xdg_popup = new xdg_popup_t(client, id, surface, x, y);

}

void client_shell_t::xdg_shell_pong(struct wl_client *client,
	 struct wl_resource *resource, uint32_t serial)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}



client_shell_t * client_shell_t::get(wl_resource * resource) {
	return reinterpret_cast<client_shell_t *>(wl_resource_get_user_data(resource));
}


void client_shell_t::xdg_shell_delete(struct wl_resource *resource) {
	client_shell_t::get(resource)->xdg_shell_resource = nullptr;

	/* TODO */
}


client_shell_t::client_shell_t(
		page_context_t * ctx,
		wl_client * client,
		uint32_t id) :
		_ctx{ctx},
		xdg_shell_resource{nullptr},
		client{client}
{

	/* allocate a wayland resource for the provided 'id' */
	xdg_shell_resource = wl_resource_create(client, &::xdg_shell_interface, 1,
			id);

	/**
	 * Define the implementation of the resource and the user_data,
	 * i.e. callbacks that must be used for this resource.
	 **/
	wl_resource_set_implementation(xdg_shell_resource,
			&_xdg_shell_implementation, this, &xdg_shell_delete);

}

}



