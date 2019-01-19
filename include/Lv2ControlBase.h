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

#include <QMutex> // TODO: try to remove
#include <QDebug>
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


enum Lv2IssueType
{
	unknownPortFlow,
	unknownPortType,
	tooManyInputChannels,
	tooManyOutputChannels,
	noOutputChannel,
	noMonoSupport, //!< mono is not yet supported
	portHasNoDef,
	portHasNoMin,
	portHasNoMax,
	featureNotSupported,
	cvPort,
	none
};

class Lv2Issue
{
	Lv2IssueType m_issueType;
	std::string m_info;
	static const char* msgFor(const Lv2IssueType& it);

public:
	Lv2Issue(Lv2IssueType it, std::string msg = std::string())
		: m_issueType(it), m_info(msg)
	{
	}
	friend QDebug operator<<(QDebug stream, const Lv2Issue& iss);
};

QDebug operator<<(QDebug stream, const Lv2Issue& iss);


// TODO: clean up protected/private

/**
	Common util class for Lv2 plugins

	This class provides everything Lv2 plugins have in common. It's not
	named Lv2Plugin, because
	* it does not inherit Instrument
	* The Plugin subclass Effect does not inherit this class

	public members: members accessed from Lv2Manager
	protected members: members accessed from Plugin subclasses and friends


*/
class Lv2ControlBase
{
public:
	static Plugin::PluginTypes check(Lilv::Plugin m_plugin,
		std::vector<Lv2Issue> &issues, bool printIssues = false);

public: /* due to clang bug, should be protected */
	/*
		port structs
	*/
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

	//! @note All Lv2 audio ports are float, this is only the visualisation
	enum class PortVis {
		None,
		Integer,
		Enumeration,
		Toggled
	};

	struct ConstVisitor;
	struct Visitor;

	struct PortMeta
	{
		PortType m_type = PortType::Unknown;
		PortFlow m_flow = PortFlow::Unknown;
		PortVis m_vis = PortVis::None;
		float m_def = .0f, m_min = .0f, m_max = .0f;
		bool m_optional = false;
		bool m_used = true;
	};

	struct PortBase : public PortMeta
	{
		Lilv::Port m_port = Lilv::Port(nullptr, nullptr);

		virtual void accept(Visitor& v) = 0;
		virtual void accept(ConstVisitor& v) const = 0;

		void copyBuffersFromLmms(const sampleFrame *lmmsBuf,
			unsigned channel, fpp_t frames);
		void copyBuffersToLmms(sampleFrame *lmmsBuf,
			unsigned channel, fpp_t frames) const;
		QString name() const;

		virtual ~PortBase();
	};

#define IS_PORT_TYPE \
	void accept(Visitor& v) override { v.visit(*this); } \
	void accept(ConstVisitor& v) const override { v.visit(*this); }

	struct ControlPortBase : public PortBase
	{
		IS_PORT_TYPE

		//! LMMS models
		//! Always up-to-date, except during runs
		union
		{
			class FloatModel *m_floatModel;
			class IntModel *m_intModel;
			class BoolModel *m_boolModel;
		} m_connectedModel;

		AutomatableModel *getModel(Lv2ControlBase::PortVis m_type);
	};

	struct ControlPort : public ControlPortBase
	{
		IS_PORT_TYPE

		//! Data location which Lv2 plugins see
		//! Model values are being copied here every run
		//! Between runs, this data is not up-to-date
		float m_val;
	};

	struct CvPort : public ControlPortBase
	{
		IS_PORT_TYPE

		//! Data location which Lv2 plugins see
		//! Model values are being copied here every run
		//! Between runs, this data is not up-to-date

		std::vector<float> buffer;
	};

	struct AudioPort : public PortBase
	{
		IS_PORT_TYPE

		std::vector<float> buffer;
	};

	struct UnknownPort : public PortBase
	{
		IS_PORT_TYPE
	};
#undef IS_PORT_TYPE

	struct ConstVisitor
	{
#define CAN_VISIT(clss) \
	virtual void visit(const clss& ) {}

		CAN_VISIT(ControlPortBase)
		CAN_VISIT(ControlPort)
		CAN_VISIT(AudioPort)
		CAN_VISIT(CvPort)
		CAN_VISIT(UnknownPort)
		virtual ~ConstVisitor();
#undef CAN_VISIT
	};
	struct Visitor
	{
#define CAN_VISIT(clss) \
	virtual void visit(clss& ) {}

