#pragma once

#include <string>
#include <vector>

namespace lab {

class NodeRegistry
{
    struct Detail;
    Detail* _detail;

public:
    NodeRegistry();
    ~NodeRegistry();

    static NodeRegistry& Instance();
    static bool Register(char const*const name);
    std::vector<std::string> Names() const;
};



} // lab