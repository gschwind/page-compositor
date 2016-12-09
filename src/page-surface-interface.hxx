/*
 * page-surface-interface.hxx
 *
 *  Created on: Nov 26, 2016
 *      Author: gschwind
 */

#ifndef SRC_PAGE_SURFACE_INTERFACE_HXX_
#define SRC_PAGE_SURFACE_INTERFACE_HXX_

#include <set>
#include <string>

#include <compositor.h>

#include "tree-types.hxx"

namespace page {

using namespace std;

/**
 * This interface allow allow views to communicate to backend
 **/
struct page_surface_interface {

	view_w _master_view;

	virtual ~page_surface_interface() = default;

	virtual struct weston_surface * surface() const = 0;
	virtual struct weston_view * create_weston_view() = 0;
	virtual int32_t width() const = 0;
	virtual int32_t height() const = 0;
	virtual string const & title() const = 0;
	virtual void send_configure(int32_t width, int32_t height, set<uint32_t> const & states) = 0;
	virtual void send_close() = 0;

};

}

#else

struct page_surface_interface;

#endif /* SRC_PAGE_SURFACE_INTERFACE_HXX_ */
