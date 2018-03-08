#ifndef _SCRIPT_PROCESSOR_NODE_H_
#define _SCRIPT_PROCESSOR_NODE_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include <functional>
#include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <AudioNode.h>

using namespace std;
using namespace v8;
using namespace node;

namespace lab {

class ScriptProcessor : public AudioProcessor {
public:
  ScriptProcessor(unsigned int numChannels, function<void(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess)> &&kernel);
  virtual ~ScriptProcessor();

  virtual void initialize() override;
  virtual void uninitialize() override;
  virtual void process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess) override;
  virtual void reset() override;
  virtual double tailTime(ContextRenderLock & r) const override;
  virtual double latencyTime(ContextRenderLock & r) const override;

protected:
  function<void(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess)> m_kernel;
};

class AudioBufferSourceNode : public SampledAudioNode {
public:
  AudioBufferSourceNode(function<void()> &&finishedCallback);
  ~AudioBufferSourceNode();
protected:
  virtual void finish(ContextRenderLock &r);

  function<void()> finishedCallback;
};

class AudioMultiProcessorNode : public AudioNode {
public:

    AudioMultiProcessorNode(unsigned int numChannels);
    virtual ~AudioMultiProcessorNode() {}

    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void pullInputs(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // Called in the main thread when the number of channels for the input may have changed.
    virtual void checkNumberOfChannelsForInput(ContextRenderLock&, AudioNodeInput*) override;

    // Returns the number of channels for both the input and the output.
    unsigned numberOfChannels();

protected:

    virtual double tailTime(ContextRenderLock & r) const override;
    virtual double latencyTime(ContextRenderLock & r) const override;

    AudioProcessor * processor();

    std::unique_ptr<AudioProcessor> m_processor;

};

class ScriptProcessorNode : public AudioMultiProcessorNode {
public:
  ScriptProcessorNode(unsigned int numChannels, function<void(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess)> &&kernel);
  virtual ~ScriptProcessorNode();

protected:
};

}

namespace webaudio {

class AudioBuffer : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate);

protected:
  static NAN_METHOD(New);

  AudioBuffer(uint32_t sampleRate, Local<Array> buffers);
  ~AudioBuffer();

  static NAN_GETTER(Length);
  static NAN_GETTER(NumberOfChannels);
  static NAN_METHOD(GetChannelData);
  static NAN_METHOD(CopyFromChannel);
  static NAN_METHOD(CopyToChannel);

  uint32_t sampleRate;
  Nan::Persistent<Array> buffers;

  friend class ScriptProcessorNode;
};

class AudioBufferSourceNode : public AudioNode {
public:
  static Handle<Object> Initialize(Isolate *isolate);
  void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  AudioBufferSourceNode();
  ~AudioBufferSourceNode();
  
  static NAN_METHOD(New);
  static NAN_METHOD(Start);
  static NAN_METHOD(Stop);
  static NAN_GETTER(BufferGetter);
  static NAN_SETTER(BufferSetter);
  
  Nan::Persistent<Object> buffer;
};

class AudioProcessingEvent : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate);

protected:
  static NAN_METHOD(New);

  AudioProcessingEvent(Local<Object> inputBuffer, Local<Object> outputBuffer);
  ~AudioProcessingEvent();

  static NAN_GETTER(InputBufferGetter);
  static NAN_GETTER(OutputBufferGetter);
  static NAN_GETTER(NumberOfInputChannelsGetter);
  static NAN_GETTER(NumberOfOutputChannelsGetter);

  Nan::Persistent<Object> inputBuffer;
  Nan::Persistent<Object> outputBuffer;
};

class ScriptProcessorNode : public AudioNode {
public:
  static Handle<Object> Initialize(Isolate *isolate, Local<Value> audioBufferCons, Local<Value> audioProcessingEventCons);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);

  ScriptProcessorNode();
  ~ScriptProcessorNode();

  static NAN_GETTER(OnAudioProcessGetter);
  static NAN_SETTER(OnAudioProcessSetter);
  void ProcessInAudioThread(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess);
  static void ProcessInMainThread(uv_async_t *handle);

  Nan::Persistent<Function> audioBufferConstructor;
  Nan::Persistent<Function> audioProcessingEventConstructor;
  Nan::Persistent<Function> onAudioProcess;

  uv_async_t threadAsync;
  uv_sem_t threadSemaphore;
  static ScriptProcessorNode *threadScriptProcessorNode;
  static vector<const float*> *threadSources;
  static vector<float*> *threadDestinations;
  static size_t threadFramesToProcess;
};

}

#endif
