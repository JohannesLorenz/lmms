/*
 * Lv2UridMap.cpp - Lv2UridMap implementation
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


#include "Lv2UridMap.h"

#ifdef LMMS_HAVE_LV2

void UridMap::CachedUrids::init(UridMap& uri_map)
{
#define EXT_ "http://lv2plug.in/ns/ext/"

	// Use string literals here instead of LV2 defines to avoid LV2 dependency
	m_atomChunk          = uri_map.map(EXT_"atom#Chunk");
	m_atomPath           = uri_map.map(EXT_"atom#Path");
	m_atomSequence       = uri_map.map(EXT_"atom#Sequence");
	m_atomEventTransfer  = uri_map.map(EXT_"atom#eventTransfer");
	m_atomURID           = uri_map.map(EXT_"atom#URID");
	m_atomBlank          = uri_map.map(EXT_"atom#Blank");
	m_atomObject         = uri_map.map(EXT_"atom#Object");
	m_atomFloat          = uri_map.map(EXT_"atom#Float");
	m_logError           = uri_map.map(EXT_"log#Error");
	m_logNote            = uri_map.map(EXT_"log#Note");
	m_logTrace           = uri_map.map(EXT_"log#Trace");
	m_logWarning         = uri_map.map(EXT_"log#Warning");
	m_midiMidiEvent      = uri_map.map(EXT_"midi#MidiEvent");
	m_timePosition       = uri_map.map(EXT_"time#Position");
	m_timeBar            = uri_map.map(EXT_"time#bar");
	m_timeBarBeat        = uri_map.map(EXT_"time#barBeat");
	m_timeBeatUnit       = uri_map.map(EXT_"time#beatUnit");
	m_timeBeatsPerBar    = uri_map.map(EXT_"time#beatsPerBar");
	m_timeBeatsPerMinute = uri_map.map(EXT_"time#beatsPerMinute");
	m_timeFrame          = uri_map.map(EXT_"time#frame");
	m_timeSpeed          = uri_map.map(EXT_"time#speed");
	m_patchGet           = uri_map.map(EXT_"patch#Get");
	m_patchSet           = uri_map.map(EXT_"patch#Set");
	m_patchProperty      = uri_map.map(EXT_"patch#property");
	m_patchValue         = uri_map.map(EXT_"patch#value");
	m_stateStateChanged  = uri_map.map(EXT_"state#StateChanged"); // since LV2 1.15.1
#undef EXT_
}

static LV2_URID staticMap(LV2_URID_Map_Handle handle, const char* uri)
{
	UridMap* map = static_cast<UridMap*>(handle);
	return map->map(uri);
}

static const char* staticUnmap(LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
	UridMap* map = static_cast<UridMap*>(handle);
	return map->unmap(urid);
}

UridMap::UridMap()
{
	m_mapFeature.handle = static_cast<LV2_URID_Map_Handle>(this);
	m_mapFeature.map = staticMap;
	m_unmapFeature.handle = static_cast<LV2_URID_Unmap_Handle>(this);
	m_unmapFeature.unmap = staticUnmap;

	m_cachedUrids.init(*this);
}

LV2_URID UridMap::map(const char *uri)
{
	LV2_URID result = 0u;
	auto itr = m_map.find(uri);

	if (itr == m_map.end())
	{
		try
		{
			std::string uriStr = uri;

			// the URID map is global, so mutex it
			m_MapMutex.lock();
			{
				// 1 is the first free URID
				std::size_t index = 1u + m_unMap.size();
				auto pr = m_map.emplace(std::move(uriStr), index);
				if (pr.second)
				{
					m_unMap.emplace_back(pr.first->first.c_str());
					result = static_cast<LV2_URID>(index);
				}
			}
			m_MapMutex.unlock();

		}
		catch(...) { /* result variable is already 0 */ }
	}
	else
	{
		result = itr->second;
	}

	return result;
}

const char *UridMap::unmap(LV2_URID urid)
{
	std::size_t idx = static_cast<std::size_t>(urid) - 1;
	return (idx < m_unMap.size()) ? m_unMap[idx] : nullptr;
}

#endif // LMMS_HAVE_LV2

