/*
 * Lv2ControlDialog.h - control dialog for amplifier effect
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

#ifndef LV2_FX_CONTROL_DIALOG_H
#define LV2_FX_CONTROL_DIALOG_H

#include "EffectControlDialog.h"

class Lv2FxControlDialog : public EffectControlDialog
{
	Q_OBJECT

	class QPushButton *m_toggleUIButton;
	class QPushButton *m_reloadPluginButton;

	class Lv2FxControls *lv2Controls();
	void modelChanged() override;

public:
	Lv2FxControlDialog(class Lv2FxControls *controls);
	virtual ~Lv2FxControlDialog() override {}

private slots:
	void toggleUI();
	void reloadPlugin();
};

#endif
