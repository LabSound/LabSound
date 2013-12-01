
#ifndef Noncopyable_h_included
#define Noncopyable_h_included
#define WTF_MAKE_NONCOPYABLE(T) private: T(const T&); T& operator=(const T&)
#endif
