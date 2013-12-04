// Copyright (c) 2013 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "config.h"
#include "ThreadSpecific.h"

#include "StdLibExtras.h"
#include "ThreadingPrimitives.h"
#include "DoublyLinkedList.h"

#if !USE(PTHREADS)

namespace WTF {

/*
static DoublyLinkedList<PlatformThreadSpecificKey>& destructorsList()
{
    DEFINE_STATIC_LOCAL(DoublyLinkedList<PlatformThreadSpecificKey>, staticList, ());
    return staticList;
}

static Mutex& destructorsMutex()
{
    DEFINE_STATIC_LOCAL(Mutex, staticMutex, ());
    return staticMutex;
}

class PlatformThreadSpecificKey : public DoublyLinkedListNode<PlatformThreadSpecificKey> {
public:
    friend class DoublyLinkedListNode<PlatformThreadSpecificKey>;

    PlatformThreadSpecificKey(void (*destructor)(void *))
        : m_destructor(destructor)
    {
        m_tlsKey = TlsAlloc();
        if (m_tlsKey == TLS_OUT_OF_INDEXES)
            CRASH();
    }

    ~PlatformThreadSpecificKey()
    {
        TlsFree(m_tlsKey);
    }

    void setValue(void* data) { TlsSetValue(m_tlsKey, data); }
    void* value() { return TlsGetValue(m_tlsKey); }

    void callDestructor()
    {
       if (void* data = value())
            m_destructor(data);
    }

private:
    void (*m_destructor)(void *);
    DWORD m_tlsKey;
    PlatformThreadSpecificKey* m_prev;
    PlatformThreadSpecificKey* m_next;
};

*/ 

long& tlsKeyCount()
{
    static long count;
    return count;
}

DWORD* tlsKeys()
{
    static DWORD keys[kMaxTlsKeySize];
    return keys;
}

/* 
void threadSpecificKeyCreate(ThreadSpecificKey* key, void (*destructor)(void *))
{
    *key = new PlatformThreadSpecificKey(destructor);

    MutexLocker locker(destructorsMutex());
    destructorsList().push(*key);
}

void threadSpecificKeyDelete(ThreadSpecificKey key)
{
    MutexLocker locker(destructorsMutex());
    destructorsList().remove(key);
    delete key;
}

void threadSpecificSet(ThreadSpecificKey key, void* data)
{
    key->setValue(data);
}

void* threadSpecificGet(ThreadSpecificKey key)
{
    return key->value();
}

*/

void ThreadSpecificThreadExit() {

    for (long i = 0; i < tlsKeyCount(); i++) {

        // The layout of ThreadSpecific<T>::Data does not depend on T. So we are safe to do the static cast to ThreadSpecific<int> in order to access its data member.
        ThreadSpecific<int>::Data* data = static_cast<ThreadSpecific<int>::Data*>(TlsGetValue(tlsKeys()[i]));
        if (data)
            data->destructor(data);
    }

}


} // namespace WTF

#endif // !USE(PTHREADS)
