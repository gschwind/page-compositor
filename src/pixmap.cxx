/*
 * pixmap.cxx
 *
 *  Created on: 28 août 2015
 *      Author: gschwind
 */

#include "pixmap.hxx"

#include "exception.hxx"
#include "page.hxx"
#include "buffer-manager-server-protocol.h"

namespace page {

void pixmap_t::_request_buffer() {
	/* test create buffer */
	_serial = wl_display_next_serial(_ctx->_dpy);
	zzz_buffer_manager_send_get_buffer(_ctx->_buffer_manager_resource, _serial,
			_w, _h);
	wl_display_flush_clients(_ctx->_dpy);
}

pixmap_t::
pixmap_t(page_t * ctx, pixmap_format_e format, unsigned width, unsigned height) :
	_ctx{ctx},
	_w{width},
	_h{height},
	_resource{nullptr},
	_serial{0}
{

	if(not _ctx->_buffer_manager_resource) {
		/* wait for bind_buffer_manager call */
	} else {
		_request_buffer();
	}

}

pixmap_t::~pixmap_t() {
	cairo_surface_destroy(_surf);
}

cairo_surface_t * pixmap_t::get_cairo_surface() const {
	return _surf;
}

unsigned pixmap_t::witdh() const {
	return _w;
}

unsigned pixmap_t::height() const {
	return _h;
}

void pixmap_t::bind_buffer_manager() {
	if(not _resource)
		_request_buffer();
}

void pixmap_t::ack_buffer(wl_client * client, wl_resource * resource,
		uint32_t serial, wl_resource * surface, wl_resource * buffer) {
	wl_shm_buffer * b = wl_shm_buffer_get(buffer);
	resource = buffer;

	_resource = buffer;
	_wbuffer = weston_buffer_from_resource(buffer);
	_wsurface = reinterpret_cast<weston_surface*>(wl_resource_get_user_data(surface));

	_surf = cairo_image_surface_create_for_data((uint8_t*)wl_shm_buffer_get_data(b),
			CAIRO_FORMAT_ARGB32, wl_shm_buffer_get_width(b),
			wl_shm_buffer_get_height(b), wl_shm_buffer_get_stride(b));

}

}


