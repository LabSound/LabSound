#include <AudioContext.h>

namespace webaudio {

AudioContext::AudioContext() : audioContext(defaultAudioContext.get()) {}

AudioContext::~AudioContext() {}

Handle<Object> AudioContext::Initialize(Isolate *isolate, Local<Value> audioSourceNodeCons, Local<Value> audioDestinationNodeCons, Local<Value> gainNodeCons, Local<Value> analyserNodeCons) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioContext"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto,"close", Close);
  Nan::SetMethod(proto,"createMediaElementSource", CreateMediaElementSource);
  Nan::SetMethod(proto,"createMediaStreamSource", CreateMediaStreamSource);
  Nan::SetMethod(proto,"createMediaStreamDestination", CreateMediaStreamDestination);
  Nan::SetMethod(proto,"createMediaStreamTrackSource", CreateMediaStreamTrackSource);
  Nan::SetMethod(proto,"createGain", CreateGain);
  Nan::SetMethod(proto,"createAnalyser", CreateAnalyser);
  Nan::SetAccessor(proto, JS_STR("currentTime"), CurrentTimeGetter);
  Nan::SetAccessor(proto, JS_STR("sampleRate"), SampleRateGetter);

  Local<Function> ctorFn = ctor->GetFunction();

  ctorFn->Set(JS_STR("AudioSourceNode"), audioSourceNodeCons);
  ctorFn->Set(JS_STR("AudioDestinationNode"), audioDestinationNodeCons);
  ctorFn->Set(JS_STR("GainNode"), gainNodeCons);
  ctorFn->Set(JS_STR("AnalyserNode"), analyserNodeCons);

  return scope.Escape(ctorFn);
}

void AudioContext::Close() {
  Nan::ThrowError("AudioContext::Close: not implemented"); // TODO
}

Local<Object> AudioContext::CreateMediaElementSource(Local<Function> audioDestinationNodeConstructor, Local<Object> mediaElement, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    mediaElement,
    audioContextObj,
  };
  Local<Object> audioSourceNodeObj = audioDestinationNodeConstructor->NewInstance(sizeof(argv)/sizeof(argv[0]), argv);

  return audioSourceNodeObj;
}

void AudioContext::CreateMediaStreamSource() {
  Nan::ThrowError("AudioContext::CreateMediaStreamSource: not implemented"); // TODO
}

void AudioContext::CreateMediaStreamDestination() {
  Nan::ThrowError("AudioContext::CreateMediaStreamDestination: not implemented"); // TODO
}

void AudioContext::CreateMediaStreamTrackSource() {
  Nan::ThrowError("AudioContext::CreateMediaStreamTrackSource: not implemented"); // TODO
}

Local<Object> AudioContext::CreateGain(Local<Function> gainNodeConstructor, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    audioContextObj,
  };
  Local<Object> gainNodeObj = gainNodeConstructor->NewInstance(sizeof(argv)/sizeof(argv[0]), argv);

  return gainNodeObj;
}

void AudioContext::Suspend() {
  Nan::HandleScope scope;

  Nan::ThrowError("AudioContext::Suspend: not implemented"); // TODO
}

void AudioContext::Resume() {
  Nan::HandleScope scope;

  Nan::ThrowError("AudioContext::Resume: not implemented"); // TODO
}

NAN_METHOD(AudioContext::New) {
  Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = new AudioContext();
  audioContext->Wrap(audioContextObj);

  Local<Function> audioDestinationNodeConstructor = Local<Function>::Cast(audioContextObj->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("AudioDestinationNode")));
  Local<Value> argv[] = {
    audioContextObj,
  };
  Local<Object> audioDestinationNodeObj = audioDestinationNodeConstructor->NewInstance(sizeof(argv)/sizeof(argv[0]), argv);
  audioContextObj->Set(JS_STR("destination"), audioDestinationNodeObj);

  info.GetReturnValue().Set(audioContextObj);
}

NAN_METHOD(AudioContext::Close) {
  Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->Close();
}

NAN_METHOD(AudioContext::CreateMediaElementSource) {
  Nan::HandleScope scope;

  if (info[0]->IsObject() && info[0]->ToObject()->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("name"))->StrictEquals(JS_STR("Audio"))) {
    Local<Object> audioContextObj = info.This();
    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

    Local<Function> audioDestinationNodeConstructor = Local<Function>::Cast(audioContextObj->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("AudioSourceNode")));
    Local<Object> audioNodeObj = audioContext->CreateMediaElementSource(audioDestinationNodeConstructor, Local<Object>::Cast(info[0]), audioContextObj);

    info.GetReturnValue().Set(audioNodeObj);
  } else {
    Nan::ThrowError("AudioContext::CreateMediaElementSource: invalid arguments");
  }
}

NAN_METHOD(AudioContext::CreateMediaStreamSource) {
  Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->CreateMediaStreamSource();
}

NAN_METHOD(AudioContext::CreateMediaStreamDestination) {
  Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->CreateMediaStreamDestination();
}

NAN_METHOD(AudioContext::CreateMediaStreamTrackSource) {
  Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->CreateMediaStreamTrackSource();
}

NAN_METHOD(AudioContext::CreateGain) {
  Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> gainNodeConstructor = Local<Function>::Cast(audioContextObj->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("GainNode")));
  Local<Object> gainNodeObj = audioContext->CreateGain(gainNodeConstructor, audioContextObj);

  info.GetReturnValue().Set(gainNodeObj);
}

NAN_METHOD(AudioContext::CreateAnalyser) {
  Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> analyserNodeConstructor = Local<Function>::Cast(audioContextObj->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("AnalyserNode")));
  Local<Object> analyserNodeObj = audioContext->CreateGain(analyserNodeConstructor, audioContextObj);

  info.GetReturnValue().Set(analyserNodeObj);
}

NAN_METHOD(AudioContext::Suspend) {
  Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->Suspend();
}

NAN_METHOD(AudioContext::Resume) {
  Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->Resume();
}

NAN_GETTER(AudioContext::CurrentTimeGetter) {
  Nan::HandleScope scope;

  info.GetReturnValue().Set(JS_NUM(defaultAudioContext->currentTime()));
}

NAN_GETTER(AudioContext::SampleRateGetter) {
  Nan::HandleScope scope;

  info.GetReturnValue().Set(JS_NUM(defaultAudioContext->sampleRate()));
}

}