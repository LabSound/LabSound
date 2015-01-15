
#ifndef included_wtf_RefPtr_h
#define included_wtf_RefPtr_h

#include "Assertions.h"
#include "PassOwnPtr.h"
#include <algorithm>

namespace WTF {

    template<typename T> ALWAYS_INLINE void refIfNotNull(T* ptr) {
        if (LIKELY(ptr != 0))
            ptr->ref();
    }

    template<typename T> ALWAYS_INLINE void derefIfNotNull(T* ptr) {
        if (LIKELY(ptr != 0))
            ptr->deref();
    }

    //----------------------------------------------------
    // PassRefPtr

    template<typename T> class RefPtr;
    template<typename T> class PassRefPtr;
    template<typename T> PassRefPtr<T> adoptRef(T*);

    template<typename T> class PassRefPtr {
    public:
        PassRefPtr(T* ptr) : _ptr(ptr) { refIfNotNull(ptr); }
        template<typename RefPtrType> PassRefPtr(const RefPtr<RefPtrType>&);

        T* leakRef() const;

        T* get() const { return _ptr; }

        T& operator*() const { return *_ptr; }
        ALWAYS_INLINE T* operator->() const { return _ptr; }

        ALWAYS_INLINE bool operator!() const { return 0 == _ptr; }
        ALWAYS_INLINE operator bool() const { return 0 != _ptr; }

        friend PassRefPtr adoptRef<T>(T*);

    private:
        PassRefPtr(T* ptr, bool) : _ptr(ptr) { }
        mutable T* _ptr;
    };

	// Don't force inline or VS2012+ will hella complain
    template<typename T> PassRefPtr<T> adoptRef(T* p) {
        return PassRefPtr<T>(p, true);
    }

    //----------------------------------------------------
    // RefPtr

    template<typename T> class RefPtr {
    public:
        RefPtr() : _ptr(0) { }
        
        RefPtr(T* d) : _ptr(d) {
            refIfNotNull(_ptr);
        }

        RefPtr(const RefPtr<T>& rhs) {
            _ptr = rhs.get();
            refIfNotNull(_ptr);
        }

        RefPtr(const PassRefPtr<T>& rhs) {
            _ptr = rhs.get();
            refIfNotNull(_ptr);
        }

        template<typename U>
        RefPtr(const PassRefPtr<U>& rhs) {
            _ptr = rhs.get();
            refIfNotNull(_ptr);
        }

        ~RefPtr() {
            derefIfNotNull(_ptr);
        }

        T* get() const { return _ptr; }

        T& operator*() const { return *_ptr; }
        ALWAYS_INLINE T* operator->() const { return _ptr; }

        ALWAYS_INLINE bool operator!() const { return 0 == _ptr; }
        ALWAYS_INLINE operator bool() const { return 0 != _ptr; }

        ALWAYS_INLINE RefPtr<T>& operator=(T* optr) {
            RefPtr ptr = optr;
            std::swap(_ptr, ptr._ptr);
            return *this;
        }

        ALWAYS_INLINE RefPtr<T>& operator=(const PassRefPtr<T>& o) {
            RefPtr ptr = o;
            std::swap(_ptr, ptr._ptr);
            return *this;
        }

        template<typename U>
        ALWAYS_INLINE RefPtr<T>& operator=(const PassRefPtr<U>& o) {
            RefPtr ptr(o);
            std::swap(_ptr, ptr._ptr);
            return *this;
        }

        ALWAYS_INLINE RefPtr<T>& operator=(RefPtr& o) {
            RefPtr ptr = std::move(o);
            std::swap(_ptr, ptr._ptr);
            return *this;
        }

        template<typename U>
        ALWAYS_INLINE RefPtr<T>& operator=(RefPtr<U>& o) {
            RefPtr ptr = std::move(o);
            std::swap(_ptr, ptr._ptr);
            return *this;
        }

        ALWAYS_INLINE void clear() {
            derefIfNotNull(_ptr);
            _ptr = 0;
        }

        PassRefPtr<T> release() {
            PassRefPtr<T> tmp = adoptRef(_ptr);
            _ptr = 0;
            return tmp;
        }

        T* _ptr;
    };

