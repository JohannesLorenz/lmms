/*
 * Lv2Instrument.cpp - implementation of LV2 instrument
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

#include "Lv2Instrument.h"

#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QTemporaryFile>
#include <lv2.h>

#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "Lv2SubPluginFeatures.h"
#include "StringPairDrag.h" // DnD
#include "gui_templates.h"
#include "embed.h"
#include "plugin_export.h"

Plugin::Descriptor PLUGIN_EXPORT lv2instrument_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"LV2",
	QT_TRANSLATE_NOOP("Lv2Instrument",
		"plugin for using arbitrary LV2 instruments inside LMMS."),
	"Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	new Lv2SubPluginFeatures(Plugin::Instrument)
};

DataFile::Types Lv2Instrument::settingsType()
{
	return DataFile::InstrumentTrackSettings;
}

void Lv2Instrument::setNameFromFile(const QString &name)
{
	instrumentTrack()->setName(name);
}

Lv2Instrument::Lv2Instrument(InstrumentTrack *instrumentTrackArg,
	Descriptor::SubPluginFeatures::Key *key) :
	Instrument(instrumentTrackArg, &lv2instrument_plugin_descriptor, key),
	Lv2ControlBase(key->attributes["plugin"])
{
#ifdef LV2_INSTRUMENT_USE_MIDI
	for (int i = 0; i < NumKeys; ++i) {
		m_runningNotes[i] = 0;
	}
#endif
	if (m_plugin)
	{
		connect(instrumentTrack()->pitchRangeModel(), SIGNAL(dataChanged()),
			this, SLOT(updatePitchRange()));
		connect(Engine::mixer(), SIGNAL(sampleRateChanged()),
			this, SLOT(reloadPlugin())); // TODO: refactor to Lv2ControlBase?

		// now we need a play-handle which cares for calling play()
		InstrumentPlayHandle *iph =
			new InstrumentPlayHandle(this, instrumentTrackArg);
		Engine::mixer()->addPlayHandle(iph);
	}
}

Lv2Instrument::~Lv2Instrument()
{
	Engine::mixer()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::TypeNotePlayHandle |
						  PlayHandle::TypeInstrumentPlayHandle);
}

void Lv2Instrument::saveSettings(QDomDocument &doc, QDomElement &that)
{
	Lv2ControlBase::saveSettings(doc, that);
}

void Lv2Instrument::loadSettings(const QDomElement &that)
{
	Lv2ControlBase::loadSettings(that);
}

// not yet working
#ifndef LV2_INSTRUMENT_USE_MIDI
void Lv2Instrument::playNote(NotePlayHandle *nph, sampleFrame *)
{
	// no idea what that means
	if (nph->isMasterNote() || (nph->hasParent() && nph->isReleased()))
	{
		return;
	}

	const f_cnt_t tfp = nph->totalFramesPlayed();

	const float LOG440 = 2.643452676f;

	int midiNote = (int)floor(
		12.0 * (log2(nph->unpitchedFrequency()) - LOG440) - 4.0);

	qDebug() << "midiNote: " << midiNote << ", r? " << nph->isReleased();
	// out of range?
	if (midiNote <= 0 || midiNote >= 128)
	{
		return;
	}

	if (tfp == 0)
	{
		const int baseVelocity =
			instrumentTrack()->midiPort()->baseVelocity();
		m_plugin->send_osc("/noteOn", "iii", 0, midiNote, baseVelocity);
	}
	else if (nph->isReleased() &&
		!nph->instrumentTrack()
			 ->isSustainPedalPressed()) // note is released during
						    // this period
	{
		m_plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
	else if (nph->framesLeft() <= 0)
	{
		m_plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
}
#endif

void Lv2Instrument::play(sampleFrame *buf)
{
	if (m_plugin)
	{
		// always >0, so the cast is OK
		unsigned fpp = static_cast<unsigned>(
			Engine::mixer()->framesPerPeriod());

	//	m_pluginMutex.lock();
		m_instance->run(Engine::mixer()->processingSampleRate());
	//	m_pluginMutex.unlock();

		for (std::size_t f = 0; f < fpp; ++f)
		{
			buf[f][0] = outPorts.left->m_data.m_audioData.buffer[f];
			buf[f][1] = outPorts.right->m_data.m_audioData.buffer[f];
		}
	}
	instrumentTrack()->processAudioBuffer(
		buf, Engine::mixer()->framesPerPeriod(), nullptr);
}

void Lv2Instrument::updatePitchRange()
{
	qDebug() << "Lmms: Cannot update pitch range for lv2 plugin:"
		    "not implemented yet";
}

QString Lv2Instrument::nodeName() const
{
	return Lv2ControlBase::nodeName();
}

#ifdef LV2_INSTRUMENT_USE_MIDI
bool Lv2Instrument::handleMidiEvent(
	const MidiEvent &event, const MidiTime &time, f_cnt_t offset)
{
	(void)time;
	(void)offset;
#ifdef TODO
	switch (event.type())
	{
	// the old zynaddsubfx plugin always uses channel 0
	case MidiNoteOn:
		if (event.velocity() > 0)
		{
			if (event.key() <= 0 || event.key() >= 128)
			{
				break;
			}
			if (m_runningNotes[event.key()] > 0)
			{
				m_pluginMutex.lock();
				writeOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
			++m_runningNotes[event.key()];
			m_pluginMutex.lock();
			writeOsc("/noteOn", "iii", 0, event.key(),
				event.velocity());
			m_pluginMutex.unlock();
		}
		break;
	case MidiNoteOff:
		if (event.key() > 0 && event.key() < 128) {
			if (--m_runningNotes[event.key()] <= 0)
			{
				m_pluginMutex.lock();
				writeOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
		}
		break;
		/*              case MidiPitchBend:
				m_master->SetController( event.channel(),
		   C_pitchwheel, event.pitchBend()-8192 ); break; case
		   MidiControlChange: m_master->SetController( event.channel(),
					midiIn.getcontroller(
		   event.controllerNumber() ), event.controllerValue() );
				break;*/
	default:
		break;
	}
