/*
 * xdg_surface_base_view.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 */

#ifndef SRC_XDG_SURFACE_BASE_VIEW_HXX_
#define SRC_XDG_SURFACE_BASE_VIEW_HXX_

#include <memory>

#include "xdg-surface-base.hxx"

#include "tree.hxx"

namespace page {

using namespace std;

/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
struct xdg_surface_base_view_t : public tree_t {
	xdg_surface_base_view_t(xdg_surface_base_view_t const & c) = delete;
	xdg_surface_base_view_t();
	virtual ~xdg_surface_base_view_t();

};

}

#endif /* SRC_XDG_SURFACE_BASE_VIEW_HXX_ */
