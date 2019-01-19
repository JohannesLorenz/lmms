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
#include <algorithm>
#include <cassert>
#include <lv2.h>

#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "Mixer.h"
#include "Lv2Manager.h"
#include "embed.h"
#include "gui_templates.h"




Lilv::Node Lv2ControlBase::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}




const char *Lv2ControlBase::portFlowToStr(Lv2ControlBase::PortFlow pf)
{
	switch(pf)
	{
	case PortFlow::Unknown: return "unknown";
	case PortFlow::Input: return "input";
	case PortFlow::Output: return "output";
	}
	return "";
}

const char *Lv2ControlBase::portTypeToStr(Lv2ControlBase::PortType pt)
{
	switch(pt)
	{
	case PortType::Unknown: return "unknown";
	case PortType::Control: return "control";
	case PortType::Audio: return "audio";
	case PortType::Event: return "event";
	case PortType::Cv: return "cv";
	}
	return "";
}

const char *Lv2ControlBase::portVisToStr(Lv2ControlBase::PortVis pv)
{
	switch(pv)
	{
	case PortVis::Toggled: return "toggled";
	case PortVis::Enumeration: return "enumeration";
	case PortVis::Integer: return "integer";
	case PortVis::None: return "none";
	}
	return "";
}

Lv2ControlBase::Lv2ControlBase(const QString& uri) :
	m_plugin(Engine::getLv2Manager()->getPlugin(uri)->me),
	m_hasGUI(false)
{
	if(!m_plugin)
	{
		qDebug() << ":-( ! No descriptor found for" << uri;
	}
	// TODO: error handling?

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

void Lv2ControlBase::run(unsigned frames)
{
	m_instance->run(static_cast<unsigned>(frames));
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




void Lv2ControlBase::copyModelsFromLmms()
{
	struct Copy : public Visitor
	{
		void visit(ControlPort& ctrl)
		{
			if(ctrl.m_flow == PortFlow::Input)
			{
				auto& mdl = ctrl.m_connectedModel;
				switch (ctrl.m_vis)
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
		}
		void visit(CvPort& cv)
		{
			if(cv.m_flow == PortFlow::Input)
			{
				// simulate cv ports, LMMS can't handle them yet
				auto& mdl = cv.m_connectedModel;
				switch (cv.m_vis)
				{
					case PortVis::None:
						std::fill(cv.buffer.begin(), cv.buffer.end(), mdl.m_floatModel->value());
						break;
					case PortVis::Integer:
					case PortVis::Enumeration:
						std::fill(cv.buffer.begin(), cv.buffer.end(),
							static_cast<float>(mdl.m_intModel->value()));
						break;
					case PortVis::Toggled:
						std::fill(cv.buffer.begin(), cv.buffer.end(),
							static_cast<float>(mdl.m_boolModel->value()));
						break;
				}
			}
		}
	} copy;

	for (PortBase* port : m_ports)
	{
		port->accept(copy);
	}
}




void Lv2ControlBase::PortBase::copyBuffersFromLmms(const sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	AudioPort* audio = dcast<AudioPort>(this);
	if(audio)
	{
		unsigned framesUnsigned = static_cast<unsigned>(frames);
		for (std::size_t f = 0; f < framesUnsigned; ++f)
		{
			audio->buffer[f] = lmmsBuf[f][channel];
		}
	}
}




void Lv2ControlBase::PortBase::copyBuffersToLmms(sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames) const
{
	const AudioPort* audio = dcast<AudioPort>(this);
	if(audio)
	{
		unsigned framesUnsigned = static_cast<unsigned>(frames);
		for (std::size_t f = 0; f < framesUnsigned; ++f)
		{
			lmmsBuf[f][channel] = audio->buffer[f];
		}
	}
}




QString Lv2ControlBase::PortBase::name() const
{
	LilvNode* node = lilv_port_get_name(m_port.parent, m_port.me);
	QString res = lilv_node_as_string(node);
	lilv_node_free(node);
	return res;
}




void Lv2ControlBase::shutdownPlugin()
{
	m_instance->deactivate();
	m_instance = nullptr;
}




std::vector<Lv2Issue> Lv2ControlBase::getPortMetaData(
	Lilv::Plugin& plugin, unsigned portNum,
	PortMeta &meta)
{
	std::vector<Lv2Issue> portIssues;
	auto issue = [&portIssues](Lv2IssueType i, const char* msg = "") {
		portIssues.emplace_back(i, msg); };

	Lv2Manager* man = Engine::getLv2Manager();
	// TODO: simplify: lilvPort.is_a(uri(LV2_CORE__InputPort))
	Lilv::Node InputPort = man->uri(LV2_CORE__InputPort);
	Lilv::Node OutputPort = man->uri(LV2_CORE__OutputPort);
	Lilv::Node AudioPort = man->uri(LV2_CORE__AudioPort);
	Lilv::Node ControlPort = man->uri(LV2_CORE__ControlPort);
	Lilv::Node CvPort = man->uri(LV2_CORE__CVPort);
	Lilv::Node connectionOptional = man->uri(LV2_CORE__connectionOptional);

	Lilv::Port lilvPort = plugin.get_port_by_index(portNum);
	const char* portName = lilv_node_as_string(lilvPort.get_name());

	meta.m_optional = lilvPort.has_property(connectionOptional);

	Lilv::Node propInt = man->uri(LV2_CORE__integer);
	Lilv::Node propToggled = man->uri(LV2_CORE__toggled);
	Lilv::Node propEnumeration = man->uri(LV2_CORE__enumeration);

	meta.m_vis = lilvPort.has_property(propInt)
		? PortVis::Integer
		: lilvPort.has_property(propEnumeration)
		? PortVis::Enumeration
		: lilvPort.has_property(propToggled)
		? PortVis::Toggled
		: PortVis::None;

	if (lilvPort.is_a(InputPort)) {
		meta.m_flow = PortFlow::Input;
	}
	else if (lilvPort.is_a(OutputPort)) {
		meta.m_flow = PortFlow::Output;
	} else {
		meta.m_flow = PortFlow::Unknown;
		issue(unknownPortFlow, portName);
	}

	meta.m_def = .0f; meta.m_min = .0f; meta.m_max = .0f;

	if (lilvPort.is_a(ControlPort)) {
		meta.m_type = PortType::Control;

		if(meta.m_flow == PortFlow::Input)
		{
			bool isToggle = meta.m_vis == PortVis::Toggled;

			// lilvmm does not support ranges yet, so use lilv C lib
			LilvNode * defN, * minN = nullptr, * maxN = nullptr;
			lilv_port_get_range(plugin.me, lilvPort.me, &defN,
					isToggle ? nullptr : &minN,
					isToggle ? nullptr : &maxN);

			auto takeRangeValue = [&](LilvNode* node,
				float& storeHere, Lv2IssueType it) {
				if(node) {
					storeHere = lilv_node_as_float(node);
				}
				else {
					issue(it, portName);
				}
				lilv_node_free(node);
			};

			takeRangeValue(defN, meta.m_def, portHasNoDef);
			if(!isToggle)
			{
				takeRangeValue(minN, meta.m_min, portHasNoMin);
				takeRangeValue(maxN, meta.m_max, portHasNoMax);
			}
		}
	}
	else if (lilvPort.is_a(AudioPort)) {
		meta.m_type = PortType::Audio;
	} else if (lilvPort.is_a(CvPort)) {
		issue(cvPort);
		meta.m_type = PortType::Cv;
	} else {
		if (meta.m_optional) {
			meta.m_used = false;
		}
		else {
			issue(Lv2IssueType::unknownPortType, portName);
			meta.m_type = PortType::Unknown;
		}
	}

	// TODO: logarithmic, noOnGui, rangeSteps

	return portIssues;
}




Plugin::PluginTypes Lv2ControlBase::check(Lilv::Plugin plugin,
	std::vector<Lv2Issue>& issues, bool printIssues)
{
	unsigned maxPorts = plugin.get_num_ports();
	unsigned audioChannels[2] = { 0, 0 }; // input and output count

	for(unsigned portNum = 0; portNum < maxPorts; ++portNum)
	{
		PortMeta meta;
		// does all checks:
		std::vector<Lv2Issue> tmp =
			getPortMetaData(plugin, portNum, meta);
		std::move(tmp.begin(), tmp.end(), std::back_inserter(issues));

		if(meta.m_type == PortType::Audio)
			++audioChannels[meta.m_flow == PortFlow::Output];
	}

	if(audioChannels[0] > 2)
		issues.emplace_back(tooManyInputChannels,
			std::to_string(audioChannels[0]));
	if(audioChannels[1] == 0)
		issues.emplace_back(noOutputChannel);
	else if(audioChannels[1] > 2)
		issues.emplace_back(tooManyOutputChannels,
			std::to_string(audioChannels[1]));
	if(audioChannels[0] * audioChannels[1] == 1)
		issues.emplace_back(noMonoSupport);

	if(printIssues && issues.size())
	{
		qDebug() << "Lv2 plugin " << plugin.get_name().as_string()
			<< " can not be loaded:";
		for(const Lv2Issue& issue : issues)
		{
			qDebug() << "  - " << issue;
		}
	}

	Lilv::Nodes reqFeats = plugin.get_required_features();
	for(LilvIter* itr = reqFeats.begin();
		! reqFeats.is_end(itr); itr = reqFeats.next(itr))
	{
		issues.emplace_back(featureNotSupported,
			lilv_node_as_string(reqFeats.get(itr).me));
	}

	return (audioChannels[1] > 2 || audioChannels[0] > 2)
		? Plugin::Undefined
		: audioChannels[0]
			? Plugin::Effect
			: Plugin::Instrument;
}

void Lv2ControlBase::dumpPort(std::size_t num)
{
	struct DumpPortDetail : public ConstVisitor
	{
		void visit(const ControlPort& ctrl) override {
			qDebug() << "  control port";
			// output ports may be uninitialized yet, only print inputs
			if(ctrl.m_flow == PortFlow::Input)
			{
				qDebug() << "    value:" << ctrl.m_val;
			}
		}
		void visit(const AudioPort& audio) override {
			qDebug() << "  audio port";
			qDebug() << "    buffer size:" << audio.buffer.size();
		}
	};

	const PortBase& port = *m_ports[num];
	qDebug().nospace() << "port " << num << ":";
	qDebug() << "  name:"
		<< lilv_node_as_string(lilv_port_get_name(m_plugin, port.m_port));
	qDebug() << "  flow: " << portFlowToStr(port.m_flow);
	qDebug() << "  type: " << portTypeToStr(port.m_type);
	qDebug() << "  visualization: " << portVisToStr(port.m_vis);
	if(port.m_type == PortType::Control || port.m_type == PortType::Cv)
	{
		qDebug() << "  default:" << port.m_def;
		qDebug() << "  min:" << port.m_min;
		qDebug() << "  max:" << port.m_max;
	}
	qDebug() << "  optional: " << port.m_optional;
	qDebug() << "  => USED: " << port.m_used;

	DumpPortDetail dumper;
	port.accept(dumper);
}

void Lv2ControlBase::dumpPorts()
{
	std::size_t num = 0;
	for(const PortBase* port: m_ports)
	{
		(void)port;
		dumpPort(num++);
	}
}


void Lv2ControlBase::createPort(unsigned portNum)
{
	PortBase*& port = m_ports[portNum];

	PortMeta meta;
	getPortMetaData(m_plugin, portNum, meta);

	if (meta.m_type == PortType::Control)
	{
		ControlPort* ctrl = new ControlPort;
		if(meta.m_flow == PortFlow::Input)
		{
			switch(meta.m_vis)
			{
				case PortVis::None:
					ctrl->m_connectedModel.m_floatModel
						= new FloatModel(meta.m_def,
							meta.m_min,
							meta.m_max, 0.1f /*TODO*/,
							nullptr, ctrl->name());
					break;
				case PortVis::Integer:
				case PortVis::Enumeration:
					ctrl->m_connectedModel.m_intModel
						= new IntModel(
							static_cast<int>(meta.m_def),
							static_cast<int>(meta.m_min),
							static_cast<int>(meta.m_max),
							nullptr, ctrl->name());
					break;
				case PortVis::Toggled:
					ctrl->m_connectedModel.m_boolModel
						= new BoolModel(
							static_cast<bool>(meta.m_def),
							nullptr, ctrl->name());
					break;
			}
		}
		port = ctrl;
	}
	else if (meta.m_type == PortType::Audio) {
		AudioPort* audio = new AudioPort;
		audio->buffer.resize(static_cast<std::size_t>(
					Engine::mixer()->framesPerPeriod()));
		port = audio;
	} else {
		port = new UnknownPort;
	}

	*dynamic_cast<PortMeta*>(port) = meta;
	port->m_port = m_plugin.get_port_by_index(portNum);
}




void Lv2ControlBase::destroyPorts()
{
	for(PortBase* p: m_ports)
	{
		delete p;
	}
}




void Lv2ControlBase::createPorts()
{
	// register ports at the control base after creation,
	// i.e. link their data or count them
	struct RegisterPort : public Visitor
	{
		Lv2ControlBase* controlBase;

		void visit(ControlPort& ctrl)
		{
			if(ctrl.m_flow == PortFlow::Input)
			{
				controlBase->m_connectedModels.emplace(
					lilv_node_as_string(ctrl.m_port.get_symbol()),
					ctrl.getModel(ctrl.m_vis));
				++controlBase->m_controlCount;
			}
		}

		void visit(AudioPort& audio)
		{
			StereoPortRef* portRef;
			StereoPortRef dummy;
			switch(audio.m_flow)
			{
				case PortFlow::Input:
					portRef = &controlBase->m_inPorts;
					break;
				case PortFlow::Output:
					portRef = &controlBase->m_outPorts;
					break;
				case PortFlow::Unknown:
					portRef = &dummy;
					break;
			}
			// in Lv2, leftPort is defined to be the first port
			if(!portRef->left) {
				portRef->left = &audio;
			} else if(!portRef->right) {
				portRef->right = &audio;
			}
		}
	};

	unsigned maxPorts = m_plugin.get_num_ports();
	m_ports.resize(maxPorts);

	for(unsigned portNum = 0; portNum < maxPorts; ++portNum)
	{
		createPort(portNum);
		RegisterPort registerPort;
		registerPort.controlBase = this;
		m_ports[portNum]->accept(registerPort);
	}

	// initially assign model values to port values
	copyModelsFromLmms();

	dumpPorts();
}




AutomatableModel *Lv2ControlBase::ControlPortBase::getModel(PortVis type)
{
	using amp = AutomatableModel*;
	return	(type == PortVis::Integer || type == PortVis::Enumeration)
		? static_cast<amp>(m_connectedModel.m_intModel)
		: type == PortVis::Toggled
		? static_cast<amp>(m_connectedModel.m_boolModel)
		: static_cast<amp>(m_connectedModel.m_floatModel);
}




void Lv2ControlBase::connectPort(unsigned num)
{
	struct Connect : public Visitor
	{
		unsigned m_num;
		Lilv::Instance* m_instance;
		void con(void* location) {
			m_instance->connect_port(m_num, location);
		}
		void visit(ControlPort& ctrl) { con(&ctrl.m_val); }
		void visit(AudioPort& audio) { con(&audio.buffer); }
		void visit(UnknownPort&) { con(nullptr); }
	} connect;
	connect.m_num = num;
	connect.m_instance = m_instance;
	m_ports[num]->accept(connect);
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
		createPorts();

		m_instance = Lilv::Instance::create(m_plugin,
			Engine::mixer()->processingSampleRate(),
			nullptr);

		if(!m_instance)
		{
			qCritical() << "Failed to create an instance of"
				<< m_plugin.get_name().as_string();
		}

		m_pluginMutex.unlock();

		for(unsigned portNum = 0; portNum < m_ports.size(); ++portNum)
			connectPort(portNum);

		m_instance->activate();
	}

	return true;
}




AutomatableModel *Lv2ControlBase::modelAtPort(const QString &uri)
{
	// unused currently
	AutomatableModel *mod;
	auto itr = m_connectedModels.find(uri.toUtf8().data());
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
				 << "OSC port (received port\"" << uri
				 << "\", path \"" << uri << "\")";
			mod = nullptr;
#if 0
		}
#endif
	}
	return mod;
}




