#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QTemporaryFile>

#define OSC_USE_QLIBRARY

#ifdef OSC_USE_QLIBRARY
	#include <QLibrary>
#else
	#include <dlfcn.h>
#endif

#include "ControllerConnection.h"
#include "gui_templates.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "../include/RemotePlugin.h" // QSTR_TO_STDSTR
#include "StringPairDrag.h" // DnD
#include "oscinstrument.h"
//#include "ComboBox.h"

#include "oscplugin.h"


#include "embed.cpp"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT oscinstrument_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"OscPlugin",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Embedded Osc" ),
	"Johannes Lorenz <jlsf2013/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	"xmz",
	NULL,
} ;

}



OscInstrument::OscInstrument(InstrumentTrack * _instrumentTrack ) :
	Instrument( _instrumentTrack, &oscinstrument_plugin_descriptor ),
	m_hasGUI( false )
{
	for( int i = 0; i < NumKeys; ++i )
	 m_runningNotes[i] = 0;

	initPlugin();

	// now we need a play-handle which cares for calling play()
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this, _instrumentTrack );
	Engine::mixer()->addPlayHandle( iph );

	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( reloadPlugin() ) );

/*	connect( instrumentTrack()->pitchRangeModel(), SIGNAL( dataChanged() ),
				this, SLOT( updatePitchRange() ) );*/
}




OscInstrument::~OscInstrument()
{
	shutdownPlugin();
	Engine::mixer()->removePlayHandlesOfTypes( instrumentTrack(),
				PlayHandle::TypeNotePlayHandle
						   | PlayHandle::TypeInstrumentPlayHandle );
}

#ifndef OSC_USE_MIDI
void OscInstrument::playNote(NotePlayHandle *_n, sampleFrame *)
{
	// no idea what that means
	if( _n->isMasterNote() || ( _n->hasParent() && _n->isReleased() ) )
	{
		return;
	}

	const f_cnt_t tfp = _n->totalFramesPlayed();

	const float LOG440 = 2.643452676f;

	int midiNote = (int)floor( 12.0 * ( log2( _n->unpitchedFrequency() ) - LOG440 ) - 4.0 );

	qDebug() << "midiNote: " << midiNote << ", r? " << _n->isReleased();
	// out of range?
	if( midiNote <= 0 || midiNote >= 128 )
	{
		return;
	}

	if( tfp == 0 )
	{
		const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();

		osc_plugin->sendOsc("/noteOn", "iii", 0, midiNote, baseVelocity);

/*		SF2PluginData * pluginData = new SF2PluginData;
		pluginData->midiNote = midiNote;
		pluginData->lastPanning = 0;
		pluginData->lastVelocity = _n->midiVelocity( baseVelocity );
		pluginData->fluidVoice = NULL;
		pluginData->isNew = true;
		pluginData->offset = _n->offset();
		pluginData->noteOffSent = false;

		_n->m_pluginData = pluginData;

		// insert the nph to the playing notes vector
		m_playingNotesMutex.lock();
		m_playingNotes.append( _n );
		m_playingNotesMutex.unlock();*/
	}
	else if( _n->isReleased() && ! _n->instrumentTrack()->isSustainPedalPressed() ) // note is released during this period
	{
	/*	SF2PluginData * pluginData = static_cast<SF2PluginData *>( _n->m_pluginData );
		pluginData->offset = _n->framesBeforeRelease();
		pluginData->isNew = false;

		m_playingNotesMutex.lock();
		m_playingNotes.append( _n );
		m_playingNotesMutex.unlock();*/
		osc_plugin->sendOsc("/noteOff", "ii", 0, midiNote);
	}
	else if( _n->framesLeft() <= 0 )
	{
		osc_plugin->sendOsc("/noteOff", "ii", 0, midiNote);
	}
}
#endif



