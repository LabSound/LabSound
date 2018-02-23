#include <Audio.h>
#include <memory>
#include <algorithm>

namespace webaudio {

Audio::Audio() {}
Audio::~Audio() {}

Handle<Object> Audio::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("Audio"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "load", Load);
  Nan::SetMethod(proto, "play", Play);
  Nan::SetMethod(proto, "pause", Pause);
  Nan::SetAccessor(proto, JS_STR("currentTime"), CurrentTimeGetter);
  Nan::SetAccessor(proto, JS_STR("duration"), DurationGetter);
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

NAN_METHOD(Audio::New) {
  Nan::HandleScope scope;

  Audio *audio = new Audio();
  Local<Object> audioObj = info.This();
  audio->Wrap(audioObj);

  info.GetReturnValue().Set(audioObj);
}

void Audio::Load(uint8_t *bufferValue, size_t bufferLength, const char *extensionValue) {
  lab::ContextRenderLock lock(defaultAudioContext.get(), "Audio::Load");

  vector<uint8_t> buffer(bufferLength);
  memcpy(buffer.data(), bufferValue, bufferLength);
  
  string extension(extensionValue);

  audioBus = lab::MakeBusFromMemory(buffer, extension, false);
  audioNode.reset(new lab::SampledAudioNode());
  audioNode->setBus(lock, audioBus);
  
  defaultAudioContext->connect(defaultAudioContext->destination(), audioNode, 0, 0); // XXX make this node connection manual
}

void Audio::Play() {
  audioNode->start(0);
}

void Audio::Pause() {
  audioNode->stop(0);
}

NAN_METHOD(Audio::Load) {
  if (info[0]->IsTypedArray() && info[1]->IsString()) {
    Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());

    Local<ArrayBufferView> arrayBufferView = Local<ArrayBufferView>::Cast(info[0]);
    Local<ArrayBuffer> arrayBuffer = arrayBufferView->Buffer();
    v8::String::Utf8Value extensionValue(info[1]->ToString());
    
    audio->Load((uint8_t *)arrayBuffer->GetContents().Data() + arrayBufferView->ByteOffset(), arrayBufferView->ByteLength(), *extensionValue);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(Audio::Play) {
  Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());
  audio->Play();
}

NAN_METHOD(Audio::Pause) {
  Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());
  audio->Pause();
}

NAN_GETTER(Audio::CurrentTimeGetter) {
  Nan::HandleScope scope;
  
  Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());

  double now = defaultAudioContext->currentTime();
  double startTime = audio->audioNode->startTime();
  double duration = audio->audioNode->duration();
  double currentTime = std::min<double>(std::max<double>(startTime - now, 0), duration);

  info.GetReturnValue().Set(JS_NUM(currentTime));
}

NAN_GETTER(Audio::DurationGetter) {
  Nan::HandleScope scope;
  
  Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());

  double duration = audio->audioNode->duration();

  info.GetReturnValue().Set(JS_NUM(duration));
}

}