#include <Audio.h>
#include <memory>
#include <algorithm>

#include <rtaudio/RtAudio.h>

using namespace v8;

#define JS_STR(...) Nan::New<v8::String>(__VA_ARGS__).ToLocalChecked()
#define JS_INT(val) Nan::New<v8::Integer>(val)
#define JS_NUM(val) Nan::New<v8::Number>(val)
#define JS_FLOAT(val) Nan::New<v8::Number>(val)
#define JS_BOOL(val) Nan::New<v8::Boolean>(val)

namespace webaudio {

Audio::Audio() : audioNode(new lab::SampledAudioNode()) {}

Audio::~Audio() {}

Handle<Object> Audio::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("Audio"));
  Nan::SetMethod(ctor, "getDevices", GetDevices);

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "load", Load);
  Nan::SetMethod(proto, "play", Play);
  Nan::SetMethod(proto, "pause", Pause);
  Nan::SetAccessor(proto, JS_STR("currentTime"), CurrentTimeGetter);
  Nan::SetAccessor(proto, JS_STR("duration"), DurationGetter);
  Nan::SetAccessor(proto, JS_STR("loop"), LoopGetter, LoopSetter);

  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

NAN_METHOD(Audio::GetDevices) {
  RtAudio audio;
  size_t n = audio.getDeviceCount();

  Local<Object> lst = Array::New(Isolate::GetCurrent());
  size_t i = 0;
  for (size_t i = 0; i < n; i++) {
    RtAudio::DeviceInfo info(audio.getDeviceInfo(i));
    Local<Object> obj = Object::New(Isolate::GetCurrent());
    lst->Set(i, obj);
    obj->Set(JS_STR("name"), JS_STR(info.name.c_str()));
    obj->Set(JS_STR("outputChannels"), JS_NUM(info.outputChannels));
    obj->Set(JS_STR("inputChannels"), JS_NUM(info.inputChannels));
    obj->Set(JS_STR("duplexChannels"), JS_NUM(info.duplexChannels));
    obj->Set(JS_STR("isDefaultOutput"), JS_BOOL(info.isDefaultOutput));
    obj->Set(JS_STR("isDefaultInput"), JS_BOOL(info.isDefaultInput));
    obj->Set(JS_STR("preferredSampleRate"), JS_NUM(info.preferredSampleRate));
    {
      Local<Object> sampleRates = Array::New(Isolate::GetCurrent());
      size_t j = 0;
      for (auto rate : info.sampleRates) {
        sampleRates->Set(j++, JS_NUM(rate));
      }
      obj->Set(JS_STR("sampleRates"), sampleRates);
    }
    {
      Local<Object> nativeFormats = Array::New(Isolate::GetCurrent());
      size_t j = 0;
      if (info.nativeFormats & RTAUDIO_SINT8) {
        nativeFormats->Set(j++, JS_STR("int8"));
      }
      if (info.nativeFormats & RTAUDIO_SINT16) {
        nativeFormats->Set(j++, JS_STR("int16"));
      }
      if (info.nativeFormats & RTAUDIO_SINT32) {
        nativeFormats->Set(j++, JS_STR("int32"));
      }
      if (info.nativeFormats & RTAUDIO_FLOAT32) {
        nativeFormats->Set(j++, JS_STR("float32"));
      }
      if (info.nativeFormats & RTAUDIO_FLOAT64) {
        nativeFormats->Set(j++, JS_STR("float64"));
      }
      obj->Set(JS_STR("nativeFormats"), nativeFormats);
    }
  }
  info.GetReturnValue().Set(lst);
}

NAN_METHOD(Audio::New) {
  Nan::HandleScope scope;

  Audio *audio = new Audio();
  Local<Object> audioObj = info.This();
  audio->Wrap(audioObj);

  info.GetReturnValue().Set(audioObj);
}

void Audio::Load(uint8_t *bufferValue, size_t bufferLength) {
  vector<uint8_t> buffer(bufferLength);
  memcpy(buffer.data(), bufferValue, bufferLength);

  string error;

  shared_ptr<lab::AudioBus> audioBus(lab::MakeBusFromMemory(buffer, false, &error));
  if (audioBus) {
    lab::AudioContext *defaultAudioContext = getDefaultAudioContext();
    {
      lab::ContextRenderLock lock(defaultAudioContext, "Audio::Load");
      audioNode->setBus(lock, audioBus);
    }

    defaultAudioContext->connect(defaultAudioContext->destination(), audioNode, 0, 0); // default connection
  } else {
    Nan::ThrowError(error.c_str());
  }
}

void Audio::Play() {
  audioNode->start(0);
}

void Audio::Pause() {
  audioNode->stop(0);
}

NAN_METHOD(Audio::Load) {
  if (info[0]->IsArrayBuffer()) {
    Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());

    Local<ArrayBuffer> arrayBuffer = Local<ArrayBuffer>::Cast(info[0]);

    audio->Load((uint8_t *)arrayBuffer->GetContents().Data(), arrayBuffer->ByteLength());
  } else if (info[0]->IsTypedArray()) {
    Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());

    Local<ArrayBufferView> arrayBufferView = Local<ArrayBufferView>::Cast(info[0]);
    Local<ArrayBuffer> arrayBuffer = arrayBufferView->Buffer();

    audio->Load((uint8_t *)arrayBuffer->GetContents().Data() + arrayBufferView->ByteOffset(), arrayBufferView->ByteLength());
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

  double now = getDefaultAudioContext()->currentTime();
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

NAN_GETTER(Audio::LoopGetter) {
  Nan::HandleScope scope;

  Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());

  bool loop = audio->audioNode->loop();
  info.GetReturnValue().Set(JS_BOOL(loop));
}

NAN_SETTER(Audio::LoopSetter) {
  Nan::HandleScope scope;

  if (value->IsBoolean()) {
    bool loop = value->BooleanValue();

    Audio *audio = ObjectWrap::Unwrap<Audio>(info.This());

    audio->audioNode->setLoop(loop);
  } else {
    Nan::ThrowError("loop: invalid arguments");
  }
}

}
