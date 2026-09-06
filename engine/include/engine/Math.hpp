#pragma once
#include <array>
#include <cmath>
#include <stdexcept>

namespace engine {
struct Vec3 { float x = 0, y = 0, z = 0; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
inline Vec3 normalized(Vec3 value) {
    const float length = std::sqrt(dot(value, value));
    if (length <= 0) { throw std::invalid_argument("Cannot normalize a zero vector."); }
    return {value.x/length, value.y/length, value.z/length};
}
// Column-major matrices: clip = projection * view * model * point.
struct Mat4 {
    std::array<float, 16> values{};
    static Mat4 identity() {
        Mat4 result;
        result.values[0] = result.values[5] = result.values[10] = result.values[15] = 1;
        return result;
    }
    static Mat4 transform(Vec3 position, Vec3 scale) {
        Mat4 result = identity();
        result.values[0] = scale.x; result.values[5] = scale.y; result.values[10] = scale.z;
        result.values[12] = position.x; result.values[13] = position.y; result.values[14] = position.z;
        return result;
    }
};
inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 result;
    for (int column=0; column<4; ++column)
        for (int row=0; row<4; ++row)
            for (int k=0; k<4; ++k)
                result.values[column*4+row] += a.values[k*4+row] * b.values[column*4+k];
    return result;
}
inline Vec3 transformPoint(const Mat4& matrix, Vec3 point) {
    const auto& m = matrix.values;
    const float w = m[3]*point.x + m[7]*point.y + m[11]*point.z + m[15];
    if (w == 0) { throw std::invalid_argument("Point has zero homogeneous W."); }
    return {(m[0]*point.x+m[4]*point.y+m[8]*point.z+m[12])/w,
        (m[1]*point.x+m[5]*point.y+m[9]*point.z+m[13])/w,
        (m[2]*point.x+m[6]*point.y+m[10]*point.z+m[14])/w};
}
inline Mat4 lookAt(Vec3 eye, Vec3 target) {
    const Vec3 forward = normalized(target-eye);
    const Vec3 right = normalized(cross(forward, {0,1,0}));
    const Vec3 up = cross(right, forward);
    Mat4 result = Mat4::identity();
    auto& m = result.values;
    m[0]=right.x; m[4]=right.y; m[8]=right.z;
    m[1]=up.x; m[5]=up.y; m[9]=up.z;
    m[2]=-forward.x; m[6]=-forward.y; m[10]=-forward.z;
    m[12]=-dot(right,eye); m[13]=-dot(up,eye); m[14]=dot(forward,eye);
    return result;
}
// Right-handed OpenGL projection: visible depth maps to [-1, +1].
inline Mat4 perspective(float verticalFov, float aspect, float nearPlane, float farPlane) {
    if (!(aspect > 0 && nearPlane > 0 && farPlane > nearPlane && verticalFov > 0 && verticalFov < 3.14159265f)) {
        throw std::invalid_argument("Invalid perspective parameters.");
    }
    const float f = 1.0f / std::tan(verticalFov * 0.5f);
    Mat4 result;
    auto& m = result.values;
    m[0]=f/aspect; m[5]=f; m[10]=(farPlane+nearPlane)/(nearPlane-farPlane);
    m[11]=-1; m[14]=(2*farPlane*nearPlane)/(nearPlane-farPlane);
    return result;
}
}
