/*
 * client.hxx
 *
 *  Created on: 25 sept. 2015
 *      Author: gschwind
 */

#ifndef SRC_CLIENT_HXX_
#define SRC_CLIENT_HXX_

#include <list>

#include "type.hxx"

namespace wpage {

using namespace std;

struct client_t {
	list<xdg_shell_t *> xdg_shell_list;

};

}



#endif /* SRC_CLIENT_HXX_ */
