/*
 * LinkedModelGroupViews.h - views for groups of linkable models
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

#include "LinkedModelGroupViews.h"

#include <QGridLayout>

#include "Controls.h"
#include "LedCheckbox.h"
#include "LinkedModelGroups.h"


/*
	LinkedModelGroupViewBase
*/


LinkedModelGroupViewBase::LinkedModelGroupViewBase(QWidget* parent,
	bool isLinking, int colNum, int curProc, int nProc, const QString& name) :
	QGroupBox(parent),
	m_colNum(colNum),
	m_isLinking(isLinking),
	m_grid(new QGridLayout(this))
{
	QString chanName;
	if(name.isNull())
	{
		switch(nProc)
		{
			case 1: break; // don't display any channel name
			case 2:
				chanName = QObject::tr(curProc ? "Right" : "Left");
				break;
			default:
				chanName = QObject::tr("Channel ") + QString::number(curProc + 1);
				break;
		}
	}
	else {
		chanName = name;
	}

	if(!chanName.isNull()) { setTitle(chanName); }
}




LinkedModelGroupViewBase::~LinkedModelGroupViewBase() {}




void LinkedModelGroupViewBase::modelChanged(LinkedModelGroup *group)
{
	// reconnect models
	QVector<ControlBase*>::Iterator itr = m_controls.begin();
	std::vector<AutomatableModel*> models = group->models();
	Q_ASSERT(m_controls.size() == static_cast<int>(models.size()));

	for(AutomatableModel* mdl : models)
	{
		(*itr++)->setModel(mdl);
	}

	std::size_t count = 0;
	for (LedCheckBox* led : m_leds)
	{
		led->setModel(group->linkEnabledModel(count++));
	}
}




void LinkedModelGroupViewBase::addControl(ControlBase* ctrl)
{
	int colNum2 = m_colNum * (1 + m_isLinking);
	int wdgNum = m_controls.size() * (1 + m_isLinking);
	if(ctrl)
	{
		int x = wdgNum%colNum2, y = wdgNum/colNum2;

		// start in row one, add widgets cell by cell
		if(m_isLinking) {
			LedCheckBox* cb = new LedCheckBox(nullptr);
			m_grid->addWidget(cb, y, x);
			m_leds.push_back(cb);
		}

		m_controls.push_back(ctrl);
		m_grid->addWidget(ctrl->topWidget(), y, x + 1, Qt::AlignCenter);
		wdgNum += m_isLinking;
		++wdgNum;
	}

	// matter of taste (takes a lot of space):
	// makeAllGridCellsEqualSized();
}




void LinkedModelGroupViewBase::makeAllGridCellsEqualSized()
{
	int rowHeight = 0, colWidth = 0;
	for(int row = 0; row < m_grid->rowCount(); ++row)
	{
		for(int col = 0; col < m_grid->columnCount(); ++col)
		{
			QLayoutItem* layout;
			if((layout = m_grid->itemAtPosition(row, col)))
			{
				rowHeight = qMax(rowHeight, layout->geometry().height());
				colWidth = qMax(colWidth, layout->geometry().width());
			}
		}
	}

	for(int row = 0; row < m_grid->rowCount(); ++row)
	{
		m_grid->setRowMinimumHeight(row, rowHeight);
	}

	for(int col = 0; col < m_grid->columnCount(); ++col)
	{
		m_grid->setColumnMinimumWidth(col, colWidth);
	}
}


/*
	LinkedModelGroupsViewBase
*/


LinkedModelGroupsViewBase::LinkedModelGroupsViewBase(
	LinkedModelGroups *ctrlBase)
{
	if(ctrlBase->multiChannelLinkModel())
	{
		m_multiChannelLink = new LedCheckBox(QObject::tr("Link Channels"),
												nullptr);
	}
}




LinkedModelGroupsViewBase::~LinkedModelGroupsViewBase() {}




void LinkedModelGroupsViewBase::modelChanged(LinkedModelGroups *groups)
{
	if(groups->multiChannelLinkModel())
	{
		m_multiChannelLink->setModel(groups->multiChannelLinkModel());
	}

	for(std::size_t i = 0; getGroupView(i) && groups->getGroup(i); ++i)
	{
		getGroupView(i)->modelChanged(groups->getGroup(i));
	}
}