void OscInstrument::saveSettings( QDomDocument & _doc,
											QDomElement & _this )
{
	QTemporaryFile tf;
	if( tf.open() )
	{
		const std::string fn = QSTR_TO_STDSTR(
									QDir::toNativeSeparators( tf.fileName() ) );
		m_pluginMutex.lock();
		osc_plugin->sendOsc("/save-master", "s", fn.c_str());
		osc_plugin->sendOsc("/save-master", "s", "master.tmp");
		m_pluginMutex.unlock();
		sleep(2);

		QByteArray a = tf.readAll();
		QDomDocument doc( "mydoc" );
		if( doc.setContent( a ) )
		{
			QDomNode n = _doc.importNode( doc.documentElement(), true );
			_this.appendChild( n );
		}
	}
}




void OscInstrument::loadSettings( const QDomElement & _this )
{

	if( !_this.hasChildNodes() )
	{
		return;
	}

/*	m_portamentoModel.loadSettings( _this, "portamento" );
	m_filterFreqModel.loadSettings( _this, "filterfreq" );
	m_filterQModel.loadSettings( _this, "filterq" );
	m_bandwidthModel.loadSettings( _this, "bandwidth" );
	m_fmGainModel.loadSettings( _this, "fmgain" );
	m_resCenterFreqModel.loadSettings( _this, "rescenterfreq" );
	m_resBandwidthModel.loadSettings( _this, "resbandwidth" );
	m_forwardMidiCcModel.loadSettings( _this, "forwardmidicc" );
*/
	QDomDocument doc;
	QDomElement data = _this.firstChildElement( "Osc-data" );
	if( data.isNull() )
	{
		data = _this.firstChildElement();
	}
	doc.appendChild( doc.importNode( data, true ) );

	QTemporaryFile tf;
	tf.setAutoRemove( false );
	if( tf.open() )
	{
		QByteArray a = doc.toString( 0 ).toUtf8();
		tf.write( a );
		tf.flush();

		const std::string fn = QSTR_TO_STDSTR( QDir::toNativeSeparators( tf.fileName() ) );

		m_pluginMutex.lock();
		osc_plugin->sendOsc("/load-master", "s", fn.c_str());
		m_pluginMutex.unlock();

	/*	m_modifiedControllers.clear();
		for( const QString & c : _this.attribute( "modifiedcontrollers" ).split( ',' ) )
		{
			if( !c.isEmpty() )
			{
				switch( c.toInt() )
				{
					case C_portamento: updatePortamento(); break;
					case C_filtercutoff: updateFilterFreq(); break;
					case C_filterq: updateFilterQ(); break;
					case C_bandwidth: updateBandwidth(); break;
					case C_fmamp: updateFmGain(); break;
					case C_resonance_center: updateResCenterFreq(); break;
					case C_resonance_bandwidth: updateResBandwidth(); break;
					default:
						break;
				}
			}
		}*/

		emit settingsChanged();
	}

}




void OscInstrument::loadFile( const QString & _file )
{
	m_pluginMutex.lock();
	osc_plugin->sendOsc("/load-master", "s", _file.toAscii().data());
	m_pluginMutex.unlock();
/*	const std::string fn = QSTR_TO_STDSTR( _file );
	if( m_remotePlugin )
	{
		m_remotePlugin->lock();
		m_remotePlugin->sendMessage( RemotePlugin::message( IdLoadPresetFile ).addString( fn ) );
		m_remotePlugin->waitForMessage( IdLoadPresetFile );
		m_remotePlugin->unlock();
	}
	else
	{
		m_pluginMutex.lock();
		m_plugin->loadPreset( fn );
		m_pluginMutex.unlock();
	}
*/
	instrumentTrack()->setName( QFileInfo( _file ).baseName().replace( QRegExp( "^[0-9]{4}-" ), QString() ) );

//	m_modifiedControllers.clear();

	emit settingsChanged();
}




QString OscInstrument::nodeName() const
{
	return oscinstrument_plugin_descriptor.name;
}




void OscInstrument::play( sampleFrame * _buf )
{
	if(osc_plugin)
	{
		unsigned long buffersize = osc_plugin->buffersize();
		float outputl[buffersize];
		float outputr[buffersize];
m_pluginMutex.lock();
		osc_plugin->runSynth(outputl, outputr, buffersize);
m_pluginMutex.unlock();
		// TODO: move to MixHelpers
		for( int f = 0; f < buffersize; ++f )
		{
			_buf[f][0] = outputl[f];
			_buf[f][1] = outputr[f];
		}
	}

	instrumentTrack()->processAudioBuffer( _buf, Engine::mixer()->framesPerPeriod(), NULL );
}


