/*
 * shared_pixmap.hxx
 *
 *  Created on: 14 juin 2014
 *      Author: gschwind
 */

#ifndef PIXMAP_HXX_
#define PIXMAP_HXX_

#include <cairo/cairo.h>

namespace page {

class display_t;

using namespace std;

enum pixmap_format_e {
	PIXMAP_RGB,
	PIXMAP_RGBA
};

/**
 * Self managed pixmap and cairo.
 **/
class pixmap_t {
	cairo_surface_t * _surf;
	unsigned _w, _h;

	pixmap_t(pixmap_t const & x);
	pixmap_t & operator=(pixmap_t const & x);
public:

	pixmap_t(pixmap_format_e format, unsigned width, unsigned height);
	~pixmap_t();

	cairo_surface_t * get_cairo_surface() const;
	unsigned witdh() const;
	unsigned height() const;

};

}

#endif /* SHARED_PIXMAP_HXX_ */
