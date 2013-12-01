
#ifndef FloatPoint3d_included_h
#define FloatPoint3d_included_h

#include <float.h>
#include <math.h>

class FloatPoint3D;
FloatPoint3D operator-(const FloatPoint3D& v1, const FloatPoint3D& v2);

class FloatPoint3D {
public:
    FloatPoint3D() { }

    FloatPoint3D(float x, float y, float z)
    : x(x), y(y), z(z) { }

    float x, y, z;

    bool isZero() const {
        return fabsf(x) < FLT_EPSILON && fabsf(y) < FLT_EPSILON && fabsf(z) < FLT_EPSILON;
    }

    void normalize() {
        if (!isZero()) {
            float l = 1.0f / sqrtf(x * x + y * y + z * z);
            x *= l;
            y *= l;
            z *= l;
        }
    }

    float length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    float dot(const FloatPoint3D& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    FloatPoint3D cross(const FloatPoint3D& point) const {
        FloatPoint3D result;
        result.cross(*this, point);
        return result;
    }

    inline void cross(const FloatPoint3D& v1, const FloatPoint3D& v2) {
        float x_ = v1.y * v2.z - v1.z * v2.y;
        float y_ = v1.z * v2.x - v1.x * v2.z;
        z = v1.x * v2.y - v1.y * v2.x;
        x = x_;
        y = y_;
    }

    FloatPoint3D& operator-(const FloatPoint3D& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    float distanceTo(const FloatPoint3D& rhs) const
    {
        return (*this - rhs).length();
    }
};

inline FloatPoint3D operator-(const FloatPoint3D& v1, const FloatPoint3D& v2) {
    return FloatPoint3D(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline FloatPoint3D operator*(float scale, const FloatPoint3D& rhs) {
    return FloatPoint3D(scale * rhs.x, scale * rhs.y, scale * rhs.z);
}


#endif
