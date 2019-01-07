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

Lv2Manager::Lv2Manager()
{
//	world = lilv_world_new();
//	lilv_world_load_all(world);
//	plugins = lilv_world_get_all_plugins(world);
	m_world.load_all();
	Lilv::Plugins plugins = m_world.get_all_plugins();

	for(LilvIter* itr = plugins.begin(); !plugins.is_end(itr);
		itr = plugins.next(itr))
	{
		Lilv::Plugin curPlug = plugins.get(itr);
		Lilv::PluginClass curClass = curPlug.get_class();
		qDebug() << "Class URI:" << curClass.get_uri().as_uri();

		Lv2Info info(curPlug);

		info.m_uri = curPlug.get_uri().as_uri(); // TODO: duplicate to key?

		info.m_type = Plugin::Effect; // TODO!!!

		m_lv2InfoMap[curPlug.get_uri().as_uri()] = std::move(info);
	}


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

Plugin::PluginTypes Lv2Manager::computePluginType(Lilv::Plugin *plugin)
{
/*	plugin->get_class().
	if()
	{

	}*/
	return Plugin::PluginTypes::Effect;
#if 0
	LV2_Handle *plug = desc->instantiate();

	struct TypeChecker final : public virtual lv2::audio::visitor
	{
		std::size_t m_inCount = 0, m_outCount = 0;
		void visit(lv2::audio::in &) override { ++m_inCount; }
		void visit(lv2::audio::out &) override { ++m_outCount; }
		void visit(lv2::audio::stereo::in &) override { ++++m_inCount; }
		void visit(lv2::audio::stereo::out &) override
		{
			++++m_outCount;
		}
	} tyc;

	for (const lv2::simple_str &portname : desc->port_names())
	{
		try
		{
			plug->port(portname.data()).accept(tyc);
		}
		catch (lv2::port_not_found &)
		{
			return Plugin::PluginTypes::Undefined;
		}
	}

	delete plug;

	Plugin::PluginTypes res;
	if (tyc.m_inCount > 2 || tyc.m_outCount > 2)
	{
		res = Plugin::PluginTypes::Undefined;
	} // TODO: enable mono effects?
	else if (tyc.m_inCount == 2 && tyc.m_outCount == 2)
	{
		res = Plugin::PluginTypes::Effect;
	}
	else if (tyc.m_inCount == 0 && tyc.m_outCount == 2)
	{
		res = Plugin::PluginTypes::Instrument;
	}
	else
	{
		res = Plugin::PluginTypes::Other;
	}

	qDebug() << "Plugin type of " << lv2::unique_name(*desc).c_str() << ":";
	qDebug() << (res == Plugin::PluginTypes::Undefined
			? "  undefined"
			: res == Plugin::PluginTypes::Effect
				? "  effect"
				: res == Plugin::PluginTypes::Instrument
					? "  instrument"
					: "  other");

	return res;
#endif
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
