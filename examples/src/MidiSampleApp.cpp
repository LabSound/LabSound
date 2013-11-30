/*
 *  MidiSampleApp.cpp
 *
 */

/*
 Copyright (c) 2012, Nick Porcino
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 * The names of its contributors may not be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Note that the sample midi files were obtained from the jasmid
// distribution on github, and they contain their own internal
// copyright notices.

#include "MidiSampleApp.h"
#include "ConcurrentQueue.h"
#include "OptionParser.h"

#include "LabSound.h"

#include "ADSRNode.h"
#include "ClipNode.h"
#include "SfxrNode.h"
#include "OscillatorNode.h"
#include "SupersawNode.h"

#include "LabMidi/LabMidiCommand.h"
#include "LabMidi/LabMidiIn.h"
#include "LabMidi/LabMidiOut.h"
#include "LabMidi/LabMidiPorts.h"
#include "LabMidi/LabMidiSoftSynth.h"
#include "LabMidi/LabMidiSong.h"
#include "LabMidi/LabMidiSongPlayer.h"
#include "LabMidi/LabMidiUtil.h"

#include <iostream>

#ifdef _MSC_VER
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <time.h>
#   include <sys/time.h>
#   include <unistd.h>
#endif

using namespace LabSound;
using namespace Lab;

class MidiSynthDelegate {
public:
    virtual void noteOff(uint8_t channel, uint8_t program, uint8_t velocity) {}
    virtual void noteOn(uint8_t channel, uint8_t program, uint8_t note, uint8_t velocity) {}
    virtual void programChange(uint8_t channel, uint8_t program) {}
    virtual void polyPressure(uint8_t channel, uint8_t program) {}
    virtual void controlChange(uint8_t channel, uint8_t program) {}
    virtual void channelPressure(uint8_t channel, uint8_t program) {}
    virtual void pitchBend(uint8_t channel, uint8_t program) {}
};

class MidiStateMachine
{
public:
    static const int midiChannels = 16;
    uint8_t program[midiChannels];
    uint8_t channel;

    MidiStateMachine(MidiSynthDelegate* d)
    : channel(0)
    , delegate(d) {
        for (int i = 0; i < midiChannels; ++i) {
            program[i] = 1; // default acoustic grand piano
        }
    }

    ~MidiStateMachine() {
        // don't delete delegate, the state machine doesn't own it
    }

    void execMidiCommand(MidiCommand const*const mc) {
        uint8_t newChannel = mc->command & 0xf;
        switch (mc->command & 0xf0) {
            case MIDI_NOTE_OFF:
                channel = newChannel;
                if (delegate)
                    delegate->noteOff(channel, program[channel], mc->byte2);
                break;
            case MIDI_NOTE_ON:
                channel = newChannel;
                if (delegate)
                    delegate->noteOn(channel, program[channel], mc->byte1, mc->byte2);
                break;
            case MIDI_POLY_PRESSURE:
                channel = newChannel;
                if (delegate)
                    delegate->polyPressure(channel, mc->byte1);
                break;
            case MIDI_CONTROL_CHANGE:
                channel = newChannel;
                if (delegate)
                    delegate->controlChange(channel, mc->byte1);
                break;
            case MIDI_PROGRAM_CHANGE:
                program[channel] = mc->byte2;
                if (delegate)
                    delegate->programChange(channel, program[channel]);
                break;
            case MIDI_CHANNEL_PRESSURE:
                channel = newChannel;
                if (delegate)
                    delegate->channelPressure(channel, mc->byte1);
                break;
            case MIDI_PITCH_BEND:
                channel = newChannel;
                if (delegate)
                    delegate->pitchBend(channel, mc->byte1);
                break;

            case 0xf0:
                switch (mc->command) {
                    // system common
                    case MIDI_SYSTEM_EXCLUSIVE: break;
                    case MIDI_TIME_CODE: break;
                    case MIDI_SONG_POS_POINTER: break;
                    case MIDI_SONG_SELECT: break;
                    case MIDI_RESERVED1: break;
                    case MIDI_RESERVED2: break;
                    case MIDI_TUNE_REQUEST: break;
                    case MIDI_EOX: break;

                    // system realtime
                    case MIDI_TIME_CLOCK: break;
                    case MIDI_RESERVED3: break;
                    case MIDI_START: break;
                    case MIDI_CONTINUE: break;
                    case MIDI_STOP: break;
                    case MIDI_RESERVED4: break;
                    case MIDI_ACTIVE_SENSING: break;
                    case MIDI_SYSTEM_RESET: break;
                }
                break;
        }
    }

    void panic() {
        for (int i = 0; i < midiChannels; ++i) {
            if (delegate)
                delegate->noteOff(i, program[i], 0);
        }
    }

    MidiSynthDelegate* delegate;
};

class SimpleMidiSynth: public MidiSynthDelegate {
public:
    SimpleMidiSynth(AudioContext* context)
    : context(context) {
        masterGain = ClipNode::create(context, 44100);
        masterGain->setMode(ClipNode::TANH);
        masterGain->aVal()->setValue(1.0f);
        masterGain->bVal()->setValue(0.25f);
        LabSound::connect(masterGain.get(), context->destination());
    }

    virtual void noteOff(uint8_t channel, uint8_t program, uint8_t velocity) {
        for (auto i : oscs[channel]) {
            i->gain->noteOff();
        }
    }


    struct NoteRecord {
        int unique;
        RefPtr<OscillatorNode> osc;
        RefPtr<ADSRNode> gain;
    };

    static bool isFinished(const NoteRecord* i) {
        return i->gain->finished();
    }

    virtual void noteOn(uint8_t channel, uint8_t program, uint8_t note, uint8_t velocity) {
        static int unique = 0;
        ++unique;

        if (!velocity) {
            noteOff(channel, program, velocity);
        }
        else {
            float freq = Lab::noteToFrequency(note);
            if (freeOscs.empty()) {
                NoteRecord* newNote = new NoteRecord();
#if 0
                newNote->osc = SupersawNode::create(context, 44100);
                newNote->osc->detune()->setValue(2);
                newNote->osc->sawCount()->setValue(4);
                newNote->osc->update();
#else
                newNote->osc = OscillatorNode::create(context, 44100);
                ExceptionCode ec;
                newNote->osc->setType(OscillatorNode::SAWTOOTH, ec);
                newNote->osc->start(0);
#endif
                newNote->gain = ADSRNode::create(context, 44100);
                LabSound::connect(newNote->osc.get(), newNote->gain.get());
                LabSound::connect(newNote->gain.get(), masterGain.get());
                freeOscs.push_front(newNote);
            }
            auto osc = freeOscs.back();
            osc->unique = unique;
            freeOscs.pop_back();
            osc->osc->frequency()->setValue(freq);
            osc->osc->frequency()->resetSmoothedValue();
            osc->gain->noteOn();
            oscs[channel].push_front(osc);
            //printf("Created %d on %d\n", unique, channel);
        }

        // expire any used voices
        // First copy them to the free list
        for (int c = 0; c < 16; ++c)
            for (auto i = oscs[c].begin(); i != oscs[c].end(); ++i) {
                if ((*i)->gain->finished()) {
                    freeOscs.push_front(*i);
                    //printf("Expired %d\n", (*i)->unique);
                }
            }
        // then remove them from the active list
        for (int c = 0; c < 16; ++c)
          oscs[c].erase( std::remove_if(std::begin(oscs[c]), std::end(oscs[c]), isFinished), std::end(oscs[c]) );

        int inUse = 0;
        for (int c = 0; c < 16; ++c) {
            inUse += oscs[c].size();
        }
        //printf("%d [%d]\n", inUse, freeOscs.size());
    }
    virtual void programChange(uint8_t channel, uint8_t program) {
        //printf("%d %d\n", channel, program);
    }

    AudioContext* context;
    RefPtr<ClipNode> masterGain;

    std::deque<NoteRecord*> freeOscs;
    std::deque<NoteRecord*> oscs[16];
};



class MidiSampleApp::Detail
{
public:
    Detail(AudioContext* context)
    : midiIn(new Lab::MidiIn())
    , midiPorts(new Lab::MidiPorts())
    , midiSongPlayer(0)
    , synth(new SimpleMidiSynth(context))
    {
        msm = new MidiStateMachine(synth);
        if (!midiIn->createVirtualPort("MidiSampleApp"))
            std::cerr << "Couldn't create a virtual port called MidiSampleApp" << std::endl;
        else
            midiIn->addCallback(midiCallback, this);
    }

    ~Detail() {
        delete midiPorts;
        delete midiSongPlayer;
        delete midiIn;
        delete msm;
        delete synth;
    }

    static void midiCallback(void* userData, Lab::MidiCommand* mc) {
        // Don't want to block the MIDI thread. Audio commands can only be issued from
        // the audio main thread, so queue the midi commands for the audio main thread
        // service loop update.
        MidiSampleApp::Detail* self = (MidiSampleApp::Detail*) userData;
        self->midiQ.push(Lab::MidiCommand(*mc));
    }

    void update(double t) {
        midiSongPlayer->update(t);
        Lab::MidiCommand mc;
        if (midiQ.try_pop(mc)) {
            msm->execMidiCommand(&mc);
        }
    }

    void listPorts() {
        midiPorts->refreshPortList();
        int c = midiPorts->inPorts();
        if (c == 0)
            std::cout << "No MIDI input ports found" << std::endl;
        else {
            std::cout << "MIDI input ports:" << std::endl;
            for (int i = 0; i < c; ++i)
                std::cout << "   " << i << ": " << midiPorts->inPort(i) << std::endl;
            std::cout << std::endl;
        }

        c = midiPorts->outPorts();
        if (c == 0)
            std::cout << "No MIDI output ports found" << std::endl;
        else {
            std::cout << "MIDI output ports:" << std::endl;
            for (int i = 0; i < c; ++i)
                std::cout << "   " << i << ": " << midiPorts->outPort(i) << std::endl;
            std::cout << std::endl;
        }
    }

    Lab::MidiSongPlayer* midiSongPlayer;
    Lab::MidiIn* midiIn;
    Lab::MidiPorts* midiPorts;
    double startTime;

    MidiStateMachine* msm;
    SimpleMidiSynth* synth;

    concurrent_queue<Lab::MidiCommand> midiQ;
};

MidiSampleApp::MidiSampleApp(AudioContext* context)
: _detail(new Detail(context)) {
    _detail->listPorts();
}

MidiSampleApp::~MidiSampleApp() {
    delete _detail;
}

double MidiSampleApp::getElapsedSeconds() {
#ifdef _MSC_VER
    static double freq;
    static LARGE_INTEGER start;
    static bool init = true;
    if (init) {
        LARGE_INTEGER lFreq;
        QueryPerformanceFrequency(&lFreq);
        freq = double(lFreq.QuadPart);
        QueryPerformanceCounter(&lStart);
        init = false;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return double(now.QuadPart - start.QuadPart) / freq;
#else
    timeval t;
    gettimeofday(&t, 0);
    return double(t.tv_sec) + double(t.tv_usec) * 1.0e-6f;
#endif
}

void MidiSampleApp::setup() {
}

void MidiSampleApp::update(double t) {
    _detail->update(t);
}

bool MidiSampleApp::running(double t) {
    if (!_detail->midiSongPlayer)
        return false;

    return t <= (_detail->midiSongPlayer->length() + 0.5f);
}

int main(int argc, char** argv) {
    RefPtr<AudioContext> context = LabSound::init();
    MidiSampleApp app(context.get());

    OptionParser op("MidiPlayer");
    int port = -1;
    std::string path;
    op.AddIntOption("o", "outport", port, "A valid MIDI output port number");
    op.AddStringOption("f", "file", path, "Path to the file to play");
    if (op.Parse(argc, argv)) {
        if (port == -1 || !path.length())
            op.Usage();
        else {
            Lab::MidiOut* midiOut = new Lab::MidiOut();
            if (!midiOut->openPort(port)) {
                std::cerr << "Couldn't open port " << port << std::endl;
            }
            else {
                Lab::MidiSong* midiSong = new Lab::MidiSong();
                midiSong->parse(path.c_str(), true);
                app._detail->midiSongPlayer = new Lab::MidiSongPlayer(midiSong);
                app._detail->midiSongPlayer->addCallback(Lab::MidiOut::playerCallback, midiOut);
                app._detail->midiSongPlayer->play(0);

                double startTime = app.getElapsedSeconds();

                while (app.running(app.getElapsedSeconds() - startTime)) {
#ifdef _MSC_VER
                    Sleep(1); // 1ms delay --- to do - shouldn't sleep this long
#else
                    usleep(100); // 0.1ms delay
#endif
                    app.update(app.getElapsedSeconds() - startTime);
                }
                delete midiOut;
            }
        }
    }

    return 1;
}

