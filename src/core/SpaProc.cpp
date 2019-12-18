

#include "../../include/SpaProc.h"

#include <QDebug>
#include <QDir>
#include <QTemporaryFile>
#include <QUrl>
#include <cmath>
#include <spa/spa.h>
#include <spa/audio.h>

#include "AutomatableModel.h"
#include "Engine.h"
#include "Mixer.h"
#include "SpaOscModel.h"

SpaProc::SpaProc(Model *parent, const spa::descriptor* desc, DataFile::Types settingsType) :
	LinkedModelGroup(parent),
	m_spaDescriptor(desc),
	m_ports(Engine::mixer()->framesPerPeriod()),
//	writeOscInUse(ATOMIC_FLAG_INIT), // not MSVC-compatible, workaround below
	m_settingsType(settingsType)
{
	m_writeOscInUse.clear(); // workaround
	initPlugin();
}

SpaProc::~SpaProc() { shutdownPlugin(); }




void SpaProc::saveState(QDomDocument &doc, QDomElement &that)
{
	if (m_spaDescriptor->save_has())
	{
		QTemporaryFile tf;
		if (tf.open())
		{
			const QString fn = QDir::toNativeSeparators(tf.fileName());
//			m_pluginMutex.lock();
			m_plugin->save(fn.toUtf8().data(), ++m_saveTicket);
//			m_pluginMutex.unlock();

			while (!m_plugin->save_check(fn.toUtf8().data(), m_saveTicket)) {
				QThread::msleep(1);
			}

			QDomCDATASection cdata = doc.createCDATASection(
				QString::fromUtf8(tf.readAll()));
			that.appendChild(cdata);
		}
		tf.remove();
	}
}

void SpaProc::loadState(const QDomElement &that)
{
	if (!that.hasChildNodes())
	{
		return;
	}

	for (QDomNode node = that.firstChild(); !node.isNull();
		node = node.nextSibling())
	{
		QDomCDATASection cdata = node.toCDATASection();
		QDomElement elem;
		// load internal state?
		if (!cdata.isNull() && m_spaDescriptor->load_has())
		{
			QTemporaryFile tf;
			tf.setAutoRemove(false);
			if (tf.open())
			{
				tf.write(cdata.data().toUtf8());
				tf.flush();
				loadFile(tf.fileName());
			}
		}
	}
}


void SpaProc::loadFile(const QString &file)
{
	const QByteArray fn = file.toUtf8();
//	m_pluginMutex.lock();
	m_plugin->load(fn.data(), ++m_saveTicket);
	while (!m_plugin->load_check(fn.data(), m_saveTicket)) {
		QThread::msleep(1);
	}
//	m_pluginMutex.unlock();
}

void SpaProc::reloadPlugin()
{
	// refresh ports that are only read on restore
	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	int16_t fpp = Engine::mixer()->framesPerPeriod();
	assert(fpp >= 0);
	m_ports.buffersize = static_cast<unsigned>(fpp);

	if (m_spaDescriptor->restore_has())
	{
		// use the offered restore function
//		m_pluginMutex.lock();
		m_plugin->restore(++m_restoreTicket);
//		m_pluginMutex.unlock();

		while (!m_plugin->restore_check(m_restoreTicket)) {
			QThread::msleep(1);
		}
	}
	else
	{
		// save state of current plugin instance
		DataFile m(m_settingsType);
		saveState(m, m.content());

		shutdownPlugin();
		initPlugin(); // (will create a new instance)

		// and load the settings again
		loadState(m.content());
	}
}


void SpaProc::copyModelsToPorts()
{
	for (LmmsPorts::TypedPorts &tp : m_ports.m_userPorts)
	{
		switch (tp.m_type)
		{
		case 'f':
			tp.m_val.m_f = tp.m_connectedModel.m_floatModel->value();
			break;
		case 'i':
			tp.m_val.m_i = tp.m_connectedModel.m_intModel->value();
			break;
		case 'b':
			tp.m_val.m_b = tp.m_connectedModel.m_boolModel->value();
			break;
		default:
			assert(false);
		}
	}
}

void SpaProc::shutdownPlugin()
{
	m_plugin->deactivate();

	//m_spaDescriptor->delete_plugin(m_plugin);
	delete m_plugin;
	m_plugin = nullptr;

	clearModels();
	// clear all port data (object is just raw memory after dtor call)...
	m_ports.~LmmsPorts();
	// ... so we can reuse it - C++ is just awesome
	new (&m_ports) LmmsPorts(Engine::mixer()->framesPerPeriod());
}

