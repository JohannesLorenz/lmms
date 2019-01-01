/*
 * SpaInstrument.cpp - implementation of SPA instrument
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

#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QTemporaryFile>

#include <spa/audio.h>

#include "../include/RemotePlugin.h" // QSTR_TO_STDSTR
#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "SpaSubPluginFeatures.h"
#include "StringPairDrag.h" // DnD
#include "gui_templates.h"
#include "embed.h"
#include "plugin_export.h"

#include "SpaInstrument.h"

Plugin::Descriptor PLUGIN_EXPORT spainstrument_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"SPA",
	QT_TRANSLATE_NOOP("SpaInstrument",
		"plugin for using arbitrary SPA instruments inside LMMS."),
	"Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	new SpaSubPluginFeatures(Plugin::Instrument)
};

DataFile::Types SpaInstrument::settingsType()
{
	return DataFile::InstrumentTrackSettings;
}

void SpaInstrument::setNameFromFile(const QString &name)
{
	instrumentTrack()->setName(name);
}

SpaInstrument::SpaInstrument(InstrumentTrack *instrumentTrackArg,
	Descriptor::SubPluginFeatures::Key *key) :
	Instrument(instrumentTrackArg, &spainstrument_plugin_descriptor, key),
	SpaControlBase(key->attributes["plugin"])
{
#ifdef SPA_INSTRUMENT_USE_MIDI
	for (int i = 0; i < NumKeys; ++i) {
		m_runningNotes[i] = 0;
	}
#endif
	if (m_plugin)
	{
		connect(instrumentTrack()->pitchRangeModel(), SIGNAL(dataChanged()),
			this, SLOT(updatePitchRange()));
		connect(Engine::mixer(), SIGNAL(sampleRateChanged()),
			this, SLOT(reloadPlugin())); // TODO: refactor to SpaControlBase?

		// now we need a play-handle which cares for calling play()
		InstrumentPlayHandle *iph =
			new InstrumentPlayHandle(this, instrumentTrackArg);
		Engine::mixer()->addPlayHandle(iph);
	}
}

SpaInstrument::~SpaInstrument()
{
	Engine::mixer()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::TypeNotePlayHandle |
						  PlayHandle::TypeInstrumentPlayHandle);
}

void SpaInstrument::saveSettings(QDomDocument &doc, QDomElement &that)
{
	SpaControlBase::saveSettings(doc, that);
}

void SpaInstrument::loadSettings(const QDomElement &that)
{
	SpaControlBase::loadSettings(that);
}

// not yet working
#ifndef SPA_INSTRUMENT_USE_MIDI
void SpaInstrument::playNote(NotePlayHandle *nph, sampleFrame *)
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

void SpaInstrument::play(sampleFrame *buf)
{
	if (m_plugin)
	{
		m_pluginMutex.lock();
		m_ports.samplecount = m_ports.buffersize;
		m_plugin->run();
		m_pluginMutex.unlock();
		for (std::size_t f = 0; f < m_ports.buffersize; ++f)
		{
			buf[f][0] = m_ports.m_lProcessed[f];
			buf[f][1] = m_ports.m_rProcessed[f];
		}
	}
	instrumentTrack()->processAudioBuffer(
		buf, Engine::mixer()->framesPerPeriod(), nullptr);
}

void SpaInstrument::updatePitchRange()
{
	qDebug() << "Lmms: Cannot update pitch range for spa plugin:"
		    "not implemented yet";
}

QString SpaInstrument::nodeName() const
{
	return SpaControlBase::nodeName();
}

#ifdef SPA_INSTRUMENT_USE_MIDI
bool SpaInstrument::handleMidiEvent(
	const MidiEvent &event, const MidiTime &time, f_cnt_t offset)
{
	(void)time;
	(void)offset;
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

	return true;
}
#endif

PluginView *SpaInstrument::instantiateView(QWidget *parent)
{
	return new SpaInsView(this, parent);
}

void SpaInstrument::writeOsc(const char *dest, const char *args, va_list va)
{
	writeOscInternal(dest, args, va);
}

void SpaInstrument::writeOsc(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOsc(dest, args, va);
	va_end(va);
}

unsigned SpaInstrument::netPort() const
{
	return m_plugin->net_port();
}

AutomatableModel *SpaInstrument::modelAtPort(const QString &dest)
{
	return SpaControlBase::modelAtPort(dest);
}

SpaInsView::SpaInsView(Instrument *_instrument, QWidget *_parent) :
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
		   "(GUI) of SPA."));

	m_reloadPluginButton = new QPushButton(tr("Reload Plugin"), this);

	connect(m_reloadPluginButton, SIGNAL(toggled(bool)), this,
		SLOT(reloadPlugin()));

	l->addWidget(m_toggleUIButton, 0, 0);
	l->addWidget(m_reloadPluginButton, 0, 1);

	setAcceptDrops(true);
}

SpaInsView::~SpaInsView()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	if (model && model->m_spaDescriptor->ui_ext() && model->m_hasGUI)
	{
		qDebug() << "shutting down UI...";
		model->m_plugin->ui_ext_show(false);
	}
}

void SpaInsView::dragEnterEvent(QDragEnterEvent *_dee)
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

void SpaInsView::dropEvent(QDropEvent *_de)
{
	const QString type = StringPairDrag::decodeKey(_de);
	const QString value = StringPairDrag::decodeValue(_de);
	if (type == "pluginpresetfile")
	{
		castModel<SpaInstrument>()->loadFile(value);
		_de->accept();
		return;
	}
	_de->ignore();
}

void SpaInsView::modelChanged()
{
	SpaInstrument *m = castModel<SpaInstrument>();

	/*	// set models for controller knobs
		m_portamento->setModel( &m->m_portamentoModel ); */

	m_toggleUIButton->setChecked(m->m_hasGUI);
}

void SpaInsView::toggleUI()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	if (model->m_spaDescriptor->ui_ext() &&
		model->m_hasGUI != m_toggleUIButton->isChecked())
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		model->m_plugin->ui_ext_show(model->m_hasGUI);
		ControllerConnection::finalizeConnections();
	}
}

void SpaInsView::reloadPlugin()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	model->reloadPlugin();
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	return new SpaInstrument(static_cast<InstrumentTrack*>(_parent),
		static_cast<KeyType*>(_data ));
}

}
