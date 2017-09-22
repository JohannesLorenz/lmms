#ifndef OSCPLUGIN_H
#define OSCPLUGIN_H

class OscPlugin
{
public:
	virtual void runSynth(float* outl, float* outr, unsigned long sample_count) = 0;
	virtual void sendOsc(const char* port, const char* args, ...) = 0;
	virtual unsigned long buffersize() const = 0;
	virtual ~OscPlugin() {}
};

class OscDescriptor
{
public:
	virtual const char* label() const = 0;
	virtual const char* name() const = 0;
	virtual const char* maker() const = 0;
	virtual const char* copyright() const = 0;
//	virtual int id() const = 0;
	virtual OscPlugin* instantiate(unsigned long srate) const = 0;
	virtual ~OscDescriptor() {}
};

typedef OscDescriptor* (*osc_descriptor_loader_t) (unsigned long);

#endif // OSCPLUGIN_H