struct LmmsVisitor final : public virtual spa::audio::visitor
{
	SpaProc* proc;
	SpaProc::LmmsPorts *m_ports;
	QMap<QString, AutomatableModel *> *m_connectedModels;
	const char *m_curName;
	int m_audioInputs = 0; // out
	int m_audioOutputs = 0; // out
	using spa::audio::visitor::visit; // not sure if this is right, it fixes
					  // the -Woverloaded-virtual issues

	void visit(spa::audio::in &p) override
	{
		qDebug() << "in, c: " << +p.channel;
		++m_audioInputs;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? m_ports->m_lUnprocessed.data()
				: m_ports->m_rUnprocessed.data());
	}
	void visit(spa::audio::out &p) override
	{
		qDebug() << "out, c: %d\n" << +p.channel;
		++m_audioOutputs;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? m_ports->m_lProcessed.data()
				: m_ports->m_rProcessed.data());
	}
	void visit(spa::audio::stereo::in &p) override
	{
		qDebug() << "in, stereo";
		++++m_audioInputs;
		p.left = m_ports->m_lUnprocessed.data();
		p.right = m_ports->m_rUnprocessed.data();
	}
	void visit(spa::audio::stereo::out &p) override
	{
		qDebug() << "out, stereo";
		++++m_audioOutputs;
		p.left = m_ports->m_lProcessed.data();
		p.right = m_ports->m_rProcessed.data();
	}
	void visit(spa::audio::buffersize &p) override
	{
		qDebug() << "buffersize";
		p.set_ref(&m_ports->buffersize);
	}
	void visit(spa::audio::samplerate &p) override
	{
		qDebug() << "samplerate";
		p.set_ref(&m_ports->samplerate);
	}
	void visit(spa::audio::samplecount &p) override
	{
		qDebug() << "samplecount";
		p.set_ref(&m_ports->samplecount);
	}

	template <class BaseType, class ModelClass, class... ModelCtorArgs>
	void setupPort(
		spa::audio::control_in<BaseType> &port, BaseType &portData,
		ModelClass *&connectedModel,
		const ModelCtorArgs &... modelCtorArgs)
	{
		portData = port.def;
		port.set_ref(&portData);
		connectedModel = new ModelClass(static_cast<BaseType>(port),
			modelCtorArgs..., nullptr,
			QString::fromUtf8(m_curName));
		m_connectedModels->insert(
			QString::fromUtf8(m_curName), connectedModel);
		proc->addModel(connectedModel, m_curName);
	}

	// TODO: port_ref does not work yet (clang warnings), so we use
	// control_in
	void visit(spa::audio::control_in<float> &p) override
	{
		qDebug() << "other control port (float)";
		m_ports->m_userPorts.emplace_back('f', m_curName);
		SpaProc::LmmsPorts::TypedPorts &bck = m_ports->m_userPorts.back();
		setupPort(p, bck.m_val.m_f, bck.m_connectedModel.m_floatModel,
			p.min, p.max, p.step);
	}
	void visit(spa::audio::control_in<int> &p) override
	{
		qDebug() << "other control port (int)";
		m_ports->m_userPorts.emplace_back('i', m_curName);
		SpaProc::LmmsPorts::TypedPorts &bck = m_ports->m_userPorts.back();
		setupPort(p, bck.m_val.m_i, bck.m_connectedModel.m_intModel,
			p.min, p.max);
	}
	void visit(spa::audio::control_in<bool> &p) override
	{
		qDebug() << "other control port (bool)";
		m_ports->m_userPorts.emplace_back('b', m_curName);
		SpaProc::LmmsPorts::TypedPorts &bck =
			m_ports->m_userPorts.back();
		setupPort(p, bck.m_val.m_b, bck.m_connectedModel.m_boolModel);
	}

	void visit(spa::audio::osc_ringbuffer_in &p) override
	{
		qDebug() << "ringbuffer input";
		if (m_ports->rb) {
			throw std::runtime_error("can not handle 2 OSC ports");
		}
		else
		{
			m_ports->rb.reset(
				new spa::audio::osc_ringbuffer(p.get_size()));
			p.connect(*m_ports->rb);
		}
	}
	void visit(spa::port_ref_base &) override
	{
		qDebug() << "port of unknown type";
	}
};

