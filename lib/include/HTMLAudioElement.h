#ifndef _HTML_AUDIO_ELEMENT_H_
#define _HTML_AUDIO_ELEMENT_H_

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

class HTMLAudioElement : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate);
  void Load(uint8_t *bufferValue, size_t bufferLength, const char *extensionValue);
  void Play();
  void Pause();

protected:
  static NAN_METHOD(New);
  static NAN_METHOD(Load);
  static NAN_METHOD(Play);
  static NAN_METHOD(Pause);
  static NAN_GETTER(CurrentTimeGetter);
  static NAN_GETTER(DurationGetter);

  HTMLAudioElement();
  ~HTMLAudioElement();

private:
  shared_ptr<lab::AudioBus> audioBus;
  shared_ptr<lab::SampledAudioNode> audioNode;
};

}

#endif