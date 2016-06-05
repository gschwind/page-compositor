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

#ifndef THEME_HXX_
#define THEME_HXX_

#include <typeinfo>

#include <cairo/cairo.h>

#include "color.hxx"

#include "theme-notebook.hxx"
#include "theme-split.hxx"
#include "theme-tab.hxx"
#include "pixmap.hxx"

struct margin_t {
	int top;
	int bottom;
	int left;
	int right;
};

//struct theme_alttab_entry_t {
//	//std::shared_ptr<icon64> icon;
//	std::string label;
//};

struct theme_dropdown_menu_entry_t {
	std::string label;
};

//struct theme_thumbnail_t {
//	std::shared_ptr<pixmap_t> pix;
//	std::shared_ptr<pixmap_t> title;
//};

class theme_t {

public:

	struct {
		margin_t margin;

		unsigned tab_height;
		unsigned iconic_tab_width;
		unsigned selected_close_width;
		unsigned selected_unbind_width;

		unsigned menu_button_width;
		unsigned close_width;
		unsigned hsplit_width;
		unsigned vsplit_width;
		unsigned mark_width;

		unsigned left_scroll_arrow_width;
		unsigned right_scroll_arrow_width;

	} notebook;

	struct {
		margin_t margin;
		unsigned width;
	} split;

	struct {
		margin_t margin;
		unsigned int title_height;
		unsigned int close_width;
		unsigned int bind_width;
	} floating;


	theme_t() { }
	virtual ~theme_t() { }

	virtual void render_split(cairo_t * cr, theme_split_t const * s) const = 0;
	virtual void render_notebook(cairo_t * cr, theme_notebook_t const * n) const = 0;
	virtual void render_empty(cairo_t * cr, rect const & area) const = 0;

	virtual void render_iconic_notebook(cairo_t * cr, vector<theme_tab_t> const & tabs) const = 0;

//	virtual void render_thumbnail(cairo_t * cr, rect position, theme_thumbnail_t const & t) const = 0;
//	virtual void render_thumbnail_title(cairo_t * cr, rect position, std::string const & title) const = 0;
//
//	virtual void render_popup_notebook0(cairo_t * cr,
//			icon64 * icon, unsigned int width,
//			unsigned int height, std::string const & title) const = 0;
//	virtual void render_popup_move_frame(cairo_t * cr,
//			icon64 * icon, unsigned int width,
//			unsigned int height, std::string const & title) const = 0;

	virtual void render_popup_split(cairo_t * cr, theme_split_t const * s, double current_split) const = 0;
	virtual void render_menuentry(cairo_t * cr, theme_dropdown_menu_entry_t const & item, rect const & area, bool selected) const = 0;
	virtual void update() = 0;

	virtual std::shared_ptr<pixmap_t> create_background_image(unsigned width, unsigned height) const = 0;

	virtual color_t const & get_focused_color() const = 0;
	virtual color_t const & get_selected_color() const = 0;
	virtual color_t const & get_normal_color() const = 0;
	virtual color_t const & get_mouse_over_color() const = 0;

};

#endif /* THEME_HXX_ */
