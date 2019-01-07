/*
 * Lv2ControlBase.h - implementation of LV2 interface
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

#ifndef LV2_CONTROL_BASE_H
#define LV2_CONTROL_BASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <QMutex>
#include <QString>
#include <map>
#include <memory>
#include <vector>

// general LMMS includes
#include "DataFile.h"
#include "Plugin.h"

// includes from the lv2 library
#include <lv2.h>
#include <lilv/lilvmm.hpp>


// TODO: clean up protected/private
class Lv2ControlBase
{
public:
	enum class PortFlow {
		Unknown,
		Input,
		Output
	};

	enum class PortType {
		Unknown,
		Control,
		Audio,
		Event, //!< TODO: unused, describe
		Cv //!< TODO: unused, describe
	};

	//! All Lv2 audio ports are floats, this is only the visualisation
	enum class PortVis {
		None,
		Integer,
		Enumeration,
		Toggled
	};

	struct Port
	{
		Lilv::Port m_port = Lilv::Port(nullptr, nullptr);
		PortType m_type = PortType::Unknown;
		PortFlow m_flow = PortFlow::Unknown;
		PortVis m_vis = PortVis::None;

		union Data
		{
			struct controlData
			{
				//! Data location which Lv2 plugins see
				//! Model values are being copied here every run
				//! Between runs, this data is not up-to-date
				float m_val;
				//! LMMS models
				//! Always up-to-date, except during runs
				union
				{
					class FloatModel *m_floatModel;
					class IntModel *m_intModel;
					class BoolModel *m_boolModel;
				} m_connectedModel;
				controlData() = default;
				controlData(const controlData &) = delete;

				AutomatableModel *getModel(
					Lv2ControlBase::PortVis m_type);
			} m_controlData;
			struct audioData
			{
				std::vector<float> buffer;
				audioData() = default;
			} m_audioData;
		} m_data;
	};
public:
	enum IssueType
	{
		unknownPortFlow,
		unknownPortType,
		tooManyInputChannels,
		tooManyOutputChannels,
		noOutputChannel,
		noMonoSupport, //!< mono is not yet supported
		effectWithoutInput,
		none
	};

	struct Issue
	{
		IssueType m_issueType;
		std::string m_info;
		Issue(IssueType it, const char* msg)
			: m_issueType(it), m_info(msg)
		{

		}
	};
private:
	std::vector<Port> m_ports;
	std::vector<Issue> m_issues;
public:

	void issue(IssueType it, const char* msg = "") {
		if(it != none)
		m_issues.emplace_back(it, msg);
	}

	struct StereoPortRef
	{
		//! mono port or left port in case of stereo
		Port* left = nullptr;
		//! unused, or right port in case of stereo
		Port* right = nullptr;
	};

	StereoPortRef inPorts, outPorts;
	std::size_t m_controlCount = 0;

	Lv2ControlBase(const QString &uniqueName);
	virtual ~Lv2ControlBase();

	void saveSettings(QDomDocument &doc, QDomElement &that);
	void loadSettings(const QDomElement &that);

	void loadFile(const QString &file);

	std::vector<Port>& getPorts() { return m_ports; }
	const std::vector<Issue>& getIssues() const { return m_issues; }

	Lilv::Plugin m_plugin;
	Lilv::Instance* m_instance;
//	LV2_Handle *m_plugin = nullptr;

	//! Keep all LMMS models in a dictionary
	std::map<std::string, AutomatableModel *> m_connectedModels;
	uint64_t m_loadTicket = 0, m_saveTicket = 0, m_restoreTicket = 0;

protected:
	void reloadPlugin();

	class AutomatableModel *modelAtPort(const QString &dest);

private:
	virtual DataFile::Types settingsType() = 0;
	virtual void setNameFromFile(const QString &fname) = 0;

protected:
	void copyModelsToControlPorts();

private:
	friend struct LmmsVisitor;
	friend struct TypeChecker;

protected:
	QMutex m_pluginMutex;

	bool initPlugin();
	void shutdownPlugin();

	bool m_hasGUI;
	bool m_loaded;

	QString nodeName() const { return "lv2controls"; }

private:
	//! load a file in the plugin, but don't do anything in LMMS
	void loadFileInternal(const QString &file);
	//! allocate m_ports, fill all with metadata, and assign meaning of
	//! ports
	void createPorts();
	//! fill m_ports[portNum] with metadata
	void createPort(unsigned portNum);
	//! connect m_ports[portNum] with Lv2
	void connectPort(unsigned num);
};

#endif // LMMS_HAVE_LV2

#endif // LV2_CONTROL_BASE_H
