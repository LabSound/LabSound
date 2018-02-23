#ifndef _AUDIO_NODE_H_
#define _AUDIO_NODE_H_

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

class AudioNode : public ObjectWrap {
public:
  static Handle<Object> Initialize(Isolate *isolate);

protected:
  static NAN_METHOD(New);

  AudioNode();
  ~AudioNode();

private:
};

}

#endif