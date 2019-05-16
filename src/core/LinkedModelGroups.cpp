/*
 * LinkedModelGroups.cpp - base classes for groups of linkable models
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "LinkedModelGroups.h"

#include <QDomDocument>
#include <QDomElement>

#include "AutomatableModel.h"
#include "ComboBoxModel.h"




/*
	LinkedModelGroup
*/




void LinkedModelGroup::makeLinkingProc()
{
	for (std::size_t i = 0; i < m_models.size(); ++i)
	{
		BoolModel* bmo = new BoolModel(true, this, tr("Link channels"));
		m_linkEnabled.push_back(bmo);
		connect(bmo, SIGNAL(dataChanged()), this, SLOT(linkStateChangedSlot()));
	}
}




void LinkedModelGroup::linkAllModels(bool state)
{
	for (BoolModel* bmo : m_linkEnabled) { bmo->setValue(state); }
}




void LinkedModelGroup::linkControls(LinkedModelGroup *other, int id)
{
	Q_ASSERT(id >= 0);
	std::size_t id2 = static_cast<std::size_t>(id);
	AutomatableModel::linkModels(
		m_models[id2].m_model, other->m_models[id2].m_model);
}




void LinkedModelGroup::unlinkControls(LinkedModelGroup *other, int id)
{
	Q_ASSERT(id >= 0);
	std::size_t id2 = static_cast<std::size_t>(id);
	AutomatableModel::unlinkModels(
		m_models[id2].m_model, other->m_models[id2].m_model);
}




void LinkedModelGroup::saveValues(QDomDocument &doc, QDomElement &that)
{
	for(const ModelInfo& minf : m_models)
	{
		minf.m_model->saveSettings(doc, that, minf.m_name);
	}
}




void LinkedModelGroup::saveLinksEnabled(QDomDocument &doc, QDomElement &that)
{
	if(m_linkEnabled.size())
	{
		std::size_t count = 0;
		for(BoolModel* bmo : m_linkEnabled)
		{
			//that.setAttribute(m_models[count++].m_name, bmo->value());
			bmo->saveSettings(doc, that, m_models[count++].m_name);
		}
	}
}




void LinkedModelGroup::loadValues(const QDomElement &that)
{
	for(ModelInfo& minf : m_models)
	{
		// try to load, if it fails, this will load a sane initial value
		minf.m_model->loadSettings(that, minf.m_name);
	}
}




void LinkedModelGroup::loadLinksEnabled(const QDomElement &that)
{
	if(m_linkEnabled.size())
	{
		std::size_t count = 0;
		for(BoolModel* bmo : m_linkEnabled)
		{
			bmo->loadSettings(that, m_models[count++].m_name);
		}
	}
}




void LinkedModelGroup::linkStateChangedSlot()
{
	QObject* sender = QObject::sender();
	Q_ASSERT(sender);
	BoolModel* bmo = qobject_cast<BoolModel*>(sender);
	Q_ASSERT(bmo);
	int modelNo = -1, count = 0;
	for (BoolModel* bmo2 : m_linkEnabled)
	{
		if (bmo2 == bmo) { modelNo = count; }
		++count;
	}
	Q_ASSERT(modelNo >= 0);
	emit linkStateChanged(modelNo, bmo->value());
}




void LinkedModelGroup::addModel(AutomatableModel *model, const QString &name)
{
	m_models.emplace_back(name, model);
}




/*
	LinkedModelGroups
*/




LinkedModelGroups::~LinkedModelGroups() {}




void LinkedModelGroups::createMultiChannelLinkModel()
{
	m_multiChannelLinkModel.reset(new BoolModel(true, nullptr));
}




void LinkedModelGroups::linkPort(int port, bool state)
{
	LinkedModelGroup* first = getGroup(0);
	LinkedModelGroup* cur;

	if (state)
	{
		for (std::size_t i = 1; (cur=getGroup(i)); ++i)
		{
			first->linkControls(cur, port);
		}
	}
	else
	{
		for (std::size_t i = 1; (cur=getGroup(i)); ++i)
		{
			first->unlinkControls(cur, port);
		}

		// m_multiChannelLinkModel.setValue() will call
		// updateLinkStatesFromGlobal()...
		// m_noLink will make sure that this will not unlink any other ports
		m_noLink = true;
		m_multiChannelLinkModel->setValue( false );
	}
}




