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

public:
    NodeRegistry();
    ~NodeRegistry();

    static NodeRegistry& Instance();
    static bool Register(char const*const name, CreateNodeFn, DeleteNodeFn);
    std::vector<std::string> Names() const;
    lab::AudioNode* Create(const std::string& n, lab::AudioContext& ac);
};



} // lab