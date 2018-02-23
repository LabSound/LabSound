#include <HTMLAudioElement.h>
#include <memory>
#include <algorithm>

namespace webaudio {

HTMLAudioElement::HTMLAudioElement() {}
HTMLAudioElement::~HTMLAudioElement() {}

Handle<Object> HTMLAudioElement::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("HTMLAudioElement"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto,"load", Load);
  Nan::SetMethod(proto,"play", Play);
  Nan::SetMethod(proto,"pause", Pause);
  Nan::SetAccessor(proto, JS_STR("currentTime"), CurrentTimeGetter);
  Nan::SetAccessor(proto, JS_STR("duration"), DurationGetter);
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

NAN_METHOD(HTMLAudioElement::New) {
  Nan::HandleScope scope;

  HTMLAudioElement *audioElement = new HTMLAudioElement();
  Local<Object> audioElementObj = info.This();
  audioElement->Wrap(audioElementObj);

  info.GetReturnValue().Set(audioElementObj);
}

void HTMLAudioElement::Load(uint8_t *bufferValue, size_t bufferLength, const char *extensionValue) {
  lab::ContextRenderLock lock(defaultAudioContext.get(), "HTMLAudioElement::Load");

  vector<uint8_t> buffer(bufferLength);
  memcpy(buffer.data(), bufferValue, bufferLength);
  
  string extension(extensionValue);

  audioBus = lab::MakeBusFromMemory(buffer, extension, false);
  audioNode.reset(new lab::SampledAudioNode());
  audioNode->setBus(lock, audioBus);
  
  // defaultAudioContext->connect(defaultAudioContext->destination(), audioNode, 0, 0); // XXX make this node connection manual
  defaultAudioContext->connect(defaultAudioContext->destination(), audioNode, 0, 0); // XXX make this node connection manual
}

void HTMLAudioElement::Play() {
  audioNode->start(0);
}

void HTMLAudioElement::Pause() {
  audioNode->stop(0);
}

NAN_METHOD(HTMLAudioElement::Load) {
  if (info[0]->IsTypedArray() && info[1]->IsString()) {
    HTMLAudioElement *audioElement = ObjectWrap::Unwrap<HTMLAudioElement>(info.This());

    Local<ArrayBufferView> arrayBufferView = Local<ArrayBufferView>::Cast(info[0]);
    Local<ArrayBuffer> arrayBuffer = arrayBufferView->Buffer();
    v8::String::Utf8Value extensionValue(info[1]->ToString());
    
    audioElement->Load((uint8_t *)arrayBuffer->GetContents().Data() + arrayBufferView->ByteOffset(), arrayBufferView->ByteLength(), *extensionValue);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(HTMLAudioElement::Play) {
  HTMLAudioElement *audioElement = ObjectWrap::Unwrap<HTMLAudioElement>(info.This());
  audioElement->Play();
}

NAN_METHOD(HTMLAudioElement::Pause) {
  HTMLAudioElement *audioElement = ObjectWrap::Unwrap<HTMLAudioElement>(info.This());
  audioElement->Pause();
}

NAN_GETTER(HTMLAudioElement::CurrentTimeGetter) {
  Nan::HandleScope scope;
  
  HTMLAudioElement *audioElement = ObjectWrap::Unwrap<HTMLAudioElement>(info.This());

  double now = defaultAudioContext->currentTime();
  double startTime = audioElement->audioNode->startTime();
  double duration = audioElement->audioNode->duration();
  double currentTime = std::min<double>(std::max<double>(startTime - now, 0), duration);

  info.GetReturnValue().Set(JS_NUM(currentTime));
}

NAN_GETTER(HTMLAudioElement::DurationGetter) {
  Nan::HandleScope scope;
  
  HTMLAudioElement *audioElement = ObjectWrap::Unwrap<HTMLAudioElement>(info.This());

  double duration = audioElement->audioNode->duration();

  info.GetReturnValue().Set(JS_NUM(duration));
}

}