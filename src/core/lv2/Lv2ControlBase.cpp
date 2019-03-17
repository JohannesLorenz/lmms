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




Lv2ControlBase::Lv2ControlBase(Model* that, const QString &uri) :
	m_plugin(Engine::getLv2Manager()->getPlugin(uri))
{
	if(m_plugin)
	{
		int channelsLeft = DEFAULT_CHANNELS; // LMMS plugins are stereo
		while (channelsLeft > 0)
		{
			Lv2Proc* newOne = new Lv2Proc(m_plugin, that);
			if(newOne->isValid())
			{
				channelsLeft -= std::max(
					1 + static_cast<bool>(newOne->inPorts().m_right),
					1 + static_cast<bool>(newOne->outPorts().m_right));
				Q_ASSERT(channelsLeft >= 0);
				m_procs.push_back(newOne);
			}
			else
			{
				qCritical() << "Failed instantiating LV2 processor";
				m_valid = false;
				channelsLeft = 0;
			}
		}
		if(m_valid)
		{
			m_channelsPerProc = DEFAULT_CHANNELS / m_procs.size();
			if(m_procs.size() > 1)
			{
				m_procs[0]->makeLinkingProc();
				createMultiChannelLinkModel();
			}

			// initially link all controls
			for(int i = 0; i < static_cast<int>(m_procs[0]->controlCount());
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




Lv2ControlBase::~Lv2ControlBase()
{
	for(Lv2Proc* c : m_procs) { delete c; }
}




LinkedModelGroup *Lv2ControlBase::getGroup(std::size_t idx)
{
	return (m_procs.size() > idx) ? m_procs[idx] : nullptr;
}




void Lv2ControlBase::copyModelsFromLmms() {
	for(Lv2Proc* c : m_procs) { c->copyModelsFromCore(); }
}




void Lv2ControlBase::copyBuffersFromLmms(const sampleFrame *buf, fpp_t frames) {
	unsigned offset = 0;
	for(Lv2Proc* c : m_procs) {
		c->copyBuffersFromCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void Lv2ControlBase::copyBuffersToLmms(sampleFrame *buf, fpp_t frames) const {
	unsigned offset = 0;
	for(const Lv2Proc* c : m_procs) {
		c->copyBuffersToCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void Lv2ControlBase::run(unsigned frames) {
	for(Lv2Proc* c : m_procs) { c->run(frames); }
}




void Lv2ControlBase::saveSettings(QDomDocument &doc, QDomElement &that)
{
	// TODO
	(void)doc;
	(void)that;
}




void Lv2ControlBase::loadSettings(const QDomElement &that)
{
	(void)that;
}




void Lv2ControlBase::loadFile(const QString &file)
{
	(void)file;
}




void Lv2ControlBase::reloadPlugin()
{
	// TODO
}




std::size_t Lv2ControlBase::controlCount() const {
	std::size_t res = 0;
	for(const Lv2Proc* c : m_procs)
		res += c->controlCount();
	return res;
}




#endif // LMMS_HAVE_LV2
