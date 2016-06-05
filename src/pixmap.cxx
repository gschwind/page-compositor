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

#include "pixmap.hxx"
#include "exception.hxx"

pixmap_t::pixmap_t(unsigned width, unsigned height) :
	_w{width}, _h{height}
{

	_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

	if(cairo_surface_status(_surf) != CAIRO_STATUS_SUCCESS) {
		throw exception_t{"Unable to create cairo_surface in %s (format=%s,width=%u,height=%u)",
			__PRETTY_FUNCTION__, "ARGB", width, height};
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



