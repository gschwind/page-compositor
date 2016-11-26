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

#ifndef SRC_KEYBOARD_GRAB_HANDLER_HXX_
#define SRC_KEYBOARD_GRAB_HANDLER_HXX_

#include <compositor.h>

namespace page {

/**
 * a C++ wrapper to weston_keyboard_grab.
 **/
struct keyboard_grab_handler_t {

	struct _pod_t {
		weston_keyboard_grab grab;
		keyboard_grab_handler_t * ths;
	} base;

	keyboard_grab_handler_t();

	virtual ~keyboard_grab_handler_t() { }

	virtual void key(uint32_t time, uint32_t key, uint32_t state) = 0;
	virtual void modifiers(uint32_t serial,
			  uint32_t mods_depressed, uint32_t mods_latched,
			  uint32_t mods_locked, uint32_t group) = 0;
	virtual void cancel() = 0;

};

void keyboard_start_grab(weston_keyboard * keyboard,
		keyboard_grab_handler_t * grab);

void keyboard_end_grab(weston_keyboard * keyboard);

}


#endif /* SRC_KEYBOARD_GRAB_HANDLER_HXX_ */