void LinkedModelGroups::updateLinkStatesFromGlobal()
{
	LinkedModelGroup* first = getGroup(0);
	if (m_multiChannelLinkModel->value())
	{
		first->linkAllModels(true);
	}
	else if (!m_noLink)
	{
		first->linkAllModels(false);
	}

	// if global channel link state has changed, always ignore link
	// status of individual ports in the future
	m_noLink = false;
}




struct CompareModels : public ConstModelVisitor
{
	const AutomatableModel* m_other; // in
	bool m_equal; // out

	void visit(const FloatModel& m) override {
		// most knobs are probably not more exact than 0.001
		m_equal =
			m.value() - m_other->dynamicCast<FloatModel>(m_other)->value()
			< .001f;
	}
	void visit(const IntModel& m) override {
		cmp(m, *m_other->dynamicCast<IntModel>(m_other));
	}
	void visit(const BoolModel& m) override {
		cmp(m, *m_other->dynamicCast<BoolModel>(m_other));
	}
	void visit(const ComboBoxModel& m) override {
		cmp(m, *m_other->dynamicCast<ComboBoxModel>(m_other));
	}
private:
	template<class T>
	void cmp(const TypedAutomatableModel<T>& a1,
		const TypedAutomatableModel<T>& a2) {
		m_equal = a1.value() == a2.value();
	}
};




bool LinkedModelGroups::allModelsEqual() const
{
	bool allEqual = true;
	const LinkedModelGroup* lmg0 = getGroup(0);
	for(std::size_t mdl = 0; mdl < lmg0->models().size() && allEqual; ++mdl)
	{
		if(lmg0->linkEnabledModel(mdl)->value())
		{
			// if they are linked, we know they're equal => no check
		}
		else
		{
			for(std::size_t grp = 1; grp; ++grp)
			{
				const LinkedModelGroup* lmg = getGroup(grp);
				if(lmg)
				{
					CompareModels cmp;
					cmp.m_other = lmg->models()[mdl].m_model;
					lmg0->models()[mdl].m_model->accept(cmp);
					allEqual = cmp.m_equal;
				}
				else {
					grp = 0;
				}
			}
		}
	}
	return allEqual;
}




void LinkedModelGroups::saveSettings(QDomDocument& doc, QDomElement& that)
{
	if(getGroup(0))
	{
		m_multiChannelLinkModel->saveSettings(doc, that, "link");

		{
			QDomElement links = doc.createElement("links");
			getGroup(0)->saveLinksEnabled(doc, links);
			that.appendChild(links);
		}

		QDomElement models = doc.createElement("models");
		that.appendChild(models);

		char chanName[] = "chan0";
		for(char* chanPtr = chanName + 4; *chanPtr >= '0'; ++*chanPtr)
		{
			LinkedModelGroup* lmg = getGroup(static_cast<std::size_t>(
												*chanPtr - '0'));
			if(lmg)
			{
				QDomElement channel = doc.createElement(
										QString::fromUtf8(chanName));
				models.appendChild(channel);
				lmg->saveValues(doc, channel);
			}
			else {
				*chanPtr = 0;
			}
		}
	}
	else {
		// don't even add a "models" node
	}
}




void LinkedModelGroups::loadSettings(const QDomElement& that)
{
	QDomElement models = that.firstChildElement("models");
	if(!models.isNull() && getGroup(0))
	{
		m_multiChannelLinkModel->loadSettings(that, "link");

		{
			QDomElement links = that.firstChildElement("links");
			if(!links.isNull()) { getGroup(0)->loadLinksEnabled(links); }
		}

		QDomElement lastChan;
		char chanName[] = "chan0";
		for(char* chanPtr = chanName + 4; *chanPtr >= '0'; ++*chanPtr)
		{
			LinkedModelGroup* lmg = getGroup(static_cast<std::size_t>(
												*chanPtr - '0'));
			if(lmg)
			{
				QDomElement chan = models.firstChildElement(chanName);
				if(!chan.isNull()) { lastChan = chan; }
				lmg->loadValues(lastChan);
			}
			else {
				*chanPtr = 0;
			}
		}
	}
}


