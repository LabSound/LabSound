
#pragma once

namespace WTF {
class MemoryObjectInfo;
};

class ActiveDOMObject
{
    public:
    ActiveDOMObject(void*, void*) { }
    virtual void reportMemoryUsage(WTF::MemoryObjectInfo*) const {}
    
    void setPendingActivity(void*) { }
    void unsetPendingActivity(void*) { }
    void suspendIfNeeded() { }
};

class ScriptExecutionContext
{
public:
};
