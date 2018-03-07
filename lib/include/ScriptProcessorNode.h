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
  ScriptProcessor(vector<function<void(ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess)>> &kernels);
  virtual ~ScriptProcessor();

  virtual void initialize() override;
  virtual void uninitialize() override;
  virtual void process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess) override;
  virtual void reset() override;
  virtual double tailTime(ContextRenderLock & r) const override;
  virtual double latencyTime(ContextRenderLock & r) const override;

protected:
  vector<function<void(ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess)>> m_kernels;
};

class ScriptProcessorNode : public AudioBasicProcessorNode {
public:
  ScriptProcessorNode(vector<function<void(ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess)>> &kernels);
  virtual ~ScriptProcessorNode();
};

}

namespace webaudio {

class ScriptProcessorNode : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);

  ScriptProcessorNode();
  ~ScriptProcessorNode();

  static void Process(lab::ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess);

protected:
  std::shared_ptr<lab::ScriptProcessorNode> audioNode;
};

}

#endif
