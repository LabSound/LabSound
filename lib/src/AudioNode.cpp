#include <AudioNode.h>
#include <memory>
#include <algorithm>

namespace webaudio {

AudioNode::AudioNode() {}
AudioNode::~AudioNode() {}

Handle<Object> AudioNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioNode"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

NAN_METHOD(AudioNode::New) {
  Nan::HandleScope scope;

  AudioNode *audioNode = new AudioNode();
  Local<Object> audioNodeObj = info.This();
  audioNode->Wrap(audioNodeObj);

  info.GetReturnValue().Set(audioNodeObj);
}

}