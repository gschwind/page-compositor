/*
 * Copyright (2010-2016) Benoit Gschwind
 *
 * This file is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SRC_WL_SHELL_SURFACE_HXX_
#define SRC_WL_SHELL_SURFACE_HXX_

#include <string>
#include <vector>
#include <typeinfo>
#include <compositor.h>

#include "tree-types.hxx"
#include "listener.hxx"

#include "wayland-interface.hxx"
#include "icon_handler.hxx"
#include "theme.hxx"

#include "floating_event.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_pixmap.hxx"

#include "xdg-surface-base.hxx"
#include "xdg-surface-popup.hxx"
#include "page-surface-interface.hxx"

namespace page {

class page_t;

using namespace wcxx;
using namespace std;

struct wl_shell_surface_t :
	public wl_shell_surface_vtable,
	public page_surface_interface {

	friend class page::page_t;

	page_context_t *        _ctx;
	struct wl_client *      _client;
	struct weston_surface * _surface;
	uint32_t                _id;
	struct wl_resource *    _resource;
	struct wl_listener      _surface_destroy;

	int32_t _width;
	int32_t _heigth;

	listener_t<struct weston_surface> on_surface_destroy;
	listener_t<struct weston_surface> on_surface_commit;

	struct _state {
		std::string title;
		bool fullscreen;
		bool maximized;
		bool minimized;
		wl_resource * transient_for;
		rect geometry;

		_state() {
			fullscreen = false;
			maximized = false;
			minimized = false;
			title = "";
			transient_for = nullptr;
			geometry = rect{0,0,0,0};
		}

	} _current;

	wl_shell_surface_t * _parent;
	std::string str_class;

	/* 0 if ack by client, otherwise the last serial sent */
	uint32_t _ack_serial;

	signal_t<wl_shell_surface_t *> destroy;

	static auto get(struct wl_resource * r) -> wl_shell_surface_t *;

	/* private to avoid copy */
	wl_shell_surface_t(wl_shell_surface_t const &) = delete;
	wl_shell_surface_t & operator=(wl_shell_surface_t const &) = delete;

	wl_shell_surface_t(
			page_context_t * ctx,
			wl_client * client,
			weston_surface * surface,
			uint32_t id);
	virtual ~wl_shell_surface_t();

	void destroy_all_views();

	void surface_commited(struct weston_surface * s);
	void surface_destroyed(struct weston_surface * s);

	/* wl_shell_surface */
	virtual void wl_shell_surface_pong(struct wl_client * client, struct wl_resource * resource, uint32_t serial) override;
	virtual void wl_shell_surface_move(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial) override;
	virtual void wl_shell_surface_resize(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, uint32_t edges) override;
	virtual void wl_shell_surface_set_toplevel(struct wl_client * client, struct wl_resource * resource) override;
	virtual void wl_shell_surface_set_transient(struct wl_client * client, struct wl_resource * resource, struct wl_resource * parent, int32_t x, int32_t y, uint32_t flags) override;
	virtual void wl_shell_surface_set_fullscreen(struct wl_client * client, struct wl_resource * resource, uint32_t method, uint32_t framerate, struct wl_resource * output) override;
	virtual void wl_shell_surface_set_popup(struct wl_client * client, struct wl_resource * resource, struct wl_resource * seat, uint32_t serial, struct wl_resource * parent, int32_t x, int32_t y, uint32_t flags) override;
	virtual void wl_shell_surface_set_maximized(struct wl_client * client, struct wl_resource * resource, struct wl_resource * output) override;
	virtual void wl_shell_surface_set_title(struct wl_client * client, struct wl_resource * resource, const char * title) override;
	virtual void wl_shell_surface_set_class(struct wl_client * client, struct wl_resource * resource, const char * class_) override;
	virtual void wl_shell_surface_delete_resource(struct wl_resource * resource) override;

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
struct wl_shell_surface_t;
}

#endif /* SRC_WL_SHELL_SURFACE_HXX_ */
