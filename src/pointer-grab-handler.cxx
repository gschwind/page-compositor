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


#include "pointer-grab-handler.hxx"


namespace page {

static void _focus(weston_pointer_grab * grab) {
	pointer_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->focus();
}

static void _motion(weston_pointer_grab * grab, uint32_t time, weston_pointer_motion_event *event) {
	pointer_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->motion(time, event);
}

static void _button(weston_pointer_grab * grab, uint32_t time, uint32_t button, uint32_t state) {
	pointer_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->button(time, button, state);
}

static void _axis(weston_pointer_grab * grab, uint32_t time, weston_pointer_axis_event *event) {
	pointer_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->axis(time, event);
}

static void _axis_source(weston_pointer_grab * grab, uint32_t source) {
	pointer_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->axis_source(source);
}

static void _frame(weston_pointer_grab * grab) {
	pointer_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->frame();
}

static void _cancel(weston_pointer_grab * grab) {
	pointer_grab_handler_t::_pod_t * pod = wl_container_of(grab, pod, grab);
	pod->ths->cancel();
}

static weston_pointer_grab_interface _pointer_grab_handler_interface = {
		_focus,
		_motion,
		_button,
		_axis,
		_axis_source,
		_frame,
		_cancel
};

pointer_grab_handler_t::pointer_grab_handler_t() {
	base.grab.interface = &_pointer_grab_handler_interface;
	base.grab.pointer = nullptr;
	base.ths = this;
}

void pointer_start_grab(weston_pointer * pointer,
		pointer_grab_handler_t * grab) {
	weston_pointer_start_grab(pointer, &grab->base.grab);
}

void pointer_end_grab(weston_pointer * pointer) {
	weston_pointer_end_grab(pointer);
}


}