		CAN_VISIT(ControlPortBase)
		CAN_VISIT(ControlPort)
		CAN_VISIT(AudioPort)
		CAN_VISIT(CvPort)
		CAN_VISIT(UnknownPort)
		virtual ~Visitor();
#undef CAN_VISIT
	};


protected:
	const char* portFlowToStr(PortFlow pf);
	const char* portTypeToStr(PortType pt);
	const char* portVisToStr(PortVis pv);

	/*
		port casts
	*/
private:
	template<class Target>
	struct DCastVisitor : public Visitor
	{
		Target* result = nullptr;
		void visit(Target& tar) { result = &tar; }
	};

	template<class Target>
	struct ConstDCastVisitor : public ConstVisitor
	{
		const Target* result = nullptr;
		void visit(const Target& tar) { result = &tar; }
	};

protected:
	//! If you don't want to use a whole visitor, you can use dcast
	template<class Target>
	static Target* dcast(PortBase* base)
	{
		DCastVisitor<Target> vis;
		base->accept(vis);
		return vis.result;
	}

	//! const overload
	template<class Target>
	static const Target* dcast(const PortBase* base)
	{
		ConstDCastVisitor<Target> vis;
		base->accept(vis);
		return vis.result;
	}

	/*
		port access
	 */
	struct StereoPortRef
	{
		//! mono port or left port in case of stereo
		AudioPort* left = nullptr;
		//! unused, or right port in case of stereo
		AudioPort* right = nullptr;
	};

	StereoPortRef& inPorts() { return m_inPorts; }
	StereoPortRef& outPorts() { return m_outPorts; }
	std::vector<PortBase*>& getPorts() { return m_ports; }
	//! Debug function to print ports to stdout
	void dumpPorts();

	/*
		utils for the run thread
	*/
	//! Copy values from all connected models into the respective ports
	void copyModelsFromLmms();
	//! Run the Lv2 plugin instance for @param frames frames
	void run(unsigned frames);

	/*
		load and save
	*/
	//! Create ports and instance, connect ports, activate plugin
	bool initPlugin();
	//! Deactivate instance
	void shutdownPlugin();
	//! TODO: not implemented
	void reloadPlugin();

	/*
		miscellaneous
	 */
	class AutomatableModel *modelAtPort(const QString &uri);
	bool hasGui() const { return m_hasGUI; }
	void setHasGui(bool val) { m_hasGUI = val; }

	/*
		functions to complete virtuals from Plugin subclasses
	*/
	void saveSettings(QDomDocument &doc, QDomElement &that);
	void loadSettings(const QDomElement &that);
	void loadFile(const QString &file);
	std::size_t controlCount() const { return m_controlCount; }
	QString nodeName() const { return "lv2controls"; }

	/*
		ctor/dtor
	*/
	Lv2ControlBase(const QString &uniqueName);
	virtual ~Lv2ControlBase();

private:
	QMutex m_pluginMutex; // TODO: try to remove this

	Lilv::Plugin m_plugin;
	Lilv::Instance* m_instance;

	std::vector<PortBase*> m_ports;
	StereoPortRef m_inPorts, m_outPorts;
	std::size_t m_controlCount = 0;

	std::map<std::string, AutomatableModel *> m_connectedModels;

	bool m_hasGUI;
	bool m_loaded;

	virtual DataFile::Types settingsType() = 0;
	virtual void setNameFromFile(const QString &fname) = 0;

	//! load a file in the plugin, but don't do anything in LMMS
	void loadFileInternal(const QString &file);
	//! allocate m_ports, fill all with metadata, and assign meaning of
	//! ports
	void createPorts();
	//! fill m_ports[portNum] with metadata
	void createPort(unsigned portNum);
	//! connect m_ports[portNum] with Lv2
	void connectPort(unsigned num);
	//! clean up all ports
	void destroyPorts();

	void dumpPort(std::size_t num);

	static std::vector<Lv2Issue> getPortMetaData(Lilv::Plugin &plugin,
				unsigned portNum, PortMeta& meta);
	static Lilv::Node uri(const char* uriStr);
};

#endif // LMMS_HAVE_LV2

#endif // LV2_CONTROL_BASE_H
