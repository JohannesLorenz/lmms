/*
 * Lv2Manager.cpp - Implementation of Lv2Manager class
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

#include "Lv2Manager.h"

#ifdef LMMS_HAVE_LV2

#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <lv2.h>

#include <lilv/lilv.h>

#include "ConfigManager.h"
#include "Plugin.h"
#include "PluginFactory.h"
#include "Lv2ControlBase.h"

void Lv2Manager::initPlugins()
{
	Lilv::Plugins plugins = m_world.get_all_plugins();

	for(LilvIter* itr = plugins.begin(); !plugins.is_end(itr);
		itr = plugins.next(itr))
	{
		Lilv::Plugin curPlug = plugins.get(itr);

		Lv2Info info(curPlug);
		std::vector<Lv2Issue> issues;
		info.m_type = Lv2ControlBase::check(curPlug, issues, true);
		info.m_valid = issues.empty();

		m_lv2InfoMap[curPlug.get_uri().as_uri()] = std::move(info);
	}
}

Lv2Manager::Lv2Manager()
{
	m_world.load_all();


#if 0
	auto addSinglePlugin = [this](const QString &absolutePath) {
		Lv2Info info;
		info.m_lib = new QLibrary(absolutePath);
		LV2_Descriptor_loader_t lv2DescriptorLoader;

		if (info.m_lib->load())
		{
			lv2DescriptorLoader =
				reinterpret_cast<LV2_Descriptor_loader_t>(
					info.m_lib->resolve(
						LV2_Descriptor_name));

			if (lv2DescriptorLoader)
			{
				info.m_descriptor = (*lv2DescriptorLoader)(
					0 /* = plugin number, TODO */);
				if (info.m_descriptor)
				{
					info.m_type = computePluginType(
						info.m_descriptor);
					if (info.m_type !=
						Plugin::PluginTypes::Undefined)
					{
						qDebug()
							<< "Lv2Manager: Adding "
							<< lv2::unique_name(
								*info.m_descriptor)
								.c_str();
						info.m_path = absolutePath;
						m_lv2InfoMap[lv2::unique_name(
							*info.m_descriptor)] =
							std::move(info);
					}
				}
			}
			else
			{
				qWarning() << info.m_lib->errorString();
			}
		}
		else
		{
			qWarning() << info.m_lib->errorString();
		}
	};
#endif
}

Lv2Manager::~Lv2Manager()
{
	for (std::pair<const std::string, Lv2Info> &pr : m_lv2InfoMap)
	{
		pr.second.cleanup();
	}
}

// unused + untested yet
bool Lv2Manager::isSubclassOf(Lilv::PluginClass& clss, const char* uriStr)
{
	Lilv::PluginClasses allClasses = m_world.get_plugin_classes();
	Lilv::PluginClass root = m_world.get_plugin_class();
	Lilv::PluginClass gen = allClasses.get_by_uri(uri(uriStr));

	// lv2:Generator is what can be generating an LMMS instrument track
	// lv2:Instrument is lv:Generator with MIDI/piano input
	// => LMMS "Instrument" corresponds to lv2:Generator
	auto clssEq = [](Lilv::PluginClass& pc1, Lilv::PluginClass& pc2) -> bool
	{
		return lilv_node_equals(pc1.get_uri().me, pc2.get_uri().me);
	};
	bool isGen = false;
	for (;
		clssEq(clss, root) && (isGen = clssEq(clss, gen));
		clss = allClasses.get_by_uri(clss.get_parent_uri())
	) ;
	return isGen;
}

Lilv::Plugin *Lv2Manager::getPlugin(const std::string &uri)
{
	auto itr = m_lv2InfoMap.find(uri);
	return itr == m_lv2InfoMap.end() ? nullptr : &itr->second.m_plugin;
}

Lilv::Plugin *Lv2Manager::getPlugin(const QString uri)
{
	return getPlugin(std::string(uri.toUtf8()));
}

void Lv2Manager::Lv2Info::cleanup()
{
//	m_descriptor->delete_self();
//	delete m_lib;
}

#endif // LMMS_HAVE_LV2
