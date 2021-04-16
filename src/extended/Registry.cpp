
#include "LabSound/extended/Registry.h"

#include <cstdio>
#include <memory>
#include <set>

namespace lab {

NodeRegistry& NodeRegistry::Instance()
{
    static std::unique_ptr<NodeRegistry> s_registry;
    if (!s_registry)
        s_registry = std::make_unique<NodeRegistry>();

    return *s_registry.get();
}

struct NodeRegistry::Detail
{
    std::set<std::string> names;
};


NodeRegistry::NodeRegistry() :
_detail(new Detail())
{
}

NodeRegistry::~NodeRegistry()
{
    delete _detail;
}

bool NodeRegistry::Register(char const* const name)
{
    printf("Registering %s\n", name);
    Instance()._detail->names.insert(name);
    return true;
}

std::vector<std::string> NodeRegistry::Names() const
{
    std::vector<std::string> names;
    for (const auto& i : _detail->names)
        names.push_back(i);

    return names;
}


} // lab
