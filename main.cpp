#include <v8.h>
#include <node.h>
#include <nan.h>
#include <memory>
#include <algorithm>
#include "LabSound/extended/LabSound.h"
#include <Audio.h>
#include <AudioContext.h>
#include <AudioDestinationNode.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {
  
unique_ptr<lab::AudioContext> defaultAudioContext;

void Init(Handle<Object> exports) {
  Isolate *isolate = Isolate::GetCurrent();

  defaultAudioContext = lab::MakeRealtimeAudioContext();
  
  exports->Set(JS_STR("Audio"), Audio::Initialize(isolate));
  Local<Value> audioDestinationNodeCons = AudioDestinationNode::Initialize(isolate);
  exports->Set(JS_STR("AudioDestinationNode"), audioDestinationNodeCons);
  exports->Set(JS_STR("AudioContext"), AudioContext::Initialize(isolate, audioDestinationNodeCons));
}
NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

}