
#include "LabSoundConfig.h"
#include "ThreadSpecific.h"
#include "ThreadingPrimitives.h"

#if !USE(PTHREADS)

namespace WTF {

long &tlsKeyCount() {

    static long numKeys;
    return numKeys;

}

DWORD* tlsKeys() {

    static DWORD tlsKeys[kMaxTlsKeySize];
    return tlsKeys;

}

void ThreadSpecificThreadExit() {

    for (long i = 0; i < tlsKeyCount(); i++) {

        ThreadSpecific<int>::Data* threadData = static_cast<ThreadSpecific<int>::Data*>(TlsGetValue(tlsKeys()[i]));
        if (threadData) threadData->destructor(threadData);
    }

}

} // namespace WTF

#endif // !USE(PTHREADS)
