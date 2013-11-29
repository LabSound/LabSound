
#pragma once

namespace WTF {
class MemoryObjectInfo;
};

class ActiveDOMObject
{
    public:
    ActiveDOMObject(void*, void*) { }
    
    void setPendingActivity(void*) { }
    void unsetPendingActivity(void*) { }
    void suspendIfNeeded() { }
};

class ScriptExecutionContext
{
public:
};
