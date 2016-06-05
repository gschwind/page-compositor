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

#ifndef BOX_HXX_
#define BOX_HXX_

#include <X11/Xlib.h>
#include <xcb/xcb.h>

#include <cmath>
#include <sstream>

template<typename T>
struct i_rect_t {
	T x, y;
	T w, h;

	bool is_inside(T _x, T _y) const {
		return (x <= _x && _x < x + w && y <= _y && _y < y + h);
	}

	i_rect_t(i_rect_t const & b) :
			x(b.x), y(b.y), w(b.w), h(b.h) {
	}

	i_rect_t() :
			x(), y(), w(), h() {
	}

	i_rect_t(T x, T y, T w, T h) :
			x(x), y(y), w(w), h(h) {
	}

	i_rect_t(XRectangle const & rec) :
			x(rec.x), y(rec.y), w(rec.width), h(rec.height) {
	}

	i_rect_t(xcb_rectangle_t const & rec) :
			x(rec.x), y(rec.y), w(rec.width), h(rec.height) {
	}

	bool is_null() const {
		return (w <= 0 or h <= 0);
	}

	std::string to_string() const {
		std::ostringstream os;
		os << "(" << this->x << "," << this->y << "," << this->w << ","
				<< this->h << ")";
		return os.str();
	}

	bool operator==(i_rect_t const & b) const {
		return (b.x == x and b.y == y and b.w == w and b.h == h);
	}

	bool operator!=(i_rect_t const & b) const {
		return (b.x != x or b.y != y or b.w != w or b.h != h);
	}

	/* compute the smallest area that include 2 boxes */
	i_rect_t get_max_extand(i_rect_t const & b) const {
		i_rect_t<T> result;
		result.x = std::min(x, b.x);
		result.y = std::min(y, b.y);
		result.w = std::max(x + w, b.x + b.w) - result.x;
		result.h = std::max(y + h, b.y + b.h) - result.y;
		return result;
	}

	bool has_intersection(i_rect_t const & b) const {
		T left = std::max(x, b.x);
		T right = std::min(x + w, b.x + b.w);

		if(right <= left)
			return false;

		T top = std::max(y, b.y);
		T bottom = std::min(y + h, b.y + b.h);

		if(bottom <= top)
			return false;

		return true;

	}

	i_rect_t & floor() {
		x = ::floor(x);
		y = ::floor(y);
		w = ::floor(w);
		h = ::floor(h);
		return *this;
	}

	i_rect_t & round() {
		x = ::floor(x+0.5);
		y = ::floor(y+0.5);
		w = ::floor(w+0.5);
		h = ::floor(h+0.5);
		return *this;
	}

	i_rect_t & ceil() {
		x = ::ceil(x);
		y = ::ceil(y);
		w = ::ceil(w);
		h = ::ceil(h);
		return *this;
	}

	i_rect_t & operator&=(i_rect_t const & b) {
		if(this == &b) {
			*this = i_rect_t<T>(0, 0, 0, 0);
			return *this;
		}

		T left = std::max(x, b.x);
		T right = std::min(x + w, b.x + b.w);
		T top = std::max(y, b.y);
		T bottom = std::min(y + h, b.y + b.h);

		if (right <= left || bottom <= top) {
			*this = i_rect_t<T>(0, 0, 0, 0);
		} else {
			*this = i_rect_t<T>(left, top, right - left, bottom - top);
		}

		return *this;
	}

	i_rect_t operator&(i_rect_t const & b) const {
		i_rect_t<T> x = *this;
		return (x &= b);
	}

	template<typename U>
	operator i_rect_t<U>() const {
		return i_rect_t<U>(x, y, w, h);
	}

};

//typedef i_rect_t<double> i_rect;

using rect = i_rect_t<int>;


#endif /* BOX_HXX_ */