#else
	(void)event;
#endif
	return true;
}
#endif

PluginView *Lv2Instrument::instantiateView(QWidget *parent)
{
	return new Lv2InsView(this, parent);
}




Lv2InsView::Lv2InsView(Instrument *_instrument, QWidget *_parent) :
	InstrumentView(_instrument, _parent)
{
	setAutoFillBackground(true);

	QGridLayout *l = new QGridLayout(this);

	m_toggleUIButton = new QPushButton(tr("Show GUI"), this);
	m_toggleUIButton->setCheckable(true);
	m_toggleUIButton->setChecked(false);
	m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
	m_toggleUIButton->setFont(pointSize<8>(m_toggleUIButton->font()));
	connect(m_toggleUIButton, SIGNAL(toggled(bool)), this,
		SLOT(toggleUI()));
	m_toggleUIButton->setWhatsThis(
		tr("Click here to show or hide the graphical user interface "
		   "(GUI) of LV2."));

	m_reloadPluginButton = new QPushButton(tr("Reload Plugin"), this);

	connect(m_reloadPluginButton, SIGNAL(toggled(bool)), this,
		SLOT(reloadPlugin()));

	l->addWidget(m_toggleUIButton, 0, 0);
	l->addWidget(m_reloadPluginButton, 0, 1);

	setAcceptDrops(true);
}

Lv2InsView::~Lv2InsView()
{
	Lv2Instrument *model = castModel<Lv2Instrument>();
	if (model && false /* TODO: check if plugin has UI extension */ && model->m_hasGUI)
	{
		qDebug() << "shutting down UI...";
		// TODO: tell plugin to hide the UI
	}
}

void Lv2InsView::dragEnterEvent(QDragEnterEvent *_dee)
{
	void (QDragEnterEvent::*reaction)(void) = &QDragEnterEvent::ignore;

	if (_dee->mimeData()->hasFormat(StringPairDrag::mimeType()))
	{
		const QString txt =
			_dee->mimeData()->data(StringPairDrag::mimeType());
		if (txt.section(':', 0, 0) == "pluginpresetfile") {
			reaction = &QDragEnterEvent::acceptProposedAction;
		}
	}

	(_dee->*reaction)();
}

void Lv2InsView::dropEvent(QDropEvent *_de)
{
	const QString type = StringPairDrag::decodeKey(_de);
	const QString value = StringPairDrag::decodeValue(_de);
	if (type == "pluginpresetfile")
	{
		castModel<Lv2Instrument>()->loadFile(value);
		_de->accept();
		return;
	}
	_de->ignore();
}

void Lv2InsView::modelChanged()
{
	Lv2Instrument *m = castModel<Lv2Instrument>();

	/*	// set models for controller knobs
		m_portamento->setModel( &m->m_portamentoModel ); */

	m_toggleUIButton->setChecked(m->m_hasGUI);
}

void Lv2InsView::toggleUI()
{
	Lv2Instrument *model = castModel<Lv2Instrument>();
	if (false /* TODO: check if plugin has the UI extension */ &&
		model->m_hasGUI != m_toggleUIButton->isChecked())
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		// TODO: show the UI
		ControllerConnection::finalizeConnections();
	}
}

void Lv2InsView::reloadPlugin()
{
	Lv2Instrument *model = castModel<Lv2Instrument>();
	model->reloadPlugin();
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	return new Lv2Instrument(static_cast<InstrumentTrack*>(_parent),
		static_cast<KeyType*>(_data ));
}

}
