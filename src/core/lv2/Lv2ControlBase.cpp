/*
 * Lv2ControlBase.cpp - Lv2 control base class
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

#include <QtGlobal>
#include <QDir>
#include <lv2/state/state.h>

#include "AutomatableModel.h"
#include "Engine.h"
#include "Lv2Manager.h"
#include "Lv2Proc.h"




Plugin::PluginTypes Lv2ControlBase::check(const LilvPlugin *plugin,
	std::vector<PluginIssue> &issues, bool printIssues)
{
	// for some reason, all checks can be done by one processor...
	return Lv2Proc::check(plugin, issues, printIssues);
}




QString Lv2ControlBase::getPresetUri(QString presetPath)
{
	// TODO: move into mgr?
	QFileInfo info(presetPath);
	if(info.isFile())
	{
		// ok...
	}
	else if(info.isDir()) // TODO: change all QDir()... to QFileInfo
	{
		if(!presetPath.endsWith('/')) { presetPath += "/"; }
		presetPath += "testsignal_lv2.ttl"; // TODO
	}
	else
	{
		// no file or directory? Can't get URI
		return QString();
	}

	Lv2Manager* mgr = Engine::getLv2Manager();
	qDebug() << "PATH:" << presetPath;
	LilvState* state = lilv_state_new_from_file(mgr->world(),
						mgr->uridMap().mapFeature(), nullptr,
						presetPath.toUtf8().data());
	QString res = state
			? lilv_node_as_uri(lilv_state_get_plugin_uri(state))
			: QString();
	lilv_state_free(state);
	return res;
}




Lv2ControlBase::Lv2ControlBase(Model* that, const QString &uri) :
	m_plugin(Engine::getLv2Manager()->getPlugin(uri))
{
	if (m_plugin)
	{
		int procId = 0;
		int channelsLeft = DEFAULT_CHANNELS; // LMMS plugins are stereo
		while (channelsLeft > 0)
		{
			std::unique_ptr<Lv2Proc> newOne(
				new Lv2Proc(m_plugin, that, procId++));
			if (newOne->isValid())
			{
				channelsLeft -= std::max(
					1 + static_cast<bool>(newOne->inPorts().m_right),
					1 + static_cast<bool>(newOne->outPorts().m_right));
				Q_ASSERT(channelsLeft >= 0);
				m_procs.push_back(std::move(newOne));
			}
			else
			{
				qCritical() << "Failed instantiating LV2 processor";
				m_valid = false;
				channelsLeft = 0;
			}
		}
		if (m_valid)
		{
			m_channelsPerProc = DEFAULT_CHANNELS / m_procs.size();
			if (m_procs.size() > 1)
			{
				m_procs[0]->makeLinkingProc();
				createMultiChannelLinkModel();
			}

			// initially link all controls
			for (int i = 0; i < static_cast<int>(m_procs[0]->controlCount());
				++i) {
				linkPort(i, true);
			}
		}
	}
	else {
		qCritical() << "No Lv2 plugin found for URI" << uri;
		m_valid = false;
	}
}




Lv2ControlBase::~Lv2ControlBase() {}




LinkedModelGroup *Lv2ControlBase::getGroup(std::size_t idx)
{
	return (m_procs.size() > idx) ? m_procs[idx].get() : nullptr;
}




const LinkedModelGroup *Lv2ControlBase::getGroup(std::size_t idx) const
{
	return (m_procs.size() > idx) ? m_procs[idx].get() : nullptr;
}




void Lv2ControlBase::copyModelsFromLmms() {
	for (std::unique_ptr<Lv2Proc>& c : m_procs) { c->copyModelsFromCore(); }
}




void Lv2ControlBase::copyBuffersFromLmms(const sampleFrame *buf, fpp_t frames) {
	unsigned offset = 0;
	for (std::unique_ptr<Lv2Proc>& c : m_procs) {
		c->copyBuffersFromCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void Lv2ControlBase::copyBuffersToLmms(sampleFrame *buf, fpp_t frames) const {
	unsigned offset = 0;
	for (const std::unique_ptr<Lv2Proc>& c : m_procs) {
		c->copyBuffersToCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void Lv2ControlBase::run(unsigned frames) {
	for (std::unique_ptr<Lv2Proc>& c : m_procs) { c->run(frames); }
}




void Lv2ControlBase::saveSettings(QDomDocument &doc, QDomElement &that)
{
	LinkedModelGroups::saveSettings(doc, that);

	if(hasStateExtension())
	{
		QDomElement states = doc.createElement("states");
		that.appendChild(states);
		for(const std::unique_ptr<Lv2Proc>& proc : m_procs)
		{
			proc->saveState(doc, states);
		}
	}
}




void Lv2ControlBase::loadSettings(const QDomElement &that)
{
	LinkedModelGroups::loadSettings(that);
	if(hasStateExtension())
	{
		QDomElement states = that.firstChildElement("states");
		if(!states.isNull())
		{
			for(std::unique_ptr<Lv2Proc>& proc : m_procs)
			{
				proc->loadState(states);
			}
		}
	}
}




void Lv2ControlBase::loadFile(const QString &file)
{
	/*
		file can be:
		* A state.ttl file
		* A directory containing state.ttl
		* A directory containing chan0, chan1,... each containing state.ttl
	*/

	/*
		1 processor => obvious
		>1 processor => look for chan1, chan2
			=> if they exist, check size and load separately
			=> if there's only one, load one for all
	*/
	if(controls().size() == 1)
	{
		// 1 processor, so file must be one preset
		// (if it has multiple presets, the following will abort)
		controls()[0]->loadFile(file);
	}
	else
	{
		QDir fileAsDir(file);
		if(fileAsDir.exists())
		{
			if(QDir(file + "state.ttl").exists())
			{
				// only one channel, provided, so load that one into all processors
				for(std::unique_ptr<Lv2Proc>& proc : controls()) { proc->loadFile(file); }
			}
			else
			{
				// Check if we have multiple channels
				QString channel = "chanX";
				std::size_t nChans;
				bool finish = false;
				for(nChans = 0; nChans < controls().size() && !finish; ++nChans)
				{
					channel[4] = static_cast<char>('0' + nChans);
					finish = !QDir(channel).exists();
					if(!finish) {
						controls()[nChans]->loadFile(channel);
					}
				}
				Q_ASSERT(nChans>0); // of nChanes == 0, the preset had no channels
			}
		}
		else if(QFile::exists(file))
		{
			// only one channel, provided, so load that one into all processors
			for(std::unique_ptr<Lv2Proc>& proc : controls()) { proc->loadFile(file); }
		}
		else {
			// neither directory, nor file?
			Q_ASSERT(false);
		}
	}
	(void)file;
}




void Lv2ControlBase::reloadPlugin()
{
	// TODO
}




std::size_t Lv2ControlBase::controlCount() const {
	std::size_t res = 0;
	for (const std::unique_ptr<Lv2Proc>& c : m_procs) { res += c->controlCount(); }
	return res;
}




bool Lv2ControlBase::hasStateExtension() const
{
	return lilv_plugin_has_extension_data(m_plugin,
		AutoLilvNode(lilv_new_uri(Engine::getLv2Manager()->world(),
					LV2_STATE__interface)).get());
}


#endif // LMMS_HAVE_LV2
