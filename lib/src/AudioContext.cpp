#include <AudioContext.h>
#include <memory>
#include <algorithm>

namespace webaudio {

AudioContext::AudioContext() {}

AudioContext::~AudioContext() {}

Handle<Object> AudioContext::Initialize(Isolate *isolate, Local<Value> audioDestinationNodeCons) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioContext"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto,"close", Close);
  Nan::SetAccessor(proto, JS_STR("currentTime"), CurrentTimeGetter);
  Nan::SetAccessor(proto, JS_STR("sampleRate"), SampleRateGetter);
  
  Local<Function> ctorFn = ctor->GetFunction();

  ctorFn->Set(JS_STR("AudioDestinationNode"), audioDestinationNodeCons);

  return scope.Escape(ctorFn);
}

void AudioContext::Close() {
  // nothing
}

void AudioContext::Suspend() {
  // TODO
}

void AudioContext::Resume() {
  // TODO
}

NAN_METHOD(AudioContext::New) {
  Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = new AudioContext();
  audioContext->Wrap(audioContextObj);
  
  Local<Function> audioDestinationNodeConstructor = Local<Function>::Cast(audioContextObj->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("AudioDestinationNode")));
  Local<Object> audioDestinationNodeObj = audioDestinationNodeConstructor->NewInstance(0, nullptr);
  audioContextObj->Set(JS_STR("destination"), audioDestinationNodeObj);

  info.GetReturnValue().Set(audioContextObj);
}

NAN_METHOD(AudioContext::Close) {
  Nan::HandleScope scope;
  
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->Close();
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