/*
 * Lv2Worker.cpp - Lv2Worker implementation
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

#include <cassert>
#include <QDebug>

#include "Lv2Worker.h"

#ifdef LMMS_HAVE_LV2

#include "Engine.h"


namespace lmms
{


// static wrappers (TODO: avoid those somehow?)

static LV2_Worker_Status staticScheduleWork(LV2_Worker_Schedule_Handle handle,
	uint32_t size, const void* data)
{
	Lv2Worker* worker = static_cast<Lv2Worker*>(handle);
	return worker->scheduleWork(size, data);
}




static LV2_Worker_Status
staticWorkerRespond(LV2_Worker_Respond_Handle handle,
	uint32_t size, const void* data)
{
	Lv2Worker* worker = static_cast<Lv2Worker*>(handle);
	return worker->respond(size, data);
}




std::size_t Lv2Worker::bufferSize()
{
	// ardour uses this fixed size for ALSA:
	return 8192 * 4;
	// for jack, they use 4 * jack_port_type_get_buffer_size (..., JACK_DEFAULT_MIDI_TYPE)
	// (possible extension for AudioDevice)
}




Lv2Worker::Lv2Worker(LV2_Handle handle,
	const LV2_Worker_Interface* iface,
	Semaphore& common_work_lock,
	bool threaded) :
	iface(iface),
	threaded(threaded),
	handle(handle),
	response(bufferSize()),
	requests(bufferSize()),
	responses(bufferSize()),
	requestsReader(requests),
	responsesReader(responses),
	sem(0),
	work_lock(common_work_lock)
{
	assert(iface);
	m_scheduleFeature.handle = static_cast<LV2_Worker_Schedule_Handle>(this);
	m_scheduleFeature.schedule_work = staticScheduleWork;

	if (threaded) { thread = std::thread(&Lv2Worker::workerFunc, this); }

	requests.mlock();
	responses.mlock();
}




Lv2Worker::~Lv2Worker()
{
	exit = true;
	if(threaded) {
		sem.post();
		thread.join();
	}
}




LV2_Worker_Status Lv2Worker::respond(uint32_t size, const void* data)
{
	if(!size) { return LV2_WORKER_ERR_UNKNOWN; }

	responses.write((const char*)&size, sizeof(size));
	responses.write((const char*)data, size);
	return LV2_WORKER_SUCCESS;
}




void *Lv2Worker::workerFunc()
{
	std::vector<char> buf;
	uint32_t size;
	while (true) {
		sem.wait();
		if (exit) break;
		if (requestsReader.read_space() <= sizeof(size)) continue; // (should not happen)

		requestsReader.read(sizeof(size)).copy((char*)&size, sizeof(size));
		try {
			buf.resize(size);
		} catch(...) {
			qWarning() << "Error: reallocating buffer failed";
			return nullptr;
		}
		requestsReader.read(sizeof(size)).copy(buf.data(), size);

		work_lock.wait();
		iface->work(handle, staticWorkerRespond, this, size, buf.data());
		work_lock.post();
	}
	return nullptr;
}




LV2_Worker_Status Lv2Worker::scheduleWork(uint32_t size, const void *data)
{
	if (!size) { return LV2_WORKER_ERR_UNKNOWN; }

	if (threaded) {
		// Schedule a request to be executed by the worker thread
		requests.write((const char*)&size, sizeof(size));
		requests.write((const char*)data, size);
		sem.post();
	} else {
		// Execute work immediately in this thread
		work_lock.wait();
		iface->work(handle, staticWorkerRespond, this, size, data);
		work_lock.post();
	}

	return LV2_WORKER_SUCCESS;
}




void Lv2Worker::emitResponses()
{
	if (!exit) {
		std::size_t read_space = responsesReader.read_space();
		uint32_t size;
		while (read_space > sizeof(size)) {
			responsesReader.read(sizeof(size)).copy((char*)&size, sizeof(size));
			responsesReader.read(size).copy(response.data(), size);
			iface->work_response(handle, size, response.data());
			read_space -= sizeof(size) + size;
		}
	}
}


} // namespace lmms

#endif // LMMS_HAVE_LV2
