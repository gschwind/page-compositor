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

	listener_t() {
		wl_list_init(&_pod._listener.link);
		_pod._func = nullptr;
	}

	~listener_t() {
		disconnect();
	}

	template<typename F>
	void connect(wl_signal* signal, F f) {
		_func = f;
		_pod._func = &_func;
		_pod._listener.notify = &listener_t::_call;
		disconnect();
		wl_signal_add(signal, &_pod._listener);
	}

	template<typename Z>
	void connect(struct wl_signal * signal, Z * ths, void(Z::*f)(T*)) {
		connect(signal, [ths,f](T * o) -> void { (ths->*f)(o); });
	}

	void disconnect() {
		if(not wl_list_empty(&_pod._listener.link)) {
			wl_list_remove(&_pod._listener.link);
			wl_list_init(&_pod._listener.link);
		}
	}

	template<typename F>
	void resource_add_destroy_listener(struct wl_resource * resource, F f) {
		_func = f;
		_pod._func = &_func;
		_pod._listener.notify = &listener_t::_call;
		disconnect();
		wl_resource_add_destroy_listener(resource, &_pod._listener);
	}

	template<typename Z>
	void resource_add_destroy_listener(struct wl_resource * resource, Z * ths, void(Z::*f)(T*)) {
		resource_add_destroy_listener(resource, [ths,f](T * o) -> void { (ths->*f)(o); });
	}

};


}


#endif /* SRC_LISTENER_HXX_ */
