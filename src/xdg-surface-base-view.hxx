/*
 * xdg_surface_base_view.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 */

#ifndef SRC_XDG_SURFACE_BASE_VIEW_HXX_
#define SRC_XDG_SURFACE_BASE_VIEW_HXX_

#include <typeinfo>
#include <memory>

#include "xdg-surface-base.hxx"

#include "tree.hxx"

namespace page {

using namespace std;

/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
struct view_base_t : public tree_t {
	view_base_t(view_base_t const & c) = delete;
	view_base_t();
	virtual ~view_base_t();

	virtual void add_popup_child(xdg_surface_popup_view_p child, int x, int y) = 0;

};

}

#endif /* SRC_XDG_SURFACE_BASE_VIEW_HXX_ */
