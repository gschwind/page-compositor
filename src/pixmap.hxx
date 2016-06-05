/*
 * Copyright (2014-2016) Benoit Gschwind
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

#ifndef PIXMAP_HXX_
#define PIXMAP_HXX_

#include <cairo/cairo.h>

using namespace std;

class pixmap_t {
	cairo_surface_t * _surf;
	unsigned _w, _h;

	pixmap_t(pixmap_t const & x);
	pixmap_t & operator=(pixmap_t const & x);
public:

	pixmap_t(unsigned width, unsigned height);
	~pixmap_t();

	cairo_surface_t * get_cairo_surface() const;
	unsigned witdh() const;
	unsigned height() const;

};

#endif /* SHARED_PIXMAP_HXX_ */