// make the compiler happy, give each class with virtuals
// a function (the destructor here) which is in a cpp file
Lv2ControlBase::PortBase::~PortBase() {}
Lv2ControlBase::ConstVisitor::~ConstVisitor() {}
Lv2ControlBase::Visitor::~Visitor() {}

#endif // LMMS_HAVE_LV2


const char *Lv2Issue::msgFor(const Lv2IssueType &it)
{
	switch (it)
	{
		case unknownPortFlow:
			return "unknown port flow for mandatory port";
		case unknownPortType:
			return "unknown port type for mandatory port";
		case tooManyInputChannels:
			return "too many input channels";
		case tooManyOutputChannels:
			return "too many output channels";
		case noOutputChannel:
			return "no output channel";
		case noMonoSupport:
			return "mono input or output";
		case portHasNoDef:
			return "port is missing default value";
		case portHasNoMin:
			return "port is missing min value";
		case portHasNoMax:
			return "port is missing max value";
		case featureNotSupported:
			return "feature not supported";
		case cvPort:
			return "cv port not supported";
		case none:
			return nullptr;
	}
	return nullptr;
}

QDebug operator<<(QDebug stream, const Lv2Issue &iss)
{
	stream << Lv2Issue::msgFor(iss.m_issueType);
	if(iss.m_info.length())
	{
		stream.nospace() << ": " << iss.m_info.c_str();
	}
	return stream;
}
