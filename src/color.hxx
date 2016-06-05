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

#ifndef COLOR_HXX_
#define COLOR_HXX_

#include <stdexcept>
#include <cairo/cairo.h>

struct color_t {
	double r, g, b, a;

	color_t(color_t const & x) : r{x.r}, g{x.g}, b{x.b}, a{x.a} { }

	color_t() : r{0.0}, g{0.0}, b{0.0}, a{0.0} { }

	color_t(double r, double g, double b, double a = 0.0): r{r}, g{g}, b{b}, a{a} { }

	color_t(int r, int g, int b, int a = 0): r{r/255.0}, g{g/255.0}, b{b/255.0}, a{a/255.0} { }


	color_t(unsigned color) {
		set(color);
	}

	color_t(std::string const & scolor) {
		set(scolor);
	}

	void set(std::string const & scolor) {
		if(scolor.size() == 6 || scolor.size() == 8) {
			char const * c = scolor.c_str();
			unsigned x = strtoul(c, 0, 16);
			set(x);
		} else {
			throw std::runtime_error("bad color format");
		}
	}

	void set(unsigned color) {
		/** convert common alpha sense to cairo alpha **/
		a = 1.0 - ((color >> 24) & 0x000000ff) / 255.0;
		r = ((color >> 16) & 0x000000ff) / 255.0;
		g = ((color >> 8) & 0x000000ff) / 255.0;
		b = ((color >> 0) & 0x000000ff) / 255.0;
	}

	color_t operator*(double f) const {
		color_t ret{*this};
		ret.r *= f;
		ret.g *= f;
		ret.b *= f;
		return ret;
	}

};

inline void cairo_set_source_color_alpha(cairo_t * cr, color_t const & color) {
	cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
}

inline void cairo_set_source_color(cairo_t * cr, color_t const & color) {
	cairo_set_source_rgba(cr, color.r, color.g, color.b, 1.0);
}

#endif /* COLOR_HXX_ */
