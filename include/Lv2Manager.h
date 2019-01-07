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
	Plugin::PluginTypes computePluginType(Lilv::Plugin *plugin);

	Lilv::World m_world;

public:
	Lilv::Node newUri(const char* uri) {
		return m_world.new_uri(uri);
	}

	struct Lv2Info
	{
		/*const*/ QString m_uri; // TODO: remove?
		//LV2_Descriptor *m_descriptor;
		Lilv::Plugin m_plugin;
		Plugin::PluginTypes m_type;
		Lv2Info(const Lv2Info &) = delete;
		Lv2Info(Lv2Info&& other) :
			m_uri(std::move(other.m_uri)),
			m_plugin(other.m_plugin.me),
			m_type(std::move(other.m_type))
		{
		}
		Lv2Info& operator=(Lv2Info&& other)
		{
			m_uri = std::move(other.m_uri);
			m_plugin = other.m_plugin.me;
			m_type = std::move(other.m_type);
			return *this;
		}
		//! use only for std::map internals
		Lv2Info() : m_plugin(nullptr) {}
		Lv2Info(Lilv::Plugin& plug) : m_plugin(plug.me) {}
		void cleanup();
	};

	Lv2Manager();
	~Lv2Manager();

	//! returns a descriptor with @p uniqueName or nullptr if none exists
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
};

#endif // LMMS_HAVE_LV2

#endif // LV2MANAGER_H
