/*
 * Lv2Worker.h - Lv2Worker class
 *
 * Copyright (c) 2022-2022 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LV2WORKER_H
#define LV2WORKER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <thread>

#include "LocklessRingBuffer.h"
#include "LmmsSemaphore.h"

namespace lmms
{

/**
	Worker container
*/
class Lv2Worker
{
public:
	// CTOR/DTOR/feature access
	Lv2Worker(LV2_Handle handle, const LV2_Worker_Interface* iface, Semaphore& common_work_lock, bool threaded);
	~Lv2Worker();
	const LV2_Worker_Schedule* feature() const { return &m_scheduleFeature; }

	// public API
	void emitResponses();
	void notifyPluginThatRunFinished()
	{
		if(m_iface->end_run) { m_iface->end_run(m_scheduleFeature.handle); }
	}

	// to be called only by static functions
	LV2_Worker_Status scheduleWork(uint32_t size, const void* data);
	LV2_Worker_Status respond(uint32_t size, const void* data);

private:
	// functions
	void* workerFunc();
	static std::size_t bufferSize();  //!< size of internal buffers

	// parameters
	const LV2_Worker_Interface* m_iface;
	bool m_threaded;
	LV2_Handle m_handle;
	LV2_Worker_Schedule m_scheduleFeature;

	// threading/synchronization
	std::thread m_thread;
	std::vector<char> m_response;
	LocklessRingBuffer<char> m_requests, m_responses;
	LocklessRingBufferReader<char> m_requestsReader, m_responsesReader;
	std::atomic<bool> m_exit = false;
	Semaphore m_sem;
	Semaphore& m_workLock;
};


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LV2WORKER_H

