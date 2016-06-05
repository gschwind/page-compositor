/*
 * listener.hxx
 *
 *  Created on: 5 juin 2016
 *      Author: gschwind
 */

#ifndef SRC_LISTENER_HXX_
#define SRC_LISTENER_HXX_

#include <wayland-util.h>
#include <libweston-0/compositor.h>
#include <functional>


/* crazy isn't it ? */
template<typename T>
struct listener_t {
	struct _pod_t {
		wl_listener _listener;
		std::function<void(T*)> * _func;
	} _pod;
	std::function<void(T*)> _func;

	template<typename F>
	void connect(wl_signal * sig, F f);

};

template<typename T>
static void _listener_call(wl_listener* l, void* _data) {
	typename listener_t<T>::_pod_t* ths = wl_container_of(l, ths, _listener);
	(*ths->_func)(reinterpret_cast<T*>(_data));
}


template<typename T> template<typename F>
void listener_t<T>::connect(wl_signal * sig, F f) {
	_pod._func = &_func;
	_pod._listener.notify = &_listener_call<T>;
	_func = std::function<void(T*)>(f);
	wl_signal_add(sig, &_pod._listener);
}

#endif /* SRC_LISTENER_HXX_ */
