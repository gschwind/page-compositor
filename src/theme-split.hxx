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

#ifndef THEME_SPLIT_HXX_
#define THEME_SPLIT_HXX_

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

struct theme_split_t {
	int root_x, root_y;
	rect allocation;
	split_type_e type;
	double split;
	bool has_mouse_over;
};

#endif /* SPLIT_BASE_HXX_ */
