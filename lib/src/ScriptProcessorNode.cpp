#include <ScriptProcessorNode.h>
#include <AudioContext.h>

namespace lab {

ScriptProcessor::ScriptProcessor(unsigned int numChannels, function<void(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess)> &&kernel) : AudioProcessor(numChannels), m_kernel(kernel) {}
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

  bool channelCountMatches = source->numberOfChannels() == destination->numberOfChannels();
  // ASSERT(channelCountMatches);
  if (!channelCountMatches) {
    return;
  }

  size_t numChannels = source->numberOfChannels();
  vector<const float*> sources(numChannels);
  vector<float*> destinations(numChannels);
  for (unsigned i = 0; i < numChannels; ++i) {
    sources[i] = source->channel(i)->data();
    destinations[i] = destination->channel(i)->mutableData();
  }

  m_kernel(r, sources, destinations, framesToProcess);
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

ScriptProcessorNode::ScriptProcessorNode(unsigned int numChannels, function<void(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess)> &&kernel) : AudioBasicProcessorNode() {
  m_processor.reset(new ScriptProcessor(numChannels, std::move(kernel)));

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
  Nan::SetAccessor(proto, JS_STR("onaudioprocess"), OnAudioProcessGetter, OnAudioProcessSetter);
}

NAN_METHOD(ScriptProcessorNode::New) {
  Nan::HandleScope scope;

  if (info[0]->IsObject() && info[0]->ToObject()->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))) {
    Local<Object> audioContextObj = Local<Object>::Cast(info[0]);

    ScriptProcessorNode *scriptProcessorNode = new ScriptProcessorNode();
    Local<Object> scriptProcessorNodeObj = info.This();
    scriptProcessorNode->Wrap(scriptProcessorNodeObj);

    scriptProcessorNode->audioNode.reset(new lab::ScriptProcessorNode(2, [scriptProcessorNode](lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess) {
      scriptProcessorNode->Process(r, sources, destinations, framesToProcess);
    }));

    scriptProcessorNodeObj->Set(JS_STR("onaudioprocess"), Nan::Null());

    info.GetReturnValue().Set(scriptProcessorNodeObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_GETTER(ScriptProcessorNode::OnAudioProcessGetter) {
  Nan::HandleScope scope;

  ScriptProcessorNode *scriptProcessorNode = ObjectWrap::Unwrap<ScriptProcessorNode>(info.This());

  Local<Function> onAudioProcessLocal = Nan::New(scriptProcessorNode->onAudioProcess);
  info.GetReturnValue().Set(onAudioProcessLocal);
}

NAN_SETTER(ScriptProcessorNode::OnAudioProcessSetter) {
  Nan::HandleScope scope;

  if (value->IsFunction()) {
    ScriptProcessorNode *scriptProcessorNode = ObjectWrap::Unwrap<ScriptProcessorNode>(info.This());

    Local<Function> onAudioProcessLocal = Local<Function>::Cast(value);
    scriptProcessorNode->onAudioProcess.Reset(onAudioProcessLocal);
  } else {
    Nan::ThrowError("onaudioprocess: invalid arguments");
  }
}

void ScriptProcessorNode::Process(lab::ContextRenderLock& r, vector<const float*> sources, vector<float*> destinations, size_t framesToProcess) {
  if (!onAudioProcess.IsEmpty()) {
    Local<Function> onAudioProcessLocal = Nan::New(onAudioProcess);
    // XXX
  } else {
    for (size_t i = 0; i < sources.size(); i++) {
      memcpy(destinations[i], sources[i], framesToProcess * sizeof(float));
    }
  }
}

}
