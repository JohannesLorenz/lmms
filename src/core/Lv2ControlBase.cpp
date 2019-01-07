/*
 * Lv2ControlBase.cpp - base class for lv2 instruments, effects, etc
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

#include "Lv2ControlBase.h"

#ifdef LMMS_HAVE_LV2

#include <QDebug>
#include <QDir>
#include <QTemporaryFile>
#include <QLibrary>
#include <cassert>
#include <lv2.h>

#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "Mixer.h"
#include "Lv2Manager.h"
#include "embed.h"
#include "gui_templates.h"

Lv2ControlBase::Lv2ControlBase(const QString& uri) :
	m_plugin(Engine::getLv2Manager()->getPlugin(uri)->me),
	m_hasGUI(false)
{
	if(!m_plugin)
	{
		qDebug() << ":-( ! No descriptor found for" << uri;
	}

	initPlugin();
}

Lv2ControlBase::~Lv2ControlBase() { shutdownPlugin(); }

void Lv2ControlBase::saveSettings(QDomDocument &doc, QDomElement &that)
{
#ifdef TODO
	// save internal data?
	if (m_lv2Plugin->save_has())
	{
		QTemporaryFile tf;
		if (tf.open())
		{
			const std::string fn = QSTR_TO_STDSTR(
				QDir::toNativeSeparators(tf.fileName()));
			m_pluginMutex.lock();
			m_plugin->save(fn.c_str(), ++m_saveTicket);
			m_pluginMutex.unlock();

			while (!m_plugin->save_check(fn.c_str(), m_saveTicket)) {
				QThread::msleep(1);
			}

			QDomCDATASection cdata = doc.createCDATASection(
				QString::fromUtf8(tf.readAll()));
			that.appendChild(cdata);
		}
		tf.remove();
	}

	// save connected models
	if (m_connectedModels.size())
	{
		QDomElement newNode = doc.createElement("connected-models");
		QMap<QString, AutomatableModel *>::const_iterator i =
			m_connectedModels.constBegin();
		while (i != m_connectedModels.constEnd())
		{
			i.value()->saveSettings(doc, newNode, i.key());
			++i;
		}

		that.appendChild(newNode);
	}
#else
	(void)doc;
	(void)that;
#endif
}

void Lv2ControlBase::loadSettings(const QDomElement &that)
{
#ifdef TODO
	if (!that.hasChildNodes())
	{
		return;
	}

	for (QDomNode node = that.firstChild(); !node.isNull();
		node = node.nextSibling())
	{
		QDomCDATASection cdata = node.toCDATASection();
		QDomElement elem;
		// load internal state?
		if (!cdata.isNull() && m_lv2Plugin->load_has())
		{
			QTemporaryFile tf;
			tf.setAutoRemove(false);
			if (tf.open())
			{
				tf.write(cdata.data().toUtf8());
				tf.flush();
				loadFileInternal(tf.fileName());
			}
		}
		// load connected models?
		else if (node.nodeName() == "connected-models" &&
			!(elem = node.toElement()).isNull())
		{
			QDomNamedNodeMap attrs = elem.attributes();

			auto do_load = [&](const QString &name,
					       QDomElement elem) {
				AutomatableModel *m = modelAtPort(name);
				// this will automatically
				// load any "connection" node:
				m->loadSettings(elem, name);
				m_connectedModels[name] = m;
			};

			for (int i = 0; i < attrs.count(); ++i)
			{
				QDomAttr attribute = attrs.item(i).toAttr();
				do_load(attribute.name(), elem);
			}

			for (QDomElement portnode = elem.firstChildElement();
				!portnode.isNull();
				portnode = portnode.nextSiblingElement())
			{
				if (portnode.nodeName() != "connection")
				{
					QString name = portnode.nodeName();
					if (name == "automatablemodel") {
						name = portnode.attribute(
							"nodename");
					}
					do_load(name, elem);
				}
			}
		}
	}
#else
	(void)that;
#endif
}

void Lv2ControlBase::loadFileInternal(const QString &file)
{
#ifdef TODO
	const QByteArray fn = file.toUtf8();
	m_pluginMutex.lock();
	m_plugin->load(fn.data(), ++m_saveTicket);
	while (!m_plugin->load_check(fn.data(), m_saveTicket)) {
		QThread::msleep(1);
	}
	m_pluginMutex.unlock();
#else
	(void)file;
#endif
}

void Lv2ControlBase::loadFile(const QString &file)
{
#ifdef TODO
	loadFileInternal(file);
	setNameFromFile(QFileInfo(file).baseName().replace(
		QRegExp("^[0-9]{4}-"), QString()));
#else
	(void)file;
#endif
}

void Lv2ControlBase::reloadPlugin()
{
#ifdef TODO
	// refresh ports that are only read on restore
	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	int16_t fpp = Engine::mixer()->framesPerPeriod();
	assert(fpp >= 0);
	m_ports.buffersize = static_cast<unsigned>(fpp);

	if (m_lv2Plugin->restore_has())
	{
		// use the offered restore function
		m_pluginMutex.lock();
		m_plugin->restore(++m_restoreTicket);
		m_pluginMutex.unlock();

		while (!m_plugin->restore_check(m_restoreTicket)) {
			QThread::msleep(1);
}
	}
	else
	{
		// save state of current plugin instance
		DataFile m(settingsType());

		saveSettings(m, m.content());

		shutdownPlugin();
		// init plugin (will create a new instance)
		initPlugin();

		// and load the settings again
		loadSettings(m.content());
	}
#endif
}

void Lv2ControlBase::copyModelsToControlPorts()
{
	for (Port &port : m_ports)
	{
		if(port.m_type == PortType::Control)
		{
			auto& ctrl = port.m_data.m_controlData;
			auto& mdl = ctrl.m_connectedModel;
			switch (port.m_vis)
			{
				case PortVis::None:
					ctrl.m_val = mdl.m_floatModel->value();
					break;
				case PortVis::Integer:
				case PortVis::Enumeration:
					ctrl.m_val = mdl.m_intModel->value();
					break;
				case PortVis::Toggled:
					ctrl.m_val = mdl.m_boolModel->value();
					break;
			}
		}
		// Audio buffers don't require copying (hopefully)
	}
}

void Lv2ControlBase::shutdownPlugin()
{
	m_instance->deactivate();
	m_instance = nullptr;
}




void Lv2ControlBase::createPort(unsigned portNum)
{
	Lv2Manager* man = Engine::getLv2Manager();
	Lilv::Node InputPort = man->newUri(LV2_CORE__InputPort);
	Lilv::Node OutputPort = man->newUri(LV2_CORE__OutputPort);
	Lilv::Node AudioPort = man->newUri(LV2_CORE__AudioPort);
	Lilv::Node ControlPort = man->newUri(LV2_CORE__ControlPort);
	Lilv::Node connectionOptional = man->newUri(LV2_CORE__connectionOptional);

	Port& curPort = m_ports[portNum];
	curPort.m_port = m_plugin.get_port_by_index(portNum);

	const bool optional = curPort.m_port.has_property(connectionOptional);

	if (curPort.m_port.is_a(InputPort)) {
		curPort.m_flow = PortFlow::Input;
	}
	else if (curPort.m_port.is_a(OutputPort)) {
		curPort.m_flow = PortFlow::Output;
	} else if (!optional) {
		issue(IssueType::unknownPortType);
	}

	if (curPort.m_port.is_a(ControlPort)) {
		curPort.m_type = PortType::Control;
	}
	else if (curPort.m_port.is_a(AudioPort)) {
		curPort.m_type = PortType::Audio;
	} else if (!optional) {
		issue(IssueType::unknownPortFlow);
	}

	// lilvmm does not support ranges yet, so use lilv C lib
	LilvNode * defC, * minC, * maxC;
	lilv_port_get_range(m_plugin.me, curPort.m_port.me, &defC, &minC, &maxC);
	Lilv::Node def(defC), min(minC), max(maxC);

	Lilv::Node propInt = man->newUri(LV2_CORE__integer);
	Lilv::Node propToggled = man->newUri(LV2_CORE__toggled);
	Lilv::Node propEnumeration = man->newUri(LV2_CORE__enumeration);

	curPort.m_vis =
		curPort.m_port.has_property(propInt)
		? PortVis::Integer
		: curPort.m_port.has_property(propEnumeration)
		? PortVis::Enumeration
		: curPort.m_port.has_property(propToggled)
		? PortVis::Toggled
		: PortVis::None;

	// add the port's model to the dictionary
	m_connectedModels.emplace(lilv_node_as_string(curPort.m_port.get_symbol()),
		curPort.m_data.m_controlData.getModel(curPort.m_vis));
}

void Lv2ControlBase::createPorts()
{
	unsigned maxPorts = m_plugin.get_num_ports();
	m_ports.resize(maxPorts);

	for(unsigned portNum = 0; portNum < maxPorts; ++portNum)
	{
		createPort(portNum);
		Port& curPort = m_ports[portNum];
		if(curPort.m_type == PortType::Audio)
		{
			StereoPortRef* portRef;
			StereoPortRef dummy;
			IssueType it;
			switch(curPort.m_flow)
			{
				case PortFlow::Input:
					portRef = &inPorts;
					it = tooManyInputChannels;
					break;
				case PortFlow::Output:
					portRef = &outPorts;
					it = tooManyOutputChannels;
					break;
				case PortFlow::Unknown:
					portRef = &dummy;
					it = none;
					break;
			}
			// in Lv2, leftPort is defined to be the first port
			if(!portRef->left)
				portRef->left = &curPort;
			else if(!portRef->right)
				portRef->right = &curPort;
			else
				issue(it);
			break;
		}
		else if(curPort.m_type == PortType::Control)
		{
			++m_controlCount;
		}
	}

	if(!outPorts.left)
		issue(noOutputChannel);
	if(!outPorts.right)
		issue(noMonoSupport);
}

void Lv2ControlBase::connectPort(unsigned num)
{
	auto con = [num, this](void* location) {
		m_instance->connect_port(num, location);
	};
	Port& port = m_ports[num];
	switch(port.m_type)
	{
		case PortType::Audio:
			con(&port.m_data.m_audioData.buffer);
			break;
		case PortType::Control:
			con(&port.m_data.m_controlData.m_val);
			break;
		default:
			con(nullptr);
			break;
	}
}

bool Lv2ControlBase::initPlugin()
{
	m_pluginMutex.lock();
	if (!m_plugin)
	{
		m_pluginMutex.unlock();
		return false;
	}
	else
	{
		m_instance = Lilv::Instance::create(m_plugin,
			Engine::mixer()->processingSampleRate(),
			nullptr);

		if(!m_instance)
		{
			qCritical() << "Failed to create an instance of"
				<< m_plugin.get_name().as_string();
		}
		else
		{
			createPorts();
		}
		m_pluginMutex.unlock();

		for(unsigned portNum = 0; portNum < m_ports.size(); ++portNum)
			connectPort(portNum);

		m_instance->activate();
	}

	return true;
}




AutomatableModel *Lv2ControlBase::modelAtPort(const QString &dest)
{
	// unused currently
	AutomatableModel *mod;
	auto itr = m_connectedModels.find(dest.toUtf8().data());
	if (itr != m_connectedModels.end())
	{
		mod = itr->second;
	}
	else
	{
#if 0
		AutomatableModel *lv2Mod;
		{
			Lv2OscModelFactory vis(this, url.path());
			lv2::port_ref_base &base =
				m_plugin->port(url.path().toUtf8().data());
			base.accept(vis);
			lv2Mod = vis.m_res;
		}

		if (lv2Mod)
		{
			m_connectedModels.insert(url.path(), lv2Mod);
			mod = lv2Mod;
		}
		else
		{
#endif
			qDebug() << "LMMS: Could not create model from "
				 << "OSC port (received port\"" << dest
				 << "\", path \"" << dest << "\")";
			mod = nullptr;
#if 0
		}
#endif
	}
	return mod;
}

template <class UnsignedType> UnsignedType castToUnsigned(int val)
{
	return val >= 0 ? static_cast<unsigned>(val) : 0u;
}

AutomatableModel *Lv2ControlBase::Port::Data::controlData::getModel(PortVis type)
{
	using amp = AutomatableModel*;
	return	(type == PortVis::Integer || type == PortVis::Enumeration)
		? static_cast<amp>(m_connectedModel.m_intModel)
		: type == PortVis::Toggled
		? static_cast<amp>(m_connectedModel.m_boolModel)
		: static_cast<amp>(m_connectedModel.m_floatModel);
}

#endif // LMMS_HAVE_LV2

