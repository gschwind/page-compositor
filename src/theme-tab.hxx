/*
 * Copyright (2014-2016) Benoit Gschwind
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

#ifndef THEME_TAB_HXX_
#define THEME_TAB_HXX_

#include "color.hxx"

struct theme_tab_t {
	rect position;
	std::string title;
	//std::shared_ptr<icon16> icon;
	color_t tab_color;
	bool is_iconic;

	theme_tab_t() :
		position{},
		title{},
		//icon{},
		is_iconic{},
		tab_color{}
	{ }

	theme_tab_t(theme_tab_t const & x) :
		position{x.position},
		title{x.title},
		//icon{x.icon},
		is_iconic{x.is_iconic},
		tab_color{x.tab_color}
	{ }

};


#endif /* THEME_TAB_HXX_ */
