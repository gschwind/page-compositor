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
#include "config.hxx"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <libweston-0/compositor.h>
#include <libweston-0/compositor-x11.h>


using backend_init_func =
		int (*)(struct weston_compositor *c,
	    int *argc, char *argv[],
	    struct weston_config *config,
	    struct weston_backend_config *config_base);

void load_x11_backend(weston_compositor* ec) {
	weston_x11_backend_config config = {{ 0, }};
	weston_x11_backend_output_config default_output = { 0, };

	default_output.height = 600;
	default_output.width = 800;
	default_output.name = strdup("Wayland output");
	default_output.scale = 1;
	default_output.transform = WL_OUTPUT_TRANSFORM_NORMAL;

	config.base.struct_size = sizeof(weston_x11_backend_config);
	config.base.struct_version = WESTON_X11_BACKEND_CONFIG_VERSION;

	config.fullscreen = 0;
	config.no_input = 0;
	config.num_outputs = 1;
	config.outputs = &default_output;
	config.use_pixman = 0;

	auto backend_init = reinterpret_cast<backend_init_func>(
			weston_load_module("x11-backend.so", "backend_init"));
	if (!backend_init)
		return;

	backend_init(ec, NULL, NULL, NULL, &config.base);

	free(default_output.name);

}

int main(int argc, char** argv) {

    weston_log_set_handler(vprintf, vprintf);

	/* first create the wayland serveur */
	auto dpy = wl_display_create();

	auto sock_name = wl_display_add_socket_auto(dpy);
	weston_log("socket name = %s\n", sock_name);

	/* set the environment for children */
	setenv("WAYLAND_DISPLAY", sock_name, 1);

	auto ec = weston_compositor_create(dpy, nullptr);

	/* setup the keyboard layout (MANDATORY) */
	xkb_rule_names names = {
			"",					/*rules*/
			"pc104",			/*model*/
			"us",				/*layout*/
			"",					/*variant*/
			""					/*option*/
	};
	weston_compositor_set_xkb_rule_names(ec, &names);
	load_x11_backend(ec);

	return EXIT_SUCCESS;
}

