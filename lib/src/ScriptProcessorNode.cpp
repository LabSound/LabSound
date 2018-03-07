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

AudioProcessingEvent::AudioProcessingEvent(Local<Array> sources, Local<Array> destinations, uint32_t numFrames) : sources(sources), destinations(destinations), numFrames(numFrames) {}
AudioProcessingEvent::~AudioProcessingEvent() {}
Handle<Object> AudioProcessingEvent::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioProcessingEvent"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetAccessor(proto, JS_STR("numberOfInputChannels"), NumberOfInputChannelsGetter);
  Nan::SetAccessor(proto, JS_STR("numberOfOutputChannels"), NumberOfOutputChannelsGetter);
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}
NAN_METHOD(AudioProcessingEvent::New) {
  Nan::HandleScope scope;

  if (info[0]->IsArray() && info[1]->IsArray() && info[2]->IsNumber()) {
    Local<Array> sourcesArray = Local<Array>::Cast(info[0]);
    Local<Array> destinationsArray = Local<Array>::Cast(info[1]);
    uint32_t numFrames = info[2]->Uint32Value();

    AudioProcessingEvent *audioProcessingEvent = new AudioProcessingEvent(sourcesArray, destinationsArray, numFrames);
    Local<Object> audioProcessingEventObj = info.This();
    audioProcessingEvent->Wrap(audioProcessingEventObj);
  } else {
    Nan::ThrowError("AudioProcessingEvent:New: invalid arguments");
  }
}
NAN_GETTER(AudioProcessingEvent::NumberOfInputChannelsGetter) {
  Nan::HandleScope scope;

  AudioProcessingEvent *audioProcessingEvent = ObjectWrap::Unwrap<AudioProcessingEvent>(info.This());
  Local<Array> sources = Nan::New(audioProcessingEvent->sources);
  info.GetReturnValue().Set(JS_INT(sources->Length()));
}
NAN_GETTER(AudioProcessingEvent::NumberOfOutputChannelsGetter) {
  Nan::HandleScope scope;

  AudioProcessingEvent *audioProcessingEvent = ObjectWrap::Unwrap<AudioProcessingEvent>(info.This());
  Local<Array> destinations = Nan::New(audioProcessingEvent->destinations);
  info.GetReturnValue().Set(JS_INT(destinations->Length()));
}

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

  ctorFn->Set(JS_STR("AudioProcessingEvent"), AudioProcessingEvent::Initialize(isolate));

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

    Local<Function> audioProcessingEventConstructor = Local<Function>::Cast(audioContextObj->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("AudioProcessingEvent")));
    scriptProcessorNode->audioProcessingEventConstructor.Reset(audioProcessingEventConstructor);

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

    Local<Array> sourcesArray = Nan::New<Array>(sources.size());
    for (size_t i = 0; i < sources.size(); i++) {
      const float *source = sources[i];
      Local<ArrayBuffer> sourceArrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), (void *)source, framesToProcess * sizeof(float));
      Local<Float32Array> sourceFloat32Array = Float32Array::New(sourceArrayBuffer, 0, framesToProcess);
      sourcesArray->Set(i, sourceFloat32Array);
    }
    Local<Array> destinationsArray = Nan::New<Array>(destinations.size());
    for (size_t i = 0; i < destinations.size(); i++) {
      float *destination = destinations[i];
      Local<ArrayBuffer> destinationArrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), destination, framesToProcess * sizeof(float));
      Local<Float32Array> destinationFloat32Array = Float32Array::New(destinationArrayBuffer, 0, framesToProcess);
      destinationsArray->Set(i, destinationFloat32Array);
    }

    Local<Function> audioProcessingEventConstructorFn = Nan::New(audioProcessingEventConstructor);
    Local<Value> argv1[] = {
      sourcesArray,
      destinationsArray,
      JS_INT((uint32_t)framesToProcess)
    };
    Local<Object> audioProcessingEventObj = audioProcessingEventConstructorFn->NewInstance(sizeof(argv1)/sizeof(argv1[0]), argv1);

    Local<Value> argv2[] = {
      audioProcessingEventObj,
    };
    onAudioProcessLocal->Call(Nan::Null(), sizeof(argv2)/sizeof(argv2[0]), argv2);
  } else {
    for (size_t i = 0; i < sources.size(); i++) {
      memcpy(destinations[i], sources[i], framesToProcess * sizeof(float));
    }
  }
}

}
