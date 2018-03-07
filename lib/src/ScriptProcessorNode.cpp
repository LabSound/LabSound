#include <ScriptProcessorNode.h>
#include <AudioContext.h>

namespace lab {

ScriptProcessor::ScriptProcessor(vector<function<void(ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess)>> &kernels) : AudioProcessor(kernels.size()), m_kernels(std::move(kernels)) {}
ScriptProcessor::~ScriptProcessor() {
  if (isInitialized()) {
    uninitialize();
  }
}
void ScriptProcessor::process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess) {
  // ASSERT(source && destination);
  if (!source || !destination) {
    return;
  }

  if (!isInitialized()) {
    destination->zero();
    return;
  }

  bool channelCountMatches = source->numberOfChannels() == destination->numberOfChannels() && source->numberOfChannels() == m_kernels.size();
  // ASSERT(channelCountMatches);
  if (!channelCountMatches) {
    return;
  }

  for (unsigned i = 0; i < m_kernels.size(); ++i) {
    m_kernels[i](r, source->channel(i)->data(), destination->channel(i)->mutableData(), framesToProcess);
  }
}
void ScriptProcessor::initialize() {}
void ScriptProcessor::uninitialize() {}
void ScriptProcessor::reset() {}
double ScriptProcessor::tailTime(ContextRenderLock & r) const {
  return 0;
}
double ScriptProcessor::latencyTime(ContextRenderLock & r) const {
  return 0;
}

ScriptProcessorNode::ScriptProcessorNode(vector<function<void(ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess)>> &kernels) : AudioBasicProcessorNode() {
  m_processor.reset(new ScriptProcessor(kernels));

  initialize();
}
ScriptProcessorNode::~ScriptProcessorNode() {}

}

namespace webaudio {

ScriptProcessorNode::ScriptProcessorNode() {}

ScriptProcessorNode::~ScriptProcessorNode() {}

Handle<Object> ScriptProcessorNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("ScriptProcessorNode"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  ScriptProcessorNode::InitializePrototype(proto);
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

void ScriptProcessorNode::InitializePrototype(Local<ObjectTemplate> proto) {
  // nothing
}

NAN_METHOD(ScriptProcessorNode::New) {
  Nan::HandleScope scope;

  if (info[0]->IsObject() && info[0]->ToObject()->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))) {
    Local<Object> audioContextObj = Local<Object>::Cast(info[0]);

    ScriptProcessorNode *scriptProcessorNode = new ScriptProcessorNode();
    Local<Object> scriptProcessorNodeObj = info.This();
    scriptProcessorNode->Wrap(scriptProcessorNodeObj);

    vector<function<void(lab::ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess)>> kernels{
      ScriptProcessorNode::Process,
      ScriptProcessorNode::Process,
    };
    scriptProcessorNode->audioNode.reset(new lab::ScriptProcessorNode(kernels));

    info.GetReturnValue().Set(scriptProcessorNodeObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

void ScriptProcessorNode::Process(lab::ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess) {
  memcpy(destination, source, framesToProcess * sizeof(float));
}

}
