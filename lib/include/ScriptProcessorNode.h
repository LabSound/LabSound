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

class ScriptProcessorNode : public AudioBasicProcessorNode {
public:
  ScriptProcessorNode(unsigned int numChannels, function<void(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess)> &&kernel);
  virtual ~ScriptProcessorNode();
};

}

namespace webaudio {

class AudioProcessingEvent : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate);

protected:
  static NAN_METHOD(New);

  AudioProcessingEvent(Local<Array> sources, Local<Array> destinations, uint32_t numFrames);
  ~AudioProcessingEvent();

  static NAN_GETTER(NumberOfInputChannelsGetter);
  static NAN_GETTER(NumberOfOutputChannelsGetter);

  Nan::Persistent<Array> sources;
  Nan::Persistent<Array> destinations;
  uint32_t numFrames;
};

class ScriptProcessorNode : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);

  ScriptProcessorNode();
  ~ScriptProcessorNode();

  static NAN_GETTER(OnAudioProcessGetter);
  static NAN_SETTER(OnAudioProcessSetter);

  void Process(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess);

protected:
  std::shared_ptr<lab::ScriptProcessorNode> audioNode;
  Nan::Persistent<Function> audioProcessingEventConstructor;
  Nan::Persistent<Function> onAudioProcess;
};

}

#endif
