/*
 * listener.hxx
 *
 *  Created on: 10 juin 2016
 *      Author: gschwind
 */

#ifndef SRC_LISTENER_HXX_
#define SRC_LISTENER_HXX_

#include <functional>

namespace page {

template<typename T>
struct listener_t {
	struct listener_pod_t {
		wl_listener _listener;
		std::function<void(T*)>* _func;
	} _pod;
	std::function<void(T*)> _func;

	static void _call(wl_listener* _listener, void* data) {
		listener_pod_t* listener = wl_container_of(_listener, listener, _listener);
		(*listener->_func)(reinterpret_cast<T*>(data));
	}

	template<typename F>
	void connect(wl_signal* signal, F f) {
		_func = f;
		_pod._func = &_func;
		_pod._listener.notify = &listener_t::_call;
		wl_signal_add(signal, &_pod._listener);
	}

};


}


#endif /* SRC_LISTENER_HXX_ */
