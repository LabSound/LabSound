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

AudioBuffer::AudioBuffer(Local<Array> buffers, uint32_t numFrames) : buffers(buffers), numFrames(numFrames) {}
AudioBuffer::~AudioBuffer() {}
Handle<Object> AudioBuffer::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioBuffer"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetAccessor(proto, JS_STR("numberOfChannels"), NumberOfChannels);
  Nan::SetMethod(proto, "getChannelData", GetChannelData);
  Nan::SetMethod(proto, "copyFromChannel", CopyFromChannel);
  Nan::SetMethod(proto, "copyToChannel", CopyToChannel);
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}
NAN_METHOD(AudioBuffer::New) {
  Nan::HandleScope scope;

  if (info[0]->IsArray() && info[1]->IsNumber()) {
    Local<Array> buffers = Local<Array>::Cast(info[0]);
    uint32_t numFrames = info[1]->Uint32Value();

    AudioBuffer *audioBuffer = new AudioBuffer(buffers, numFrames);
    Local<Object> audioBufferObj = info.This();
    audioBuffer->Wrap(audioBufferObj);
  } else {
    Nan::ThrowError("AudioProcessingEvent:New: invalid arguments");
  }
}
NAN_GETTER(AudioBuffer::NumberOfChannels) {
  Nan::HandleScope scope;

  AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
  Local<Array> buffers = Nan::New(audioBuffer->buffers);

  info.GetReturnValue().Set(JS_INT(buffers->Length()));
}
NAN_METHOD(AudioBuffer::GetChannelData) {
  Nan::HandleScope scope;

  if (info[0]->IsNumber()) {
    uint32_t channelIndex = info[0]->Uint32Value();

    AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
    Local<Array> buffers = Nan::New(audioBuffer->buffers);
    Local<Value> channelData = buffers->Get(channelIndex);

    info.GetReturnValue().Set(channelData);
  } else {
    Nan::ThrowError("AudioBuffer:GetChannelData: invalid arguments");
  }
}
NAN_METHOD(AudioBuffer::CopyFromChannel) {
  Nan::HandleScope scope;

  if (info[0]->IsFloat32Array() && info[1]->IsNumber()) {
    Local<Float32Array> destinationFloat32Array = Local<Float32Array>::Cast(info[0]);
    uint32_t channelIndex = info[1]->Uint32Value();

    AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
    Local<Array> buffers = Nan::New(audioBuffer->buffers);
    Local<Value> channelData = buffers->Get(channelIndex);

    if (channelData->BooleanValue()) {
      Local<Float32Array> channelDataFloat32Array = Local<Float32Array>::Cast(channelData);
      uint32_t offset = std::min<uint32_t>(info[2]->IsNumber() ? info[2]->Uint32Value() : 0, channelDataFloat32Array->Length());
      size_t copyLength = std::min<size_t>(channelDataFloat32Array->Length() - offset, destinationFloat32Array->Length());
      for (size_t i = 0; i < copyLength; i++) {
        destinationFloat32Array->Set(i, channelDataFloat32Array->Get(offset + i));
      }
    } else {
      Nan::ThrowError("AudioBuffer:CopyFromChannel: invalid channel index");
    }
  } else {
    Nan::ThrowError("AudioBuffer:CopyFromChannel: invalid arguments");
  }
}
NAN_METHOD(AudioBuffer::CopyToChannel) {
  Nan::HandleScope scope;

  if (info[0]->IsFloat32Array() && info[1]->IsNumber()) {
    Local<Float32Array> sourceFloat32Array = Local<Float32Array>::Cast(info[0]);
    uint32_t channelIndex = info[1]->Uint32Value();

    AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
    Local<Array> buffers = Nan::New(audioBuffer->buffers);
    Local<Value> channelData = buffers->Get(channelIndex);

    if (channelData->BooleanValue()) {
      Local<Float32Array> channelDataFloat32Array = Local<Float32Array>::Cast(channelData);
      uint32_t offset = std::min<uint32_t>(info[2]->IsNumber() ? info[2]->Uint32Value() : 0, channelDataFloat32Array->Length());
      size_t copyLength = std::min<size_t>(channelDataFloat32Array->Length() - offset, sourceFloat32Array->Length());
      for (size_t i = 0; i < copyLength; i++) {
        channelDataFloat32Array->Set(offset + i, sourceFloat32Array->Get(i));
      }
    } else {
      Nan::ThrowError("AudioBuffer:CopyToChannel: invalid channel index");
    }
  } else {
    Nan::ThrowError("AudioBuffer:CopyToChannel: invalid arguments");
  }
}

