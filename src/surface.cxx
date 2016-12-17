/*
 * surface.cxx
 *
 *  Created on: 9 déc. 2016
 *      Author: gschwind
 */


#include "surface.hxx"

namespace page {

using namespace std;

surface_t::surface_t() :
_parent{nullptr},
_transient_for{nullptr},
_x_offset{0},
_y_offset{0},
_seat{nullptr},
_serial{0},
_has_popup_grab{false}
{

}

}

