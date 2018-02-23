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
  AudioNode::InitializePrototype(proto);
  AudioDestinationNode::InitializePrototype(proto);
  
  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

void AudioDestinationNode::InitializePrototype(Local<ObjectTemplate> proto) {
  Nan::SetAccessor(proto, JS_STR("maxChannelCount"), MaxChannelCountGetter);
}

NAN_METHOD(AudioDestinationNode::New) {
  Nan::HandleScope scope;

  AudioDestinationNode *audioDestinationNode = new AudioDestinationNode();
  Local<Object> audioDestinationNodeObj = info.This();
  audioDestinationNode->Wrap(audioDestinationNodeObj);

  audioDestinationNode->context.Reset(audioDestinationNodeObj);
  audioDestinationNode->numberOfInputs = 1;
  audioDestinationNode->numberOfOutputs = 0;
  audioDestinationNode->channelCount = 2;
  audioDestinationNode->channelCountMode = "explicit";
  audioDestinationNode->channelInterpretation = "speakers";

  info.GetReturnValue().Set(audioDestinationNodeObj);
}

NAN_GETTER(AudioDestinationNode::MaxChannelCountGetter) {
  Nan::HandleScope scope;
  
  info.GetReturnValue().Set(JS_INT(defaultAudioContext->maxNumberOfChannels));
}

}