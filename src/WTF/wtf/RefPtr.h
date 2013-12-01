
#ifndef included_wtf_RefPtr_h
#define included_wtf_RefPtr_h

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

    template<typename T> ALWAYS_INLINE PassRefPtr<T> adoptRef(T* p) {
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
}

using WTF::PassRefPtr;
using WTF::RefPtr;
using WTF::adoptRef;

#endif // WTF_RefPtr_h

