/*
 * page-surface-interface.hxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */

#ifndef SRC_SURFACE_HXX_
#define SRC_SURFACE_HXX_

#include <set>
#include <string>

#include <compositor.h>

#include "tree-types.hxx"

namespace page {

using namespace std;

/**
 * This interface allow allow views to communicate to backend
 **/
struct surface_t {

	view_w _master_view;

	/* parent for popup */
	surface_t * _parent;

	/* positionner */
	int32_t x_offset;
	int32_t y_offset;

	virtual ~surface_t() = default;

	virtual struct weston_surface * surface() const = 0;
	virtual struct weston_view * create_weston_view() = 0;
	virtual int32_t width() const = 0;
	virtual int32_t height() const = 0;
	virtual string const & title() const = 0;
	virtual void send_configure(int32_t width, int32_t height, set<uint32_t> const & states) = 0;
	virtual void send_close() = 0;
	virtual void send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

};

}

#else

struct surface_t;

#endif /* SRC_SURFACE_HXX_ */
