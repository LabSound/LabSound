
#include "LabSound/extended/Registry.h"

#include <cstdio>
#include <memory>
#include <map>
#include <mutex>


void LabSoundRegistryInit(lab::NodeRegistry&);

namespace lab {

std::once_flag instance_flag;
NodeRegistry& NodeRegistry::Instance()
{
    static NodeRegistry s_registry;
    std::call_once(instance_flag, []() { LabSoundRegistryInit(s_registry); });
    return s_registry;
}

struct NodeDescriptor
{
    std::string name;
    AudioNodeDescriptor * desc;
    CreateNodeFn c;
    DeleteNodeFn d;
};

struct NodeRegistry::Detail
{
    std::map<std::string, NodeDescriptor> descriptors;
};


NodeRegistry::NodeRegistry() :
_detail(new Detail())
{
}

NodeRegistry::~NodeRegistry()
{
    delete _detail;
}

bool NodeRegistry::Register(char const* const name, AudioNodeDescriptor* desc, CreateNodeFn c, DeleteNodeFn d)
{
    printf("Registering %s\n", name);
    _detail->descriptors[name] = { name, desc, c, d };
    return true;
}

std::vector<std::string> NodeRegistry::Names() const
{
    std::vector<std::string> names;
    for (const auto& i : _detail->descriptors)
        names.push_back(i.second.name);

    return names;
}

lab::AudioNode* NodeRegistry::Create(const std::string& n, lab::AudioContext& ac)
{
    auto i = _detail->descriptors.find(n);
    if (i == _detail->descriptors.end())
        return nullptr;

    return i->second.c(ac);
}

AudioNodeDescriptor const * const NodeRegistry::Descriptor(const std::string & n) const
{
    auto i = _detail->descriptors.find(n);
    if (i == _detail->descriptors.end())
        return nullptr;

    return i->second.desc;
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

} // lab
