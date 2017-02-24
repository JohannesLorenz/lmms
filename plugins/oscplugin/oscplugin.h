#ifndef OSCPLUGIN_H
#define OSCPLUGIN_H

#include <QPushButton>
#include <QString>
#include <QMap>

#include "DataFile.h"
#include "Instrument.h"
#include "InstrumentView.h"

class OscInstrument : public Instrument
{
	Q_OBJECT
public:
	OscInstrument( InstrumentTrack * _instrument_track );
	virtual ~OscInstrument();

	virtual void play( sampleFrame * _working_buffer );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void loadFile( const QString & _file );


	virtual QString nodeName() const;

	virtual Flags flags() const
	{
		return IsSingleStreamed;
	}

	virtual PluginView * instantiateView( QWidget * _parent );


private slots:
	void reloadPlugin();

private:
	void initPlugin();

	bool m_hasGUI;
//	QMutex m_pluginMutex;

	friend class OscView;

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

private slots:
	void toggleUI();

} ;

#endif // OSCPLUGIN_H
