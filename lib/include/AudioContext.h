#ifndef _AUDIO_CONTEXT_H_
#define _AUDIO_CONTEXT_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <globals.h>
#include <Audio.h>
#include <GainNode.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {

class AudioContext : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate, Local<Value> audioSourceNodeCons, Local<Value> audioDestinationNodeCons, Local<Value> gainNodeCons);
  void Close();
  Local<Object> CreateMediaElementSource(Local<Function> audioDestinationNodeConstructor, Local<Object> mediaElement, Local<Object> audioContextObj);
  void CreateMediaStreamSource();
  void CreateMediaStreamDestination();
  void CreateMediaStreamTrackSource();
  Local<Object> CreateGain(Local<Function> gainNodeConstructor, Local<Object> audioContextObj);
  void Suspend();
  void Resume();

protected:
  static NAN_METHOD(New);
  static NAN_METHOD(Close);
  static NAN_METHOD(CreateMediaElementSource);
  static NAN_METHOD(CreateMediaStreamSource);
  static NAN_METHOD(CreateMediaStreamDestination);
  static NAN_METHOD(CreateMediaStreamTrackSource);
  static NAN_METHOD(CreateGain);
  static NAN_METHOD(Suspend);
  static NAN_METHOD(Resume);
  static NAN_GETTER(CurrentTimeGetter);
  static NAN_GETTER(SampleRateGetter);

  AudioContext();
  ~AudioContext();

protected:
  lab::AudioContext *audioContext;
  
  friend class Audio;
  friend class AudioNode;
  friend class AudioSourceNode;
  friend class AudioDestinationNode;
  friend class GainNode;
  friend class AudioParam;
  friend class AudioAnalyser;
};

}

#endif