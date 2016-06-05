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

#include "compositor.hxx"

using namespace std;

using backend_init_func =
		int (*)(struct weston_compositor *c,
	    int *argc, char *argv[],
	    struct weston_config *config,
	    struct weston_backend_config *config_base);

void load_x11_backend(weston_compositor* ec) {
	weston_x11_backend_config config = {{ 0, }};
	weston_x11_backend_output_config default_output = { 0, };

	default_output.height = 1600;
	default_output.width = 1600;
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

static void bind_xdg_shell(struct wl_client *client, void *data,
				      uint32_t version, uint32_t id) {
	new xdg_shell_t{client, id};
}


compositor_t::compositor_t() {

    weston_log_set_handler(vprintf, vprintf);

	/* first create the wayland serveur */
	dpy = wl_display_create();

	auto sock_name = wl_display_add_socket_auto(dpy);
	weston_log("socket name = %s\n", sock_name);

	/* set the environment for children */
	setenv("WAYLAND_DISPLAY", sock_name, 1);

	/*
	 * Weston compositor will create all core globals:
	 *  - wl_compositor
	 *  - wl_output
	 *  - wl_subcompositor
	 *  - wl_presentation
	 *  - wl_data_device_manager
	 *  - wl_seat
	 *  - zwp_linux_dmabuf_v1
	 *  - [...]
	 *  but not the xdg_shell one.
	 *
	 */
	ec = weston_compositor_create(dpy, nullptr);
	weston_layer_init(&default_layer, &ec->cursor_layer.link);

	wl_global_create(dpy, &xdg_shell_interface, 1, NULL, bind_xdg_shell);

	connect_all();

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


	/** create the solid color background **/
	auto background = weston_surface_create(ec);
	weston_surface_set_color(background, 0.5, 0.5, 0.5, 1.0);
    weston_surface_set_size(background, 800, 600);
    pixman_region32_fini(&background->opaque);
    pixman_region32_init_rect(&background->opaque, 0, 0, 800, 600);
    weston_surface_damage(background);
    //pixman_region32_fini(&s->input);
    //pixman_region32_init_rect(&s->input, 0, 0, w, h);

    auto bview = weston_view_create(background);
	weston_view_set_position(bview, 0, 0);
	background->timeline.force_refresh = 1;
	//weston_layer_entry_insert(&default_layer.view_list, &bview->layer_link);

	auto stest = weston_surface_create(ec);
	weston_surface_set_size(stest, 800, 600);
    pixman_region32_fini(&stest->opaque);
    pixman_region32_init_rect(&stest->opaque, 0, 0, 800, 600);
    weston_surface_damage(stest);

    unsigned * data = (unsigned*)malloc(4*800*600);
    for(int i = 0; i < 800*600; ++i)
    	data[i] = 0x88ffff88;
    auto sbuffer = weston_buffer_from_memory(800, 600, 4*800, WL_SHM_FORMAT_ARGB8888, data);
    weston_surface_attach(stest, sbuffer);
    weston_surface_damage(stest);

    auto sview = weston_view_create(stest);
	weston_view_set_position(sview, 0, 0);
	stest->timeline.force_refresh = 1;
	weston_layer_entry_insert(&default_layer.view_list, &sview->layer_link);
    weston_surface_commit(stest);
    weston_surface_attach(stest, sbuffer);
    weston_surface_damage(stest);
    weston_surface_commit(stest);

}

void compositor_t::connect_all() {

	destroy.notify = [](wl_listener *l, void *data) { weston_log("compositor::destroy\n"); };
    create_surface.notify = [](wl_listener *l, void *data) { weston_log("compositor::create_surface\n"); };
    activate.notify = [](wl_listener *l, void *data) { weston_log("compositor::activate\n"); };
    transform.notify = [](wl_listener *l, void *data) { weston_log("compositor::transform\n"); };
    kill.notify = [](wl_listener *l, void *data) { weston_log("compositor::kill\n"); };
    idle.notify = [](wl_listener *l, void *data) { weston_log("compositor::idle\n"); };
    wake.notify = [](wl_listener *l, void *data) { weston_log("compositor::wake\n"); };
    show_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::show_input_panel\n"); };
    hide_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::hide_input_panel\n"); };
    update_input_panel.notify = [](wl_listener *l, void *data) { weston_log("compositor::update_input_panel\n"); };
    seat_created.notify = [](wl_listener *l, void *data) { weston_log("compositor::seat_created\n"); };
    output_created.notify = [](wl_listener *l, void *data) { weston_log("compositor::output_created\n"); };
    output_destroyed.notify = [](wl_listener *l, void *data) { weston_log("compositor::output_destroyed\n"); };
    output_moved.notify = [](wl_listener *l, void *data) { weston_log("compositor::output_moved\n"); };
    session.notify = [](wl_listener *l, void *data) { weston_log("compositor::session\n"); };


    wl_signal_add(&ec->destroy_signal, &destroy);
    wl_signal_add(&ec->create_surface_signal, &create_surface);
    wl_signal_add(&ec->activate_signal, &activate);
    wl_signal_add(&ec->transform_signal, &transform);
    wl_signal_add(&ec->kill_signal, &kill);
    wl_signal_add(&ec->idle_signal, &idle);
    wl_signal_add(&ec->wake_signal, &wake);
    wl_signal_add(&ec->show_input_panel_signal, &show_input_panel);
    wl_signal_add(&ec->hide_input_panel_signal, &hide_input_panel);
    wl_signal_add(&ec->update_input_panel_signal, &update_input_panel);
    wl_signal_add(&ec->seat_created_signal, &seat_created);
    wl_signal_add(&ec->output_created_signal, &output_created);
    wl_signal_add(&ec->output_destroyed_signal, &output_destroyed);
    wl_signal_add(&ec->output_moved_signal, &output_moved);
    wl_signal_add(&ec->session_signal, &session);
}

void compositor_t::run() {
	weston_compositor_wake(ec);
    wl_display_run(dpy);
}


