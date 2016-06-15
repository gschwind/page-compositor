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

#include "buffer-manager.hxx"

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <libweston-0/compositor.h>
#include "buffer-manager-client-protocol.h"

namespace page {

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
	/* TODO */
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

int
os_fd_set_cloexec(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		return -1;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		return -1;

	return 0;
}

static int
set_cloexec_or_close(int fd)
{
	if (os_fd_set_cloexec(fd) != 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static int
create_tmpfile_cloexec(char *tmpname)
{
	int fd;

#ifdef HAVE_MKOSTEMP
	fd = mkostemp(tmpname, O_CLOEXEC);
	if (fd >= 0)
		unlink(tmpname);
#else
	fd = mkstemp(tmpname);
	if (fd >= 0) {
		fd = set_cloexec_or_close(fd);
		unlink(tmpname);
	}
#endif

	return fd;
}


/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 *
 * If the C library implements posix_fallocate(), it is used to
 * guarantee that disk space is available for the file at the
 * given size. If disk space is insufficent, errno is set to ENOSPC.
 * If posix_fallocate() is not supported, program may receive
 * SIGBUS on accessing mmap()'ed file contents instead.
 */
int
os_create_anonymous_file(off_t size)
{
	static const char tpl[] = "/weston-shared-XXXXXX";
	const char *path;
	char *name;
	int fd;
	int ret;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		errno = ENOENT;
		return -1;
	}

	asprintf(&name, "%s%s", path, tpl);
	if (!name)
		return -1;

	fd = create_tmpfile_cloexec(name);

	free(name);

	if (fd < 0)
		return -1;

#ifdef HAVE_POSIX_FALLOCATE
	ret = posix_fallocate(fd, 0, size);
	if (ret != 0) {
		close(fd);
		errno = ret;
		return -1;
	}
#else
	ret = ftruncate(fd, size);
	if (ret < 0) {
		close(fd);
		return -1;
	}
#endif

	return fd;
}

static int
create_shm_buffer(buffer_manager_t * bm, buffer_t *buffer,
		  int width, int height, uint32_t format)
{
	struct wl_shm_pool *pool;
	int fd, size, stride;
	void *data;

	stride = width * 4;
	size = stride * height;

	fd = os_create_anonymous_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %m\n",
			size);
		return -1;
	}

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %m\n");
		close(fd);
		return -1;
	}

	pool = wl_shm_create_pool(bm->shm, fd, size);
	buffer->buffer = wl_shm_pool_create_buffer(pool, 0,
						   width, height,
						   stride, format);
	wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
	wl_shm_pool_destroy(pool);
	close(fd);

	buffer->shm_data = data;

	return 0;
}

static void
buffer_manager_shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
	buffer_manager_t * bm = reinterpret_cast<buffer_manager_t*>(data);

//	if (format == WL_SHM_FORMAT_XRGB8888)
//		d->has_xrgb = true;
}

struct wl_shm_listener buffer_manager_shm_listener = {
	buffer_manager_shm_format
};

static void zzz_buffer_manager_get_buffer(void *data,
		   struct zzz_buffer_manager *zzz_buffer_manager,
		   uint32_t serial,
		   uint32_t width,
		   uint32_t height) {

	buffer_manager_t * bm = reinterpret_cast<buffer_manager_t*>(data);

	struct buffer_t * buffer;
	int ret = 0;

	if (!buffer->buffer) {
		ret = create_shm_buffer(bm, buffer,
					width, height, WL_SHM_FORMAT_XRGB8888);

		/* paint the padding */
		memset(buffer->shm_data, 0xff, width * height * 4);
	}

	zzz_buffer_manager_ack_buffer(bm->buffer_namager, serial, buffer->buffer);

}

static const struct zzz_buffer_manager_listener _zzz_buffer_manager_listener = {
		zzz_buffer_manager_get_buffer
};

static void
buffer_manager_global(void *data, struct wl_registry *registry, uint32_t id,
              const char *interface, uint32_t version)
{
	buffer_manager_t * bm = reinterpret_cast<buffer_manager_t*>(data);

	weston_log("call %s\n", __PRETTY_FUNCTION__);

    if (strcmp(interface, "zzz_buffer_manager") == 0
    		&& version >= 1) {
    	bm->buffer_namager = reinterpret_cast<zzz_buffer_manager*>(wl_registry_bind(registry, id,
    			&zzz_buffer_manager_interface, 1));
    	zzz_buffer_manager_add_listener(bm->buffer_namager,
    			&_zzz_buffer_manager_listener, bm);

    	weston_log("FOUND\n");
    } else if (strcmp(interface, "wl_shm") == 0) {
    	bm->shm = reinterpret_cast<wl_shm*>(wl_registry_bind(registry,
					  id, &wl_shm_interface, 1));
		wl_shm_add_listener(bm->shm, &buffer_manager_shm_listener, bm);
	}

}

static void
buffer_manager_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	weston_log("call %s\n", __PRETTY_FUNCTION__);
}

static const struct wl_registry_listener buffer_manager_global_listener = {
	buffer_manager_global,
	buffer_manager_global_remove
};

void buffer_manager_main(int fd) {
	buffer_manager_t mgr;

	weston_log("call %s\n", __PRETTY_FUNCTION__);

	auto dpy = wl_display_connect_to_fd(fd);
	auto registry = wl_display_get_registry(dpy);
	wl_registry_add_listener(registry, &buffer_manager_global_listener, &mgr);

	wl_display_roundtrip(dpy);
	if (mgr.shm == NULL) {
		fprintf(stderr, "No wl_shm global\n");
		exit(1);
	}

	wl_display_roundtrip(dpy);


	bool done = false;
	while (!done) {
		if (wl_display_dispatch(dpy) < 0) {
			done = true;
		}
	}

}

}