void SpaProc::initPlugin()
{
//	m_pluginMutex.lock();
	if (!m_spaDescriptor)
	{
//		m_pluginMutex.unlock();
		m_valid = false;
	}
	else
	{
		try
		{
			spa::assert_versions_match(*m_spaDescriptor);
			m_plugin = m_spaDescriptor->instantiate();
			// TODO: unite error handling in the ctor
		}
		catch (spa::version_mismatch &mismatch)
		{
			qCritical()
				<< "Version mismatch loading plugin: "
				<< mismatch.what();
			// TODO: make an operator<<
			qCritical()
				<< "Got: " << mismatch.version.major()
				<< "." << mismatch.version.minor()
				<< "." << mismatch.version.patch()
				<< ", expect at least "
				<< mismatch.least_version.major() << "."
				<< mismatch.least_version.minor() << "."
				<< mismatch.least_version.patch();
			m_valid = false;
		}
//		m_pluginMutex.unlock();
	}

	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	spa::simple_vec<spa::simple_str> portNames =
		m_spaDescriptor->port_names();
	for (const spa::simple_str &portname : portNames)
	{
		try
		{
			// qDebug() << "portname: " << portname.data();
			spa::port_ref_base &port_ref = m_plugin->port(portname.data());

			LmmsVisitor v;
			v.proc = this;
			v.m_ports = &m_ports;
			v.m_connectedModels = &m_connectedModels;
			v.m_curName = portname.data();
			port_ref.accept(v);
			m_audioInCount += v.m_audioInputs;
			m_audioOutCount += v.m_audioOutputs;
		}
		catch (spa::port_not_found &e)
		{
			if (e.portname) {
				qWarning() << "plugin specifies invalid port \""
					   << e.portname
					   << "\", but does not provide it";
			} else {
				qWarning() << "plugin specifies invalid port, "
						  "but does not provide it";
			}
			m_valid = false; // TODO: free plugin, handle etc...
			break;
		}
	}

	if(m_valid)
	{
		// all initial ports are already set, we can do
		// initialization of buffers etc.
		m_plugin->init();

		m_plugin->activate();

		// checks not yet implemented:
		//	spa::host_utils::check_ports(descriptor, plugin);
		//	plugin->test_more();
	}
}

// TODO: maybe use event queue...
void SpaProc::writeOsc(
	const char *dest, const char *args, va_list va)
{
	// spinlock on atomic; realtime safe as the write function is very fast
	 while (m_writeOscInUse.test_and_set(std::memory_order_acquire))  // acquire lock
		; // spin
	m_ports.rb->write(dest, args, va);
	m_writeOscInUse.clear(std::memory_order_release);
}

void SpaProc::writeOsc(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOsc(dest, args, va);
	va_end(va);
}

void SpaProc::run(unsigned frames)
{
	m_ports.samplecount = static_cast<unsigned>(frames);
	m_plugin->run();
}

unsigned SpaProc::netPort() const { return m_plugin->net_port(); }


namespace detail {

void copyBuffersFromCore(std::vector<float>& portBuf,
	const sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		portBuf[f] = lmmsBuf[f][channel];
	}
}




void addBuffersFromCore(std::vector<float>& portBuf, const sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		portBuf[f] = (portBuf[f] + lmmsBuf[f][channel]) / 2.0f;
	}
}




void copyBuffersToCore(const std::vector<float>& portBuf, sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		lmmsBuf[f][channel] = portBuf[f];
	}
}

} // namespace detail

void SpaProc::copyBuffersFromCore(const sampleFrame *buf,
									unsigned offset, unsigned num,
									fpp_t frames)
{
	detail::copyBuffersFromCore(m_ports.m_lUnprocessed, buf, offset, frames);
	if (num > 1)
	{
		// if the caller requests to take input from two channels, but we only
		// have one input channel... take medium of left and right for
		// mono input
		// (this happens if we have two outputs and only one input)
		if (m_ports.m_rUnprocessed.size())
			detail::copyBuffersFromCore(m_ports.m_rUnprocessed, buf, offset + 1, frames);
		else
			detail::addBuffersFromCore(m_ports.m_lUnprocessed, buf, offset + 1, frames);
	}
}




