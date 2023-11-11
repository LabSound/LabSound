// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/LabSound.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioDevice.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"
#include "LabSound/extended/Registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>


///////////////////////
// Logging Utilities //
///////////////////////

/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

//#define LOG_USE_COLOR

static struct {
  void *udata;
  log_LockFn lock;
  FILE *fp;
  int level;
  int quiet;
} L;


static const char *level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


static void lock(void)   {
  if (L.lock) {
    L.lock(L.udata, 1);
  }
}


static void unlock(void) {
  if (L.lock) {
    L.lock(L.udata, 0);
  }
}


void log_set_udata(void *udata) {
  L.udata = udata;
}


void log_set_lock(log_LockFn fn) {
  L.lock = fn;
}


void log_set_fp(FILE *fp) {
  L.fp = fp;
}


void log_set_level(int level) {
  L.level = level;
}


void log_set_quiet(int enable) {
  L.quiet = enable ? 1 : 0;
}


void LabSoundLog(int level, const char *file, int line, const char *fmt, ...) {
  if (level < L.level) {
    return;
  }

  /* Acquire lock */
  lock();

  /* Get current time */
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  /* Log to stderr */
  if (!L.quiet) {
    va_list args;
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(
      stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
      buf, level_colors[level], level_names[level], file, line);
#else
    fprintf(stderr, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
#endif
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  /* Log to file */
  if (L.fp) {
    va_list args;
    char buf[32];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
    fprintf(L.fp, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
    va_start(args, fmt);
    vfprintf(L.fp, fmt, args);
    va_end(args);
    fprintf(L.fp, "\n");
    fflush(L.fp);
  }

  /* Release lock */
  unlock();
}

void LabSoundAssertLog(const char * file_, int line, const char * function_, const char * assertion_)
{
    const char * file = file_ ? file_ : "Unknown source file";
    const char * function = function_ ? function_ : "";
    const char * assertion = assertion_ ? assertion_ : "Assertion failed";
    printf("Assertion: %s:%s:%d - %s\n", function, file, line, assertion);
}

std::once_flag register_all_flag;
void LabSoundRegistryInit(lab::NodeRegistry& reg)
{
    using namespace lab;
    
    std::call_once(register_all_flag, [&reg]() {
        
        // core
        
        reg.Register(
            AnalyserNode::static_name(), AnalyserNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new AnalyserNode(ac); },
            [](AudioNode* n) { delete n; });

        reg.Register(
            AudioDestinationNode::static_name(), AudioDestinationNode::desc(),
            [](AudioContext& ac)->AudioNode* { return nullptr; },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            AudioHardwareInputNode::static_name(), AudioHardwareInputNode::desc(),
            [](AudioContext& ac)->AudioNode* { return nullptr; },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            BiquadFilterNode::static_name(), BiquadFilterNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new BiquadFilterNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            ChannelMergerNode::static_name(), ChannelMergerNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new ChannelMergerNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            ChannelSplitterNode::static_name(), ChannelSplitterNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new ChannelSplitterNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            ConvolverNode::static_name(), ConvolverNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new ConvolverNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            DelayNode::static_name(), DelayNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new DelayNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            DynamicsCompressorNode::static_name(), DynamicsCompressorNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new DynamicsCompressorNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            GainNode::static_name(), GainNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new GainNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            OscillatorNode::static_name(), OscillatorNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new OscillatorNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            PannerNode::static_name(), PannerNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new PannerNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            SampledAudioNode::static_name(), SampledAudioNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new SampledAudioNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            StereoPannerNode::static_name(), StereoPannerNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new StereoPannerNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            WaveShaperNode::static_name(), WaveShaperNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new WaveShaperNode(ac); },
            [](AudioNode* n) { delete n; });
        
        // extended
        
        reg.Register(
            ADSRNode::static_name(), ADSRNode::desc(),
           [](AudioContext& ac)->AudioNode* { return new ADSRNode(ac); },
           [](AudioNode* n) { delete n; });
        
        reg.Register(
            ClipNode::static_name(), ClipNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new ClipNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            DiodeNode::static_name(), DiodeNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new DiodeNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            FunctionNode::static_name(), FunctionNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new FunctionNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            GranulationNode::static_name(), GranulationNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new GranulationNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            NoiseNode::static_name(), NoiseNode::desc(),
           [](AudioContext& ac)->AudioNode* { return new NoiseNode(ac); },
           [](AudioNode* n) { delete n; });

        reg.Register(
            PeakCompNode::static_name(), PeakCompNode::desc(),
            [](AudioContext & ac) -> AudioNode * { return new PeakCompNode(ac); },
            [](AudioNode * n) { delete n; });

        reg.Register(
            PolyBLEPNode::static_name(), PolyBLEPNode::desc(),
            [](AudioContext & ac) -> AudioNode * { return new PolyBLEPNode(ac); },
            [](AudioNode * n) { delete n; });

        reg.Register(
            PowerMonitorNode::static_name(), PowerMonitorNode::desc(),
            [](AudioContext & ac) -> AudioNode * { return new PowerMonitorNode(ac); },
            [](AudioNode * n) { delete n; });

        reg.Register(
            PWMNode::static_name(), PWMNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new PWMNode(ac); },
            [](AudioNode* n) { delete n; });
        
        //reg.Register(PdNode::static_name());
        
        reg.Register(
            RecorderNode::static_name(), RecorderNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new RecorderNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            SfxrNode::static_name(), SfxrNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new SfxrNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            SpectralMonitorNode::static_name(), SpectralMonitorNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new SpectralMonitorNode(ac); },
            [](AudioNode* n) { delete n; });
        
        reg.Register(
            SupersawNode::static_name(), SupersawNode::desc(),
            [](AudioContext& ac)->AudioNode* { return new SupersawNode(ac); },
            [](AudioNode* n) { delete n; });
    });
}

