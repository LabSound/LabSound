
#include "LabSound/extended/Registry.h"

#include <cstdio>
#include <memory>
#include <map>

namespace lab {

NodeRegistry& NodeRegistry::Instance()
{
    static std::unique_ptr<NodeRegistry> s_registry;
    if (!s_registry)
        s_registry = std::make_unique<NodeRegistry>();

    return *s_registry.get();
}

struct NodeDescriptor
{
    std::string name;
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

bool NodeRegistry::Register(char const* const name, CreateNodeFn c, DeleteNodeFn d)
{
    printf("Registering %s\n", name);
    Instance()._detail->descriptors[name] = { name, c, d };
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



} // lab