void OscInstrument::reloadPlugin()
{
	// save state of current plugin instance
	DataFile m( DataFile::InstrumentTrackSettings );
	saveSettings( m, m.content() );

	shutdownPlugin();
	// init plugin (will create a new instance)
	initPlugin();

	// and load the settings again
	loadSettings( m.content() );
}



/*void OscInstrument::updatePitchRange()
{
	m_pluginMutex.lock();
	if( m_remotePlugin )
	{
		m_remotePlugin->sendMessage( RemotePlugin::message( IdZasfSetPitchWheelBendRange ).
											addInt( instrumentTrack()->midiPitchRange() ) );
	}
	else
	{
		m_plugin->setPitchWheelBendRange( instrumentTrack()->midiPitchRange() );
	}
	m_pluginMutex.unlock();

}*/

void OscInstrument::shutdownPlugin()
{
	delete osc_plugin;
	osc_plugin = nullptr;
	delete osc_descriptor;
	osc_descriptor = nullptr;

	m_pluginMutex.lock();
	if(lib) {
#ifdef OSC_USE_QLIBRARY
		lib->unload();
		delete lib;
		lib = nullptr;
#else
		dlclose(lib);
		lib = nullptr;
#endif
	}
	m_pluginMutex.unlock();
}

void OscInstrument::initPlugin()
{
	m_pluginMutex.lock();

	setLibraryName("/usr/local/lib/osc/libzyn-wavetables.so"); // TODO

	osc_descriptor_loader_t osc_descriptor_loader;
#ifdef OSC_USE_QLIBRARY
	lib = new QLibrary(libraryName);
	lib->load();

	if(!lib->isLoaded())
	 qDebug() << "Warning: Could not load library " << libraryName << ": " << lib->errorString();

	osc_descriptor_loader =
		(osc_descriptor_loader_t) lib->resolve("osc_descriptor");
#else
	lib = dlopen(libraryName.toAscii().data(), RTLD_LAZY | RTLD_LOCAL);
	if(!lib)
	 qDebug() << "Warning: Could not load library " << libraryName << ": " << strerror(errno);

	*(void **) (&osc_descriptor_loader) = dlsym(lib, "osc_descriptor");
#endif


	if(!osc_descriptor_loader)
	 qDebug() << "Warning: Could not resolve \"osc_descriptor\" in " << libraryName;

	if(osc_descriptor_loader)
	{
		osc_descriptor = (*osc_descriptor_loader)(0 /* = plugin number, TODO */);
		if(osc_descriptor)
		 osc_plugin = osc_descriptor->instantiate(Engine::mixer()->processingSampleRate());
	}
	m_pluginMutex.unlock();
/*	m_pluginMutex.lock();
	delete m_plugin;
	delete m_remotePlugin;
	m_plugin = NULL;
	m_remotePlugin = NULL;

	if( m_hasGUI )
	{
		m_remotePlugin = new OscRemotePlugin();
		m_remotePlugin->lock();
		m_remotePlugin->waitForInitDone( false );

		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdZasfLmmsWorkingDirectory ).
				addString(
					QSTR_TO_STDSTR(
						QString( ConfigManager::inst()->workingDir() ) ) ) );
		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdZasfPresetDirectory ).
				addString(
					QSTR_TO_STDSTR(
						QString( ConfigManager::inst()->factoryPresetsDir() +
								QDir::separator() + "Osc" ) ) ) );

		m_remotePlugin->updateSampleRate( Engine::mixer()->processingSampleRate() );

		// temporary workaround until the VST synchronization feature gets stripped out of the RemotePluginClient class
		// causing not to send buffer size information requests
		m_remotePlugin->sendMessage( RemotePlugin::message( IdBufferSizeInformation ).addInt( Engine::mixer()->framesPerPeriod() ) );

		m_remotePlugin->showUI();
		m_remotePlugin->unlock();
	}
	else
	{
		m_plugin = new LocalOsc;
		m_plugin->setSampleRate( Engine::mixer()->processingSampleRate() );
		m_plugin->setBufferSize( Engine::mixer()->framesPerPeriod() );
	}

	m_pluginMutex.unlock();
*/
}

