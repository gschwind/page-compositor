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

#ifndef SRC_XDG_POPUP_INTERFACE_HXX_
#define SRC_XDG_POPUP_INTERFACE_HXX_

namespace page {

struct xdg_popup_interface {

	virtual ~xdg_popup_interface() = 0;

	virtual void xdg_popup_destroy(wl_client * client,
			wl_resource * resource) = 0;

};

}

#endif /* SRC_XDG_POPUP_INTERFACE_HXX_ */
