#include <AudioDestinationNode.h>
#include <memory>
#include <algorithm>

namespace webaudio {

AudioDestinationNode::AudioDestinationNode() {}
AudioDestinationNode::~AudioDestinationNode() {}

Handle<Object> AudioDestinationNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;
  
  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioDestinationNode"));
  
  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetAccessor(proto, JS_STR("maxChannelCount"), MaxChannelCountGetter);
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

NAN_METHOD(AudioDestinationNode::New) {
  Nan::HandleScope scope;

  AudioDestinationNode *audioDestinationNode = new AudioDestinationNode();
  Local<Object> audioDestinationNodeObj = info.This();
  audioDestinationNode->Wrap(audioDestinationNodeObj);

  info.GetReturnValue().Set(audioDestinationNodeObj);
}

NAN_GETTER(AudioDestinationNode::MaxChannelCountGetter) {
  Nan::HandleScope scope;
  
  info.GetReturnValue().Set(JS_INT(defaultAudioContext->maxNumberOfChannels));
}

}