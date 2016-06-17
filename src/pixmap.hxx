/*
 * shared_pixmap.hxx
 *
 *  Created on: 14 juin 2014
 *      Author: gschwind
 */

#ifndef PIXMAP_HXX_
#define PIXMAP_HXX_

#include <libweston-0/compositor.h>
#include <cairo/cairo.h>
#include <memory>

#include "utils.hxx"

namespace page {

using namespace std;

class page_t;

enum pixmap_format_e {
	PIXMAP_RGB,
	PIXMAP_RGBA
};

/**
 * Self managed pixmap and cairo.
 **/
class pixmap_t {
	page_t * _ctx;
	uint32_t _serial;
	wl_resource * _resource;
	cairo_surface_t * _surf;
	unsigned _w, _h;
	weston_buffer * _wbuffer;
	weston_surface * _wsurface;

	void _request_buffer();

	pixmap_t(pixmap_t const & x) = delete;
	pixmap_t & operator=(pixmap_t const & x) = delete;
public:

	signal_t<pixmap_t *> on_ack_buffer;

	pixmap_t(page_t * ctx, pixmap_format_e format, unsigned width, unsigned height);
	~pixmap_t();

	cairo_surface_t * get_cairo_surface() const;
	unsigned witdh() const;
	unsigned height() const;

	void bind_buffer_manager();
	void ack_buffer(wl_client * client, wl_resource * resource,
			uint32_t serial, wl_resource * surface, wl_resource * buffer);

	uint32_t serial() const { return _serial; }
	weston_buffer * wbuffer() const { return _wbuffer; }
	weston_surface * wsurface() const { return _wsurface; };

};

using pixmap_p = shared_ptr<pixmap_t>;
using pixmap_w = weak_ptr<pixmap_t>;

}

#endif /* SHARED_PIXMAP_HXX_ */