AudioProcessingEvent::AudioProcessingEvent(Local<Object> inputBuffer, Local<Object> outputBuffer, uint32_t numFrames) : inputBuffer(inputBuffer), outputBuffer(outputBuffer), numFrames(numFrames) {}
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

  if (info[0]->IsObject() && info[1]->IsObject() && info[2]->IsNumber()) {
    Local<Object> inputBuffer = Local<Object>::Cast(info[0]);
    Local<Object> outputBuffer = Local<Object>::Cast(info[1]);
    uint32_t numFrames = info[2]->Uint32Value();

    AudioProcessingEvent *audioProcessingEvent = new AudioProcessingEvent(inputBuffer, outputBuffer, numFrames);
    Local<Object> audioProcessingEventObj = info.This();
    audioProcessingEvent->Wrap(audioProcessingEventObj);
  } else {
    Nan::ThrowError("AudioProcessingEvent:New: invalid arguments");
  }
}
NAN_GETTER(AudioProcessingEvent::NumberOfInputChannelsGetter) {
  Nan::HandleScope scope;

  AudioProcessingEvent *audioProcessingEvent = ObjectWrap::Unwrap<AudioProcessingEvent>(info.This());
  Local<Object> inputBufferObj = Nan::New(audioProcessingEvent->inputBuffer);
  Local<Value> numberOfChannels = inputBufferObj->Get(JS_STR("numberOfChannels"));
  info.GetReturnValue().Set(numberOfChannels);
}
NAN_GETTER(AudioProcessingEvent::NumberOfOutputChannelsGetter) {
  Nan::HandleScope scope;

  AudioProcessingEvent *audioProcessingEvent = ObjectWrap::Unwrap<AudioProcessingEvent>(info.This());
  Local<Object> outpuctBufferObj = Nan::New(audioProcessingEvent->outputBuffer);
  Local<Value> numberOfChannels = outpuctBufferObj->Get(JS_STR("numberOfChannels"));
  info.GetReturnValue().Set(numberOfChannels);
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

  ctorFn->Set(JS_STR("AudioBuffer"), AudioBuffer::Initialize(isolate));
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

    Local<Function> audioBufferConstructor = Local<Function>::Cast(audioContextObj->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("AudioBuffer")));
    scriptProcessorNode->audioBufferConstructor.Reset(audioBufferConstructor);
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
    Local<Function> audioBufferConstructorFn = Nan::New(audioBufferConstructor);
    Local<Value> argv1[] = {
      sourcesArray,
    };
    Local<Object> inputBuffer = audioBufferConstructorFn->NewInstance(sizeof(argv1)/sizeof(argv1[0]), argv1);

    Local<Array> destinationsArray = Nan::New<Array>(destinations.size());
    for (size_t i = 0; i < destinations.size(); i++) {
      float *destination = destinations[i];
      Local<ArrayBuffer> destinationArrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), destination, framesToProcess * sizeof(float));
      Local<Float32Array> destinationFloat32Array = Float32Array::New(destinationArrayBuffer, 0, framesToProcess);
      destinationsArray->Set(i, destinationFloat32Array);
    }
    Local<Value> argv2[] = {
      destinationsArray,
    };
    Local<Object> outputBuffer = audioBufferConstructorFn->NewInstance(sizeof(argv2)/sizeof(argv2[0]), argv2);

    Local<Function> audioProcessingEventConstructorFn = Nan::New(audioProcessingEventConstructor);
    Local<Value> argv3[] = {
      inputBuffer,
      outputBuffer,
      JS_INT((uint32_t)framesToProcess)
    };
    Local<Object> audioProcessingEventObj = audioProcessingEventConstructorFn->NewInstance(sizeof(argv3)/sizeof(argv3[0]), argv3);

    Local<Value> argv4[] = {
      audioProcessingEventObj,
    };
    onAudioProcessLocal->Call(Nan::Null(), sizeof(argv4)/sizeof(argv4[0]), argv4);
  } else {
    for (size_t i = 0; i < sources.size(); i++) {
      memcpy(destinations[i], sources[i], framesToProcess * sizeof(float));
    }
  }
}

}