void SpaProc::copyBuffersToCore(sampleFrame* buf,
								unsigned offset, unsigned num,
								fpp_t frames) const
{
	detail::copyBuffersToCore(m_ports.m_lProcessed, buf, offset + 0, frames);
	if (num > 1)
	{
		// if the caller requests to copy into two channels, but we only have
		// one output channel, duplicate our output
		// (this happens if we have two inputs and only one output)
		const std::vector<float>& portBuf = m_ports.m_rProcessed.size()
				? m_ports.m_rProcessed : m_ports.m_lProcessed;
		detail::copyBuffersToCore(portBuf, buf, offset + 1, frames);
	}
}




void SpaProc::uiExtShow(bool doShow) { m_plugin->ui_ext_show(doShow); }




struct SpaOscModelFactory : public spa::audio::visitor
{
	SpaProc *m_plugRef;
	const QString m_dest;

	float calc_floating_step(float step, float min, float max)
	{
		float d = fabsf(max - min);
		if(step == .0f)
		{
			step = (d >= 10.f) ? 0.01f
							   : (d >= 1.f) ? 0.001f
											: 0.0001f;
		}
		return step;
	}

public:
	AutomatableModel *m_res = nullptr;

	template <class ModelType, class ...MoreArgs>
	void make(MoreArgs... args)
	{
		m_res = new ModelType(m_plugRef, m_dest, args...);
	}

	template <class T> using CtlIn = spa::audio::control_in<T>;
	virtual void visit(CtlIn<float> &in)
	{
		make<FloatOscModel>(in.min, in.max,
			calc_floating_step(in.step, in.min, in.max), in.def );
	}
	virtual void visit(CtlIn<double> &in)
	{
		// LMMS has no double models, cast it all away
		float
			min = static_cast<float>(in.min),
			max = static_cast<float>(in.max),
			def = static_cast<float>(in.def),
			step = static_cast<float>(in.step);
		make<FloatOscModel>(min, max,
			calc_floating_step(step, min, max), def );
	}
	virtual void visit(CtlIn<int> &in)
	{
		make<IntOscModel>(in.min, in.max, in.def);
	}
	virtual void visit(CtlIn<bool> &in)
	{
		make<BoolOscModel>(in.def);
	}

	SpaOscModelFactory(SpaProc *ctrlBase, const QString &dest) :
		m_plugRef(ctrlBase), m_dest(dest)
	{
	}
};

void SpaProc::removeControl(AutomatableModel* mdl)
{
	auto itr = m_connectedModels.find(mdl->objectName());
	if(itr != m_connectedModels.end())
	{
		emit modelRemoved(mdl);
		m_connectedModels.erase(itr);
	}
}

// this function is also responsible to create a model, e.g. in case of DnD
AutomatableModel *SpaProc::modelAtPort(const QString &dest)
{
	QUrl url(dest);

	AutomatableModel *mod;
	auto itr2 = m_connectedModels.find(url.path());
	if (itr2 != m_connectedModels.end())
	{
		mod = *itr2;
	}
	else
	{
		AutomatableModel *spaMod;
		{
			SpaOscModelFactory vis(this, url.path());
			spa::port_ref_base &base =
				m_plugin->port(url.path().toUtf8().data());
			base.accept(vis);
			spaMod = vis.m_res;
		}

		if (spaMod)
		{
			addModel(mod = spaMod, url.path());
			m_connectedModels.insert(url.path(), spaMod);

			// View needs to create another child view, e.g. a new knob:
			emit modelAdded(spaMod);
			emit dataChanged();

			connect(mod, &AutomatableModel::destroyed,
				this, [this, mod](jo_id_t){ removeControl(mod); },
				Qt::DirectConnection);
		}
		else
		{
			qDebug() << "LMMS: Could not create model from "
				 << "OSC port (received port\"" << url.port()
				 << "\", path \"" << url.path() << "\")";
			mod = nullptr;
		}
	}
	return mod;
}




template <class UnsignedType> UnsignedType castToUnsigned(int val)
{
	return val >= 0 ? static_cast<unsigned>(val) : 0u;
}




SpaProc::LmmsPorts::LmmsPorts(int bufferSize) :
	buffersize(castToUnsigned<unsigned>(bufferSize)),
	m_lUnprocessed(castToUnsigned<std::size_t>(bufferSize)),
	m_rUnprocessed(castToUnsigned<std::size_t>(bufferSize)),
	m_lProcessed(castToUnsigned<std::size_t>(bufferSize)),
	m_rProcessed(castToUnsigned<std::size_t>(bufferSize))
{
}


