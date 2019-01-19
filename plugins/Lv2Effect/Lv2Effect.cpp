/*
 * Lv2Effect.cpp - implementation of LV2 effect
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

#include "Lv2Effect.h"

#include <QDebug>
#include <lv2.h>

#include "AutomatableModel.h"
#include "Lv2FxControlDialog.h"
#include "Lv2SubPluginFeatures.h"
#include "embed.h"
#include "plugin_export.h"

Plugin::Descriptor PLUGIN_EXPORT lv2effect_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"LV2",
	QT_TRANSLATE_NOOP("Lv2Effect",
		"plugin for using arbitrary LV2-effects inside LMMS."),
	"Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	new Lv2SubPluginFeatures(Plugin::Effect)
};

Lv2Effect::Lv2Effect(Model* parent, const Descriptor::SubPluginFeatures::Key *key) :
	Effect(&lv2effect_plugin_descriptor, parent, key),
	m_controls(this, key->attributes["uri"])
{
	qDebug() << "Constructing LV2 effect";
}

Lv2Effect::~Lv2Effect()
{
}

bool Lv2Effect::processAudioBuffer(sampleFrame *buf, const fpp_t frames)
{
	if(!isEnabled() || !isRunning())
	{
		return false;
	}

	Lv2FxControls& ctrl = m_controls;

	ctrl.inPorts().left->copyBuffersFromLmms(buf, 0, frames);
	ctrl.inPorts().right->copyBuffersFromLmms(buf, 1, frames);

	m_controls.copyModelsFromLmms();

//	m_pluginMutex.lock();
	ctrl.run(static_cast<unsigned>(frames));
//	m_pluginMutex.unlock();

	ctrl.outPorts().left->copyBuffersToLmms(buf, 0, frames);
	ctrl.outPorts().right->copyBuffersToLmms(buf, 1, frames);

	return isRunning();
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	return new Lv2Effect(_parent, static_cast<const KeyType*>(_data));
}

}
