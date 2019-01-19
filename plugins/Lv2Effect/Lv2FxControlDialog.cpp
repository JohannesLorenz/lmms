/*
 * Lv2ControlDialog.cpp - control dialog for amplifier effect
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

#include "Lv2FxControlDialog.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>
#include <lv2.h>

#include "Knob.h"
#include "LcdSpinBox.h"
#include "LedCheckbox.h"
#include "Lv2Effect.h"
#include "Lv2FxControls.h"
#include "embed.h"
#include "gui_templates.h"

Lv2FxControls *Lv2FxControlDialog::lv2Controls()
{
	return static_cast<Lv2FxControls *>(m_effectControls);
}

Lv2FxControlDialog::Lv2FxControlDialog(Lv2FxControls *controls) :
	EffectControlDialog(controls)
{
	QGridLayout *grid = new QGridLayout(this);

	m_reloadPluginButton = new QPushButton(tr("Reload Plugin"), this);
	grid->addWidget(m_reloadPluginButton, 0, 0, 1, 3);

	connect(m_reloadPluginButton, SIGNAL(toggled(bool)), this,
		SLOT(reloadPlugin()));

	if (/* DISABLES CODE */ (false)) // TODO: check if the plugin has the UI extension
	{
		m_toggleUIButton = new QPushButton(tr("Show GUI"), this);
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(
			pointSize<8>(m_toggleUIButton->font()));
		connect(m_toggleUIButton, SIGNAL(toggled(bool)), this,
			SLOT(toggleUI()));
		m_toggleUIButton->setWhatsThis(
			tr("Click here to show or hide the graphical user "
			   "interface "
			   "(GUI) of Osc."));
		grid->addWidget(m_toggleUIButton, 0, 3, 1, 3);
	}

	struct SetupWidget : public Lv2ControlBase::Visitor
	{
		QWidget* wdg = nullptr; // output
		Lv2FxControlDialog* dlg; // input
		void visit(Lv2ControlBase::ControlPort& ctrl) override
		{
			if(ctrl.m_flow == Lv2ControlBase::PortFlow::Input)
			{
				using PortVis = Lv2ControlBase::PortVis;

				switch (ctrl.m_vis)
				{
					case PortVis::None:
					{
						Knob *k = new Knob(dlg);
						k->setModel(ctrl.
							m_connectedModel.m_floatModel);
						assert(k->model());
						wdg = k;
						break;
					}
					case PortVis::Integer:
					case PortVis::Enumeration:
					{
						IntModel* mdl =
							ctrl.m_connectedModel.m_intModel;
						int range = 1 + mdl->maxValue() - mdl->minValue();
						LcdSpinBox *l = new LcdSpinBox(3/*log(range)*/, dlg);
						l->setModel(ctrl.
							m_connectedModel.m_intModel);
						assert(l->model());
						wdg = l;
						break;
					}
					case PortVis::Toggled:
					{
						LedCheckBox *l = new LedCheckBox(dlg);
						l->setModel(ctrl.
							m_connectedModel.m_boolModel);
						assert(l->model());
						wdg = l;
						break;
					}
				}
				qDebug() << "WDG:" << wdg;
			}
		}
	};

	const int rowNum = 6; // just some guess for what might look good
	int wdgNum = 0;
	for (Lv2ControlBase::PortBase* port : controls->getPorts())
	{
		SetupWidget setup;
		setup.dlg = this;
		port->accept(setup);

		if(setup.wdg)
		{
			// start in row one, add widgets cell by cell
			grid->addWidget(setup.wdg,
				1 + wdgNum / rowNum, wdgNum % rowNum);
			++wdgNum;
		}
	}
}

/*
// TODO: common UI class..., as this must be usable for instruments, too
Lv2ControlDialog::~Lv2ControlDialog()
{
	Lv2Effect *model = castModel<Lv2Effect>();

	if (model && lv2Controls()->m_lv2Descriptor->ui_ext() &&
lv2Controls()->m_hasGUI)
	{
		qDebug() << "shutting down UI...";
		model->m_plugin->ui_ext_show(false);
	}
}
*/

void Lv2FxControlDialog::modelChanged()
{
	/*	// set models for controller knobs
		m_portamento->setModel( &m->m_portamentoModel ); */

	m_toggleUIButton->setChecked(lv2Controls()->hasGui());
}

void Lv2FxControlDialog::toggleUI()
{
#if 0
	Lv2Effect *model = castModel<Lv2Effect>();
	if (model->m_lv2Descriptor->ui_ext() &&
		model->m_hasGUI != m_toggleUIButton->isChecked())
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		model->m_plugin->ui_ext_show(model->m_hasGUI);
		ControllerConnection::finalizeConnections();
	}
#endif
}

void Lv2FxControlDialog::reloadPlugin() { lv2Controls()->reloadPlugin(); }
