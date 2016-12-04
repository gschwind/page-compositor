/*
 * tree-types.hxx
 *
 *  Created on: 15 juil. 2016
 *      Author: gschwind
 */

#ifndef SRC_TREE_TYPES_HXX_
#define SRC_TREE_TYPES_HXX_

#include <memory>

namespace page {

using namespace std;

class tree_t;
using tree_p = shared_ptr<tree_t>;
using tree_w = weak_ptr<tree_t>;

class page_root_t;
using page_root_p = shared_ptr<page_root_t>;
using page_root_w = weak_ptr<page_root_t>;

class workspace_t;
using workspace_p = shared_ptr<workspace_t>;
using workspace_w = weak_ptr<workspace_t>;

class viewport_t;
using viewport_p = shared_ptr<viewport_t>;
using viewport_w = weak_ptr<viewport_t>;

class split_t;
using split_p = shared_ptr<split_t>;
using split_w = weak_ptr<split_t>;

class notebook_t;
using notebook_p = shared_ptr<notebook_t>;
using notebook_w = weak_ptr<notebook_t>;

class view_t;
using view_p = shared_ptr<view_t>;
using view_w = weak_ptr<view_t>;

class xdg_shell_client_t;
using xdg_shell_client_p = shared_ptr<xdg_shell_client_t>;
using xdg_shell_client_w = weak_ptr<xdg_shell_client_t>;

}

#endif /* SRC_TREE_TYPES_HXX_ */
