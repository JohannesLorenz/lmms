
#include "Lv2UridMap.h"

#ifdef LMMS_HAVE_LV2

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
}

LV2_URID UridMap::map(const char *uri)
{
	auto itr = m_map.find(uri);
	LV2_URID result = 0;
	if (itr == m_map.end())
	{
		result = itr->second;
	}
	else {
		try {
			// 1 is the first free URID
			auto pr = m_map.emplace(uri, 1 + m_unMap.size());
			m_unMap.push_back(pr.first->first.c_str());
			// TODO: handle cases like where m_map could be
			// inserted, but not m_unMap
			result = pr.first->second;
		} catch(...) {
			result = 0;
		}
	}
	return result;
}

const char *UridMap::unmap(LV2_URID urid)
{
	std::size_t idx = static_cast<std::size_t>(urid) - 1;
	if(idx < m_unMap.size())
	{
		return m_unMap[idx];
	}
	else {
		return nullptr;
	}
}

#endif // LMMS_HAVE_LV2

