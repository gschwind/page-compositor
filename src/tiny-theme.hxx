/*
 * Copyright (2010-2016) Benoit Gschwind
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

#ifndef TINY_THEME_HXX_
#define TINY_THEME_HXX_

#include <pango/pangocairo.h>

#include <memory>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "utils.hxx"
#include "theme.hxx"
#include "color.hxx"
#include "config-handler.hxx"
#include "pixmap.hxx"
#include "simple2-theme.hxx"

using namespace std;

class tiny_theme_t : public simple2_theme_t {

	rect compute_notebook_bookmark_position(rect const & allocation) const;
	rect compute_notebook_vsplit_position(rect const & allocation) const;
	rect compute_notebook_hsplit_position(rect const & allocation) const;
	rect compute_notebook_close_position(rect const & allocation) const;
	rect compute_notebook_menu_position(rect const & allocation) const;

	void render_notebook_selected(
			cairo_t * cr,
			theme_notebook_t const & n,
			theme_tab_t const & data,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color,
			double border_width
	) const;

	void render_notebook_normal(
			cairo_t * cr,
			theme_tab_t const & data,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color
	) const;

public:
	tiny_theme_t(config_handler_t & conf);
	virtual ~tiny_theme_t();

	virtual void render_notebook(cairo_t * cr, theme_notebook_t const * n) const;

	virtual void render_iconic_notebook(
			cairo_t * cr,
			vector<theme_tab_t> const & tabs
	) const;

};


#endif /* SIMPLE_THEME_HXX_ */
