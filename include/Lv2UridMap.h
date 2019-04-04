#ifndef LV2URIDMAP_H
#define LV2URIDMAP_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lv2/urid/urid.h>
#include <unordered_map>
#include <mutex> // TODO: use semaphore, even though this is not time critical
#include <vector>

class UridMap
{
	std::unordered_map<std::string, LV2_URID> m_map;
	std::vector<const char*> m_unMap;

	LV2_URID_Map m_mapFeature;
	LV2_URID_Unmap m_unmapFeature;

	LV2_URID m_lastUrid = 0;

	std::mutex m_MapMutex;
public:
	UridMap();

	LV2_URID map(const char* uri);
	const char* unmap(LV2_URID urid);

	LV2_URID_Map* mapFeature() { return &m_mapFeature; }
	LV2_URID_Unmap* unmapFeature() { return &m_unmapFeature; }
};

//LV2_URID (*map)(LV2_URID_Map_Handle handle, const char* uri);
//const char* (*unmap)(LV2_URID_Unmap_Handle handle, LV2_URID urid);

#endif // LMMS_HAVE_LV2
#endif // LV2URIDMAP_H
