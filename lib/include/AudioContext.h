#ifndef _AUDIO_CONTEXT_H_
#define _AUDIO_CONTEXT_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <globals.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {

class AudioContext : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate, Local<Value> audioDestinationNodeCons);
  void Close();
  void Suspend();
  void Resume();

protected:
  static NAN_METHOD(New);
  static NAN_METHOD(Close);
  static NAN_METHOD(Suspend);
  static NAN_METHOD(Resume);
  static NAN_GETTER(CurrentTimeGetter);
  static NAN_GETTER(DestinationGetter);

  AudioContext();
  ~AudioContext();

protected:
  Nan::Persistent<Object> destination;
};

}

#endif