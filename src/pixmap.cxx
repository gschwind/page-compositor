/*
 * pixmap.cxx
 *
 *  Created on: 28 août 2015
 *      Author: gschwind
 */


#include "exception.hxx"
#include "pixmap.hxx"

namespace page {

pixmap_t::
pixmap_t(pixmap_format_e format, unsigned width, unsigned height) :
	_w{width},
	_h{height}
{
	_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, _w, _h);

	if(cairo_surface_status(_surf) != CAIRO_STATUS_SUCCESS) {
		throw exception_t{"Unable to create cairo_surface in %s (format=%s,width=%u,height=%u)",
			__PRETTY_FUNCTION__, format==PIXMAP_RGB?"RGB":"RGBA", width, height};
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

}


