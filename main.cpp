#include <v8.h>
#include <node.h>
#include <nan.h>
#include <memory>
#include <algorithm>
#include "LabSound/extended/LabSound.h"
#include <Audio.h>
#include <AudioParam.h>
#include <FakeAudioParam.h>
#include <AudioSourceNode.h>
#include <AudioDestinationNode.h>
#include <GainNode.h>
#include <AnalyserNode.h>
#include <PannerNode.h>
#include <StereoPannerNode.h>
#include <AudioContext.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {
  
unique_ptr<lab::AudioContext> defaultAudioContext;

void Init(Handle<Object> exports) {
  Isolate *isolate = Isolate::GetCurrent();

  defaultAudioContext = lab::MakeRealtimeAudioContext();

  atexit([]() {
    defaultAudioContext.reset();
  });
  
  exports->Set(JS_STR("Audio"), Audio::Initialize(isolate));
  Local<Value> audioParamCons = AudioParam::Initialize(isolate);
  exports->Set(JS_STR("AudioParam"), audioParamCons);
  Local<Value> fakeAudioParamCons = FakeAudioParam::Initialize(isolate);
  exports->Set(JS_STR("FakeAudioParam"), fakeAudioParamCons);
  Local<Value> audioListenerCons = AudioListener::Initialize(isolate, fakeAudioParamCons);
  exports->Set(JS_STR("AudioListener"), audioListenerCons);
  Local<Value> audioSourceNodeCons = AudioSourceNode::Initialize(isolate);
  exports->Set(JS_STR("AudioSourceNode"), audioSourceNodeCons);
  Local<Value> audioDestinationNodeCons = AudioDestinationNode::Initialize(isolate);
  exports->Set(JS_STR("AudioDestinationNode"), audioDestinationNodeCons);
  Local<Value> gainNodeCons = GainNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("GainNode"), gainNodeCons);
  Local<Value> analyserNodeCons = AnalyserNode::Initialize(isolate);
  exports->Set(JS_STR("AnalyserNode"), analyserNodeCons);
  Local<Value> pannerNodeCons = PannerNode::Initialize(isolate, fakeAudioParamCons);
  exports->Set(JS_STR("PannerNode"), pannerNodeCons);
  Local<Value> stereoPannerNodeCons = StereoPannerNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("StereoPannerNode"), stereoPannerNodeCons);
  Local<Value> microphoneMediaStreamCons = MicrophoneMediaStream::Initialize(isolate);
  exports->Set(JS_STR("MicrophoneMediaStream"), microphoneMediaStreamCons);
  exports->Set(JS_STR("AudioContext"), AudioContext::Initialize(isolate, audioListenerCons, audioSourceNodeCons, audioDestinationNodeCons, gainNodeCons, analyserNodeCons, pannerNodeCons, stereoPannerNodeCons));
}
NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

}
