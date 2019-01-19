/*
 * Lv2Manager.h - Implementation of Lv2Manager class
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef LV2MANAGER_H
#define LV2MANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <map>
#include <lv2.h>
#include <lilv/lilvmm.hpp>

#include "Plugin.h"

//! Class to keep track of all LV2 plugins
class Lv2Manager
{
	Lilv::World m_world;

public:
	Lilv::Node uri(const char* uriStr)
	{
		// TODO: fix this when new lilvmm is released
		LilvNode* node = m_world.new_uri(uriStr);
		Lilv::Node deepCopy(node);
		return node;
	}

	struct Lv2Info
	{
		Lilv::Plugin m_plugin;
		Plugin::PluginTypes m_type;
		Lv2Info(const Lv2Info &) = delete;
		Lv2Info(Lv2Info&& other) :
			m_plugin(other.m_plugin.me),
			m_type(std::move(other.m_type)),
			m_valid(std::move(other.m_valid))
		{
		}
		Lv2Info& operator=(Lv2Info&& other)
		{
			m_plugin = other.m_plugin.me;
			m_type = std::move(other.m_type);
			m_valid = std::move(other.m_valid);
			return *this;
		}
		//! use only for std::map internals
		Lv2Info() : m_plugin(nullptr) {}
		Lv2Info(Lilv::Plugin& plug) : m_plugin(plug.me) {}
		void cleanup();
		bool m_valid = false;
	};

	void initPlugins();

	Lv2Manager();
	~Lv2Manager();

	//! Return a descriptor with @p uniqueName or nullptr if none exists
	//! @param uniqueName The lv2::unique_name of the plugin
	Lilv::Plugin *getPlugin(const std::string &uri);
	Lilv::Plugin *getPlugin(const QString uri);

	struct Iterator
	{
		std::map<std::string, Lv2Info>::iterator itr;
		bool operator!=(const Iterator &other)
		{
			return itr != other.itr;
		}
		Iterator &operator++()
		{
			++itr;
			return *this;
		}
		std::pair<const std::string, Lv2Info> &operator*()
		{
			return *itr;
		}
	};

	Iterator begin() { return {m_lv2InfoMap.begin()}; }
	Iterator end() { return {m_lv2InfoMap.end()}; }

private:
	std::map<std::string, Lv2Info> m_lv2InfoMap;
	bool isSubclassOf(Lilv::PluginClass &clss, const char *uriStr);
};

#endif // LMMS_HAVE_LV2

#endif // LV2MANAGER_H
