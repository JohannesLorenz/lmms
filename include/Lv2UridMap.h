/*
 * Lv2UridMap.cpp - Lv2UridMap class
 *
 * Copyright (c) 2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef LV2URIDMAP_H
#define LV2URIDMAP_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lv2/urid/urid.h>
#include <unordered_map>
#include <mutex> // TODO: use semaphore, even though this is not realtime critical
#include <vector>

/**
 * Complete implementation of the Lv2 Urid Map extension
 */
class UridMap
{
public:
	//! Cached URIDs for use in real-time code
	struct CachedUrids
	{
		void init(UridMap& uri_map);

		uint32_t m_atomChunk;
		uint32_t m_atomPath;
		uint32_t m_atomSequence;
		uint32_t m_atomEventTransfer;
		uint32_t m_atomURID;
		uint32_t m_atomBlank;
		uint32_t m_atomObject;
		uint32_t m_atomFloat;
		uint32_t m_logError;
		uint32_t m_logNote;
		uint32_t m_logTrace;
		uint32_t m_logWarning;
		uint32_t m_midiMidiEvent;
		uint32_t m_timePosition;
		uint32_t m_timeBar;
		uint32_t m_timeBarBeat;
		uint32_t m_timeBeatUnit;
		uint32_t m_timeBeatsPerBar;
		uint32_t m_timeBeatsPerMinute;
		uint32_t m_timeFrame;
		uint32_t m_timeSpeed;
		uint32_t m_patchGet;
		uint32_t m_patchSet;
		uint32_t m_patchProperty;
		uint32_t m_patchValue;
		uint32_t m_stateStateChanged;
	};

private:
	std::unordered_map<std::string, LV2_URID> m_map;
	std::vector<const char*> m_unMap;

	LV2_URID_Map m_mapFeature;
	LV2_URID_Unmap m_unmapFeature;

	LV2_URID m_lastUrid = 0;

	std::mutex m_MapMutex;

	CachedUrids m_cachedUrids;

public:
	//! constructor; will set up the features
	UridMap();

	//! map feature function
	LV2_URID map(const char* uri);
	//! unmap feature function
	const char* unmap(LV2_URID urid);

	const CachedUrids& cache() const { return m_cachedUrids; }

	// access the features
	LV2_URID_Map* mapFeature() { return &m_mapFeature; }
	LV2_URID_Unmap* unmapFeature() { return &m_unmapFeature; }
};

#endif // LMMS_HAVE_LV2
#endif // LV2URIDMAP_H
