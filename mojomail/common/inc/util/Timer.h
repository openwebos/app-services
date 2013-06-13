// @@@LICENSE
//
//      Copyright (c) 2010-2013 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// LICENSE@@@

#ifndef TIMER_H_
#define TIMER_H_

#include <glib.h>

template<class T>
class Timer
{
	typedef void (T::*Callback)();

public:
	Timer() : m_callbackId(0), m_callback(NULL), m_context(NULL)
	{
	}

	virtual ~Timer()
	{
		Cancel();
	}

	/**
	 * Set a timer
	 */
	void SetTimeout(int seconds, T* context, Callback callback)
	{
		Cancel();

		m_context = context;
		m_callback = callback;

		m_callbackId = g_timeout_add_seconds(seconds, &Timer::TimeoutHandler, gpointer(this));
	}

	/**
	 * Set a timer
	 */
	void SetTimeoutMillis(unsigned int millis, T* context, Callback callback)
	{
		Cancel();

		m_context = context;
		m_callback = callback;

		m_callbackId = g_timeout_add(millis, &Timer::TimeoutHandler, gpointer(this));
	}

	void Cancel()
	{
		if(m_callbackId > 0) {
			g_source_remove(m_callbackId);
			m_callbackId = 0;
		}
	}

	bool IsActive() const
	{
		return m_callbackId;
	}

	bool IsExpired() const
	{
		return m_context && !m_callbackId;
	}

protected:
	static gboolean TimeoutHandler(gpointer data)
	{
		Timer<T>* timer = static_cast< Timer<T>* >(data);
		timer->Run();

		return false;
	}

	void Run()
	{
		m_callbackId = 0;
		(m_context->*m_callback)();
	}

	guint		m_callbackId;
	Callback	m_callback;
	T*			m_context;
};

#endif /* TIMER_H_ */