    template<typename T, typename PtrT>
    ALWAYS_INLINE
    bool operator==(const RefPtr<T>& ref, PtrT* ptr) { return ref.get() == ptr; }

    template<typename T, typename PtrT>
    ALWAYS_INLINE
    bool operator==(PtrT* ptr, const RefPtr<T>& ref) { return ref.get() == ptr; }

    template<typename T> template<typename RefPtrType> ALWAYS_INLINE PassRefPtr<T>::PassRefPtr(const RefPtr<RefPtrType>& rhs)
    : _ptr(rhs.get())
    {
        T* ptr = _ptr;
        refIfNotNull(ptr);
    }

    template<typename T> inline T* PassRefPtr<T>::leakRef() const {
        T* ptr = _ptr;
        _ptr = 0;
        return ptr;
    }

    //----------------------------------------------------
    // OwnPtr

    template<typename T> class PassOwnPtr;
    template<typename T> PassOwnPtr<T> adoptPtr(T*);

    template<typename T> class OwnPtr {
    public:
        typedef typename std::remove_pointer<T>::type ValueType;
        typedef ValueType* PtrType;

        OwnPtr() : _ptr(0) { }
        OwnPtr(std::nullptr_t) : _ptr(0) { }
        OwnPtr(const OwnPtr& rhs) : _ptr(rhs.leakPtr()) { }
        ~OwnPtr() { deleteOwnedPtr(_ptr); }

        template<typename U>
        OwnPtr(const PassOwnPtr<U>& o)
        : _ptr(o.leakPtr())
        {
        }

        template<typename U>
        OwnPtr(PassOwnPtr<U>& o)
        : _ptr(o.leakPtr())
        {
        }

        PtrType get() const { return _ptr; }
        ValueType& operator*() const { return *_ptr; }
        PtrType operator->() const { return _ptr; }
        ALWAYS_INLINE bool operator!() const { return 0 == _ptr; }
        ALWAYS_INLINE operator bool() const { return 0 != _ptr; }

        ALWAYS_INLINE PassOwnPtr<T> release() {
            PtrType ptr = _ptr;
            _ptr = 0;
            return adoptPtr(ptr);
        }

        ALWAYS_INLINE void clear() {
            PtrType ptr = _ptr;
            _ptr = 0;
            deleteOwnedPtr(ptr);
        }

        ALWAYS_INLINE typename OwnPtr<T>::PtrType leakPtr() const {
            PtrType ptr = _ptr;
            _ptr = 0;
            return ptr;
        }

        ALWAYS_INLINE OwnPtr<T>& operator=(const PassOwnPtr<T>& o) {
            PtrType ptr = _ptr;
            _ptr = o.leakPtr();
            ASSERT(!ptr || _ptr != ptr);
            deleteOwnedPtr(ptr);
            return *this;
        }

        template<typename U>
        ALWAYS_INLINE OwnPtr<T>& operator=(const PassOwnPtr<U>& o)  {
            PtrType ptr = _ptr;
            _ptr = o.leakPtr();
            ASSERT(!ptr || _ptr != ptr);
            deleteOwnedPtr(ptr);
            return *this;
        }

        mutable PtrType _ptr;
    };

    template<typename T, typename PtrT>
    ALWAYS_INLINE
    bool operator==(const OwnPtr<T>& ref, PtrT* ptr) { return ref.get() == ptr; }

    template<typename T, typename PtrT>
    ALWAYS_INLINE
    bool operator==(PtrT* ptr, const OwnPtr<T>& ref) { return ref.get() == ptr; }

}

using WTF::adoptRef;
using WTF::OwnPtr;
using WTF::PassOwnPtr;
using WTF::PassRefPtr;
using WTF::RefPtr;

#endif // WTF_RefPtr_h

