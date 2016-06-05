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

#ifndef CONFIG_HANDLER_HXX_
#define CONFIG_HANDLER_HXX_

#include <utility>
#include <map>
#include <string>

namespace page {

class config_handler_t {

	typedef std::pair<std::string, std::string> _key_t;
	std::map<_key_t, std::string> _data;

	std::string const & find(char const * group, char const * key) const;

public:
	config_handler_t();
	~config_handler_t();

	void merge_from_file_if_exist(std::string const & filename);

	bool has_key(char const * groups, char const * key) const;

	std::string get_string(char const * groups, char const * key) const;
	double get_double(char const * groups, char const * key) const;
	long get_long(char const * group, char const * key) const;
	std::string get_value(char const * group, char const * key) const;

};


}



#endif /* CONFIG_HANDLER_HXX_ */
