#include <AudioNode.h>
#include <AudioContext.h>

namespace webaudio {

AudioNode::AudioNode() : channelCountMode(""), channelInterpretation("") {}

AudioNode::~AudioNode() {}

Handle<Object> AudioNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);

  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

void AudioNode::InitializePrototype(Local<ObjectTemplate> proto) {
  Nan::SetMethod(proto, "connect", Connect);
  Nan::SetMethod(proto, "disconnect", Disconnect);
  Nan::SetAccessor(proto, JS_STR("context"), ContextGetter);
  Nan::SetAccessor(proto, JS_STR("numberOfInputsGetter"), NumberOfInputsGetter);
  Nan::SetAccessor(proto, JS_STR("numberOfOutputsGetter"), NumberOfOutputsGetter);
  Nan::SetAccessor(proto, JS_STR("channelCount"), ChannelCountGetter);
  Nan::SetAccessor(proto, JS_STR("channelCountMode"), ChannelCountModeGetter);
  Nan::SetAccessor(proto, JS_STR("channelInterpretation"), ChannelInterpretationGetter);
}

NAN_METHOD(AudioNode::New) {
  Nan::HandleScope scope;

  AudioNode *audioNode = new AudioNode();
  Local<Object> audioNodeObj = info.This();
  audioNode->Wrap(audioNodeObj);

  info.GetReturnValue().Set(audioNodeObj);
}

NAN_METHOD(AudioNode::Connect) {
  Nan::HandleScope scope;

  if (info[0]->IsObject()) {
    Local<Value> constructorName = info[0]->ToObject()->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("name"));

    if (constructorName->StrictEquals(JS_STR("AudioSourceNode")) || constructorName->StrictEquals(JS_STR("AudioDestinationNode"))) {
      unsigned int outputIndex = info[1]->IsNumber() ? info[1]->Uint32Value() : 0;
      unsigned int inputIndex = info[2]->IsNumber() ? info[2]->Uint32Value() : 0;

      AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());
      shared_ptr<lab::AudioNode> srcAudioNode = audioNode->audioNode;

      AudioNode *argAudioNode = ObjectWrap::Unwrap<AudioNode>(Local<Object>::Cast(info[0]));
      shared_ptr<lab::AudioNode> dstAudioNode = argAudioNode->audioNode;

      Local<Object> audioContextObj = Nan::New(audioNode->context);
      AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
      lab::AudioContext *labAudioContext = audioContext->audioContext;

      try {
        labAudioContext->connect(dstAudioNode, srcAudioNode, outputIndex, inputIndex);
      } catch (const std::exception &e) {
        return Nan::ThrowError(e.what());
      } catch (...) {
        return Nan::ThrowError("unknown exception");
      }

      info.GetReturnValue().Set(info[0]);
    } else {
      Nan::ThrowError("AudioNode::Connect: invalid arguments");
    }
  } else {
    Nan::ThrowError("AudioNode::Connect: invalid arguments");
  }
}

NAN_METHOD(AudioNode::Disconnect) {
  Nan::HandleScope scope;

  if (info.Length() == 0) {
    AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());
    shared_ptr<lab::AudioNode> srcAudioNode = audioNode->audioNode;
    
    Local<Object> audioContextObj = Nan::New(audioNode->context);
    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
    lab::AudioContext *labAudioContext = audioContext->audioContext;
    
    try {
      labAudioContext->disconnect(nullptr, srcAudioNode);
    } catch (const std::exception &e) {
      return Nan::ThrowError(e.what());
    } catch (...) {
      return Nan::ThrowError("unknown exception");
    }
  } else {
    if (info[0]->IsObject()) {
      Local<Value> constructorName = info[0]->ToObject()->Get(JS_STR("constructor"))->ToObject()->Get(JS_STR("name"));

      if (constructorName->StrictEquals(JS_STR("AudioSourceNode")) || constructorName->StrictEquals(JS_STR("AudioDestinationNode"))) {
        AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());
        shared_ptr<lab::AudioNode> srcAudioNode = audioNode->audioNode;
        
        AudioNode *argAudioNode = ObjectWrap::Unwrap<AudioNode>(Local<Object>::Cast(info[0]));
        shared_ptr<lab::AudioNode> dstAudioNode = argAudioNode->audioNode;

        Local<Object> audioContextObj = Nan::New(audioNode->context);
        AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
        lab::AudioContext *labAudioContext = audioContext->audioContext;
        try {
          labAudioContext->disconnect(dstAudioNode, srcAudioNode);
        } catch (const std::exception &e) {
          return Nan::ThrowError(e.what());
        } catch (...) {
          return Nan::ThrowError("unknown exception");
        }

        info.GetReturnValue().Set(info[0]);
      } else {
        Nan::ThrowError("AudioNode::Disconnect: invalid arguments");
      }
    } else {
      Nan::ThrowError("AudioNode::Disconnect: invalid arguments");
    }
  }
}

NAN_GETTER(AudioNode::ContextGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());
  if (!audioNode->context.IsEmpty()) {
    info.GetReturnValue().Set(Nan::New(audioNode->context));
  } else {
    info.GetReturnValue().Set(Nan::Null());
  }
}

NAN_GETTER(AudioNode::NumberOfInputsGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  info.GetReturnValue().Set(JS_INT(audioNode->numberOfInputs));
}

NAN_GETTER(AudioNode::NumberOfOutputsGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  info.GetReturnValue().Set(JS_INT(audioNode->numberOfOutputs));
}

NAN_GETTER(AudioNode::ChannelCountGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  info.GetReturnValue().Set(JS_INT(audioNode->channelCount));
}

NAN_GETTER(AudioNode::ChannelCountModeGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  info.GetReturnValue().Set(JS_STR(audioNode->channelCountMode));
}

NAN_GETTER(AudioNode::ChannelInterpretationGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  info.GetReturnValue().Set(JS_STR(audioNode->channelInterpretation));
}

}