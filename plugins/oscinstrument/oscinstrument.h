#ifndef OSCINSTRUMENT_H
#define OSCINSTRUMENT_H

#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QMap>

#ifdef USE_MIDI
#include <QMutex>
#endif

#include "DataFile.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"

#define OSC_USE_MIDI
#define OSC_USE_QLIBRARY

class OscInstrument : public Instrument
{
	Q_OBJECT
public:
	OscInstrument( InstrumentTrack * _instrument_track );
	virtual ~OscInstrument();

#ifdef OSC_USE_MIDI
	virtual bool handleMidiEvent( const MidiEvent& event, const MidiTime& time = MidiTime(), f_cnt_t offset = 0 );
#else
	virtual void playNote(NotePlayHandle *_n, sampleFrame *);
#endif
	virtual void play( sampleFrame * _working_buffer );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void loadFile( const QString & _file );


	virtual QString nodeName() const;

	virtual Flags flags() const
	{
#ifdef OSC_USE_MIDI
		return IsSingleStreamed | IsMidiBased;
#else
		return IsSingleStreamed;
#endif
	}

	virtual PluginView * instantiateView( QWidget * _parent );

	void setLibraryName(const QString& name) { libraryName = name; }


private slots:
	void reloadPlugin();

private:

#ifdef OSC_USE_MIDI
	QMutex m_pluginMutex;
	int m_runningNotes[NumKeys];
#endif
	char oscbuffer[256];

	const class OscDescriptor* osc_descriptor = nullptr;
	class OscPlugin* osc_plugin = nullptr;

	void initPlugin();
	void shutdownPlugin();

	bool m_hasGUI;
//	QMutex m_pluginMutex;

	friend class OscView;

	bool loaded;
#ifdef OSC_USE_QLIBRARY
	class QLibrary* lib = nullptr;
#else
	void* lib = nullptr; //!< dlopen() handle
#endif
	QString libraryName;

//	class ComboBoxModel* m_pluginTypeCombo;
//	class ComboBoxModel* m_branchCombo;

signals:
	void settingsChanged();


} ;



class OscView : public InstrumentView
{
	Q_OBJECT
public:
	OscView( Instrument * _instrument, QWidget * _parent );
	virtual ~OscView();


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );

private:
	void modelChanged();

	QPushButton * m_toggleUIButton;

	//QLineEdit * m_pluginTypeEdit;
	//QLineEdit * m_branchEdit;
//	class ComboBox* m_pluginTypeCombo;
//	class ComboBox* m_branchCombo;

	QPushButton * m_reloadPluginButton;

private slots:
	void toggleUI();
	void reloadPlugin();
	void onPluginTypeComboChanged();

} ;

#endif // OSCINSTRUMENT_H
