/*
 * key_desc.hxx
 *
 *  Created on: 12 juil. 2014
 *      Author: gschwind
 */

#ifndef KEY_DESC_HXX_
#define KEY_DESC_HXX_

#include <string>
#include <xkbcommon/xkbcommon.h>
#include <compositor.h>

#include "exception.hxx"

namespace page {

using namespace std;

struct key_desc_t {
	string keysym_name;
	xkb_keysym_t ks;
	uint32_t mod;

	bool operator==(key_desc_t const & x) {
		return (ks == x.ks) and (mod == x.mod);
	}

	key_desc_t & operator=(key_desc_t const & x) {
		keysym_name = x.keysym_name;
		ks = x.ks;
		mod = x.mod;
		return *this;
	}

	key_desc_t & operator=(string const & desc) {
		_find_key_from_string(desc);
		return *this;
	}

	key_desc_t() {
		ks = XKB_KEY_NoSymbol;
		mod = 0;
	}

	key_desc_t(string const & desc) {
		_find_key_from_string(desc);
	}

	/**
	 * Parse std::string like "mod4 f" to modifier mask (mod) and keysym (ks)
	 **/
	void _find_key_from_string(string const & desc) {

		/* no binding is set */
		ks = XKB_KEY_NoSymbol;
		mod = 0;

		if(desc == "null")
			return;

		/* find all modifier */
		std::size_t bos = 0;
		std::size_t eos = desc.find(" ", bos);
		while(eos != std::string::npos) {
			std::string modifier = desc.substr(bos, eos-bos);

			/* check for supported modifier */
			if(modifier == "shift") {
				mod |= MODIFIER_SHIFT;
			} else if (modifier == "lock") {
				mod |= 0;
			} else if (modifier == "control") {
				mod |= MODIFIER_CTRL;
			} else if (modifier == "mod1") {
				mod |= MODIFIER_ALT;
			} else if (modifier == "mod2") {
				mod |= 0;
			} else if (modifier == "mod3") {
				mod |= 0;
			} else if (modifier == "mod4") {
				mod |= MODIFIER_SUPER;
			} else if (modifier == "mod5") {
				mod |= 0;
			} else {
				throw exception_t("invalid modifier '%s' for key binding", modifier.c_str());
			}

			bos = eos+1; /* next char of char space */
			eos = desc.find(" ", bos);
		}

		keysym_name = desc.substr(bos);
		ks = xkb_keysym_from_name(keysym_name.c_str(), XKB_KEYSYM_NO_FLAGS);
		if(ks == XKB_KEY_NoSymbol) {
			throw exception_t("key binding not found");
		}
	}

};


}


#endif /* KEY_DESC_HXX_ */
