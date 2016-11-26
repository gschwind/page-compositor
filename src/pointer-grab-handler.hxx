/*
 * Copyright (2016) Benoit Gschwind
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

#ifndef SRC_POINTER_GRAB_HANDLER_HXX_
#define SRC_POINTER_GRAB_HANDLER_HXX_

#include <compositor.h>

namespace page {

/**
 * a C++ wrapper to weston_pointer_grab.
 **/
struct pointer_grab_handler_t {

	struct _pod_t {
		weston_pointer_grab grab;
		pointer_grab_handler_t * ths;
	} base;

	pointer_grab_handler_t();

	virtual ~pointer_grab_handler_t() { }

	virtual void focus() = 0;
	virtual void motion(uint32_t time, weston_pointer_motion_event *event) = 0;
	virtual void button(uint32_t time, uint32_t button, uint32_t state) = 0;
	virtual void axis(uint32_t time, weston_pointer_axis_event *event) = 0;
	virtual void axis_source(uint32_t source) = 0;
	virtual void frame() = 0;
	virtual void cancel() = 0;

};

void pointer_start_grab(weston_pointer * pointer,
		pointer_grab_handler_t * grab);

void pointer_end_grab(weston_pointer * pointer);

}


#endif /* SRC_POINTER_GRAB_HANDLER_HXX_ */
