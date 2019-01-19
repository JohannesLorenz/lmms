/*
 * Lv2Instrument.h - implementation of LV2 instrument
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

#ifndef LV2_INSTRUMENT_H
#define LV2_INSTRUMENT_H

#include <QMap>
#include <QString>
#include <memory>

#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"
#include "Lv2ControlBase.h"

// whether to use MIDI vs playHandle
// currently only MIDI works
#define LV2_INSTRUMENT_USE_MIDI

class QPushButton;

class Lv2Instrument : public Instrument, public Lv2ControlBase
{
	Q_OBJECT

	DataFile::Types settingsType() override;
	void setNameFromFile(const QString &name) override;

public:
	Lv2Instrument(InstrumentTrack *instrumentTrackArg,
		 Descriptor::SubPluginFeatures::Key* key);
	~Lv2Instrument() override;

	void saveSettings(QDomDocument &doc, QDomElement &that) override;  // XXX
	void loadSettings(const QDomElement &that) override; // XXX
	void loadFile(const QString &file) override { // XXX
		Lv2ControlBase::loadFile(file); }

#ifdef LV2_INSTRUMENT_USE_MIDI
	bool handleMidiEvent(const MidiEvent &event,
		const MidiTime &time = MidiTime(), f_cnt_t offset = 0) override; // XXX
#else
	void playNote(NotePlayHandle *nph, sampleFrame *) override; // XXX
#endif
	void play(sampleFrame *buf) override; // XXX

	Flags flags() const override
	{
#ifdef LV2_INSTRUMENT_USE_MIDI
		return IsSingleStreamed | IsMidiBased;
#else
		return IsSingleStreamed;
#endif
	}

	PluginView *instantiateView(QWidget *parent) override;

private slots:
	void updatePitchRange();
	void reloadPlugin() { Lv2ControlBase::reloadPlugin(); }

private:
#ifdef LV2_INSTRUMENT_USE_MIDI
	int m_runningNotes[NumKeys];
#endif
	friend class Lv2InsView;
	QString nodeName() const override;
};

class Lv2InsView : public InstrumentView
{
	Q_OBJECT
public:
	Lv2InsView(Instrument *_instrument, QWidget *_parent);
	virtual ~Lv2InsView();

protected:
	virtual void dragEnterEvent(QDragEnterEvent *_dee);
	virtual void dropEvent(QDropEvent *_de);

private:
	void modelChanged();

	QPushButton *m_toggleUIButton;
	QPushButton *m_reloadPluginButton;

private slots:
	void toggleUI();
	void reloadPlugin();
};

#endif // LV2_INSTRUMENT_H
