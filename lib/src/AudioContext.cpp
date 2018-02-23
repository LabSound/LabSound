#include <AudioContext.h>
#include <memory>
#include <algorithm>

namespace webaudio {

AudioContext::AudioContext() {}
AudioContext::~AudioContext() {}

Handle<Object> AudioContext::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioContext"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto,"close", Close);
  Nan::SetAccessor(proto, JS_STR("currentTime"), CurrentTimeGetter);
  
  Local<Function> ctorFn = ctor->GetFunction();

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

  AudioContext *audioContext = new AudioContext();
  Local<Object> audioContextObj = info.This();
  audioContext->Wrap(audioContextObj);

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
  
  double currentTime = defaultAudioContext->currentTime();

  info.GetReturnValue().Set(JS_NUM(currentTime));
}

}