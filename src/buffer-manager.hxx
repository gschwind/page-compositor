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

#ifndef SRC_BUFFER_MANAGER_HXX_
#define SRC_BUFFER_MANAGER_HXX_

#include <map>
#include <wayland-client.h>
#include "buffer-manager-client-protocol.h"

namespace page {

struct buffer_t {
	wl_buffer * buffer;
	void *shm_data;
	int busy;
	wl_surface * surface;
};

struct buffer_manager_t {
	wl_display * display;
	wl_registry * registry;
	wl_compositor * compositor;
	wl_shm * shm;
	zzz_buffer_manager * buffer_manager;

	bool has_argb;

	std::map<uint32_t, buffer_t *> buffers;
};

void buffer_manager_main(int fd);

}

#endif /* SRC_BUFFER_MANAGER_HXX_ */
