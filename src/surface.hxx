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

	/* data related to toplevel */
	surface_t * _transient_for;


	/* data related to popup */
	surface_t * _parent;
	struct weston_seat * _seat;
	uint32_t _serial;
	int32_t _x_offset;
	int32_t _y_offset;

	bool _has_popup_grab;

	surface_t();

	virtual ~surface_t() = default;

	virtual auto surface() const -> struct weston_surface * = 0;
	virtual auto create_weston_view() -> struct weston_view * = 0;
	virtual auto width() const -> int32_t = 0;
	virtual auto height() const -> int32_t = 0;
	virtual auto title() const -> string const & = 0;
	virtual void send_configure(int32_t width, int32_t height, set<uint32_t> const & states) = 0;
	virtual void send_close() = 0;
	virtual void send_configure_popup(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

};

}

#else

struct surface_t;

#endif /* SRC_SURFACE_HXX_ */
