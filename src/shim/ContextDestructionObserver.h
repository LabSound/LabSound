
#pragma once

namespace WebCore {

class ScriptExecutionContext { public: };

class ContextDestructionObserver {
public:
    explicit ContextDestructionObserver(ScriptExecutionContext*) { }
    virtual void contextDestroyed() { }

    ScriptExecutionContext* scriptExecutionContext() const { return m_scriptExecutionContext; }

protected:
    virtual ~ContextDestructionObserver() { }
    void observeContext(ScriptExecutionContext*);

    ScriptExecutionContext* m_scriptExecutionContext;
};

} // namespace WebCore

