#pragma once

#include "LabSound/core/AudioNode.h"
#include <string>
#include <vector>

namespace lab {

typedef AudioNode* (*CreateNodeFn)(AudioContext&);
typedef void (*DeleteNodeFn)(AudioNode*);

class NodeRegistry
{
    struct Detail;
    Detail* _detail;
    NodeRegistry();
    ~NodeRegistry();

public:
    static NodeRegistry& Instance();
    bool Register(char const*const name, AudioNodeDescriptor*, CreateNodeFn, DeleteNodeFn);
    std::vector<std::string> Names() const;
    lab::AudioNode* Create(const std::string& n, lab::AudioContext& ac);
    AudioNodeDescriptor const * const Descriptor(const std::string & n) const;
};

} // lab