#ifdef OSC_USE_MIDI
bool OscInstrument::handleMidiEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
/*	MidiEvent localEvent = event;
	localEvent.setChannel( 0 );
	m_pluginMutex.lock();
	if( m_remotePlugin )
	{
		m_remotePlugin->processMidiEvent( localEvent, 0 );
	}
	else
	{
		m_plugin->processMidiEvent( localEvent );
	}
	m_pluginMutex.unlock();*/
	switch(event.type())
	{
		// the old zynaddsubfx plugin always uses channel 0
		case MidiNoteOn:
			if( event.velocity() > 0 )
			{
				if( event.key() <= 0 || event.key() >= 128 )
				{
					break;
				}
				if( m_runningNotes[event.key()] > 0 )
				{
					m_pluginMutex.lock();
					osc_plugin->sendOsc("/noteOff", "ii", 0, event.key());
					m_pluginMutex.unlock();
				}
				++m_runningNotes[event.key()];
				m_pluginMutex.lock();
				osc_plugin->sendOsc("/noteOn", "iii", 0, event.key(), event.velocity());
				m_pluginMutex.unlock();
				break;
			}
		case MidiNoteOff:
			if( event.key() > 0 && event.key() < 128 )
			if( --m_runningNotes[event.key()] <= 0 )
			{
				m_pluginMutex.lock();
				osc_plugin->sendOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
			break;
	/*              case MidiPitchBend:
			m_master->SetController( event.channel(), C_pitchwheel, event.pitchBend()-8192 );
			break;
		case MidiControlChange:
			m_master->SetController( event.channel(), midiIn.getcontroller( event.controllerNumber() ), event.controllerValue() );
			break;*/
		default:
			break;

	}

	return true;
}
#endif

PluginView * OscInstrument::instantiateView( QWidget * _parent )
{
	return new OscView( this, _parent );
}


OscView::OscView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
/*	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );*/

	QGridLayout * l = new QGridLayout( this );

/*
	l->setContentsMargins( 20, 80, 10, 10 );
	l->setVerticalSpacing( 16 );
	l->setHorizontalSpacing( 10 );

	m_portamento = new Knob( knobBright_26, this );
	m_portamento->setHintText( tr( "Portamento:" ), "" );
	m_portamento->setLabel( tr( "PORT" ) );

	m_filterFreq = new Knob( knobBright_26, this );
	m_filterFreq->setHintText( tr( "Filter Frequency:" ), "" );
	m_filterFreq->setLabel( tr( "FREQ" ) );

	m_filterQ = new Knob( knobBright_26, this );
	m_filterQ->setHintText( tr( "Filter Resonance:" ), "" );
	m_filterQ->setLabel( tr( "RES" ) );

	m_bandwidth = new Knob( knobBright_26, this );
	m_bandwidth->setHintText( tr( "Bandwidth:" ), "" );
	m_bandwidth->setLabel( tr( "BW" ) );

	m_fmGain = new Knob( knobBright_26, this );
	m_fmGain->setHintText( tr( "FM Gain:" ), "" );
	m_fmGain->setLabel( tr( "FM GAIN" ) );

	m_resCenterFreq = new Knob( knobBright_26, this );
	m_resCenterFreq->setHintText( tr( "Resonance center frequency:" ), "" );
	m_resCenterFreq->setLabel( tr( "RES CF" ) );

	m_resBandwidth = new Knob( knobBright_26, this );
	m_resBandwidth->setHintText( tr( "Resonance bandwidth:" ), "" );
	m_resBandwidth->setLabel( tr( "RES BW" ) );

	m_forwardMidiCC = new LedCheckBox( tr( "Forward MIDI Control Changes" ), this );
*/
	m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
	m_toggleUIButton->setCheckable( true );
	m_toggleUIButton->setChecked( false );
	m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleUIButton->setFont( pointSize<8>( m_toggleUIButton->font() ) );
	connect( m_toggleUIButton, SIGNAL( toggled( bool ) ), this,
							SLOT( toggleUI() ) );
	m_toggleUIButton->setWhatsThis(
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of Osc." ) );

//	m_pluginTypeEdit = new QLineEdit( tr( "Plugin Type"), this );
//	m_branchEdit = new QLineEdit( tr( "Branch" ), this );

	m_reloadPluginButton = new QPushButton( tr( "Reload Plugin" ), this );

	connect( m_reloadPluginButton, SIGNAL( toggled( bool ) ), this,
							SLOT( reloadPlugin() ) );

	l->addWidget( m_toggleUIButton, 0, 0 );
	l->addWidget( m_reloadPluginButton, 0, 1 );

/*	m_zoomingComboBox = new ComboBox( this );
	m_zoomingComboBox->setModel(model()->m_);

	connect( m_pluginTypeCombo->model(), SIGNAL(dataChanged()), this,
		SLOT(onPluginTypeComboChanged()) );*/

/*
	l->addWidget( m_toggleUIButton, 0, 0, 1, 4 );
	l->setRowStretch( 1, 5 );
	l->addWidget( m_portamento, 2, 0 );
	l->addWidget( m_filterFreq, 2, 1 );
	l->addWidget( m_filterQ, 2, 2 );
	l->addWidget( m_bandwidth, 2, 3 );
	l->addWidget( m_fmGain, 3, 0 );
	l->addWidget( m_resCenterFreq, 3, 1 );
	l->addWidget( m_resBandwidth, 3, 2 );
	l->addWidget( m_forwardMidiCC, 4, 0, 1, 4 );

	l->setRowStretch( 5, 10 );
	l->setColumnStretch( 4, 10 );*/

	setAcceptDrops( true );
}





