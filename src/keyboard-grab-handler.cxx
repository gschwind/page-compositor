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

#include "keyboard-grab-handler.hxx"

namespace page {

static void _key(weston_keyboard_grab * grab, uint32_t time, uint32_t key,
		uint32_t state)
{
	keyboard_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->key(time, key, state);
}

static void _modifiers(weston_keyboard_grab * grab, uint32_t serial,
		uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
		uint32_t group)
{
	keyboard_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->modifiers(serial, mods_depressed, mods_latched, mods_locked, group);
}

static void _cancel(weston_keyboard_grab *grab)
{
	keyboard_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->cancel();
}

static weston_keyboard_grab_interface _keyboard_grab_handler_inteface = {
		_key,
		_modifiers,
		_cancel
};

keyboard_grab_handler_t::keyboard_grab_handler_t() {
	base.grab.interface = &_keyboard_grab_handler_inteface;
	base.grab.keyboard = nullptr;
	base.ths = this;
}

void keyboard_start_grab(weston_keyboard * keyboard,
		keyboard_grab_handler_t * grab) {
	weston_keyboard_start_grab(keyboard, &grab->base.grab);
}

void keyboard_end_grab(weston_keyboard * keyboard) {
	weston_keyboard_end_grab(keyboard);
}

}