OscView::~OscView()
{
//	TODO: we should hide the UI here, but this doesn't work currently
//	if(castModel<OscInstrument>()->m_hasGUI)
//	 toggleUI();
}




void OscView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( StringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						StringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == "pluginpresetfile" )
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}
}




void OscView::dropEvent( QDropEvent * _de )
{
	const QString type = StringPairDrag::decodeKey( _de );
	const QString value = StringPairDrag::decodeValue( _de );
	if( type == "pluginpresetfile" )
	{
		castModel<OscInstrument>()->loadFile( value );
		_de->accept();
		return;
	}
	_de->ignore();
}

/*void OscView::onPluginTypeComboChanged()
{
	m_branchCombo->model()->clear();
	QString text = ComboBox.model()->currentText();
	text += '-';

	QDir oscdir("/usr/local/lib/osc/");
	const QStringList plugins = oscdir.entryList(QDir::NoFilter, QDir::Name);
	for(QString& dir : plugins)
	{
		int idx = dir.indexOf(text);
		if(idx == 0)
		{
			m_branchCombo->model()->addItem(
				end.midRef(dir.length() - text.length() - 4); // "-", ".so"
			);
		}
	}
}*/




void OscView::modelChanged()
{
	OscInstrument * m = castModel<OscInstrument>();

/*	// set models for controller knobs
	m_portamento->setModel( &m->m_portamentoModel );
	m_filterFreq->setModel( &m->m_filterFreqModel );
	m_filterQ->setModel( &m->m_filterQModel );
	m_bandwidth->setModel( &m->m_bandwidthModel );
	m_fmGain->setModel( &m->m_fmGainModel );
	m_resCenterFreq->setModel( &m->m_resCenterFreqModel );
	m_resBandwidth->setModel( &m->m_resBandwidthModel );

	m_forwardMidiCC->setModel( &m->m_forwardMidiCcModel ); */

	m_toggleUIButton->setChecked( m->m_hasGUI );
}




void OscView::toggleUI()
{
	OscInstrument * model = castModel<OscInstrument>();
	if( model->m_hasGUI != m_toggleUIButton->isChecked() )
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();

		model->osc_plugin->sendOsc(model->m_hasGUI
			? "/show-ui"
			: "/hide-ui", "");


//		model->reloadPlugin();

		ControllerConnection::finalizeConnections();
	}
}

void OscView::reloadPlugin()
{
	OscInstrument * model = castModel<OscInstrument>();
	model->reloadPlugin();
}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{

	return new OscInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}

