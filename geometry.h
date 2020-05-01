#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__
#include <cmath>
#include <cassert>
#include <iostream>

template <size_t DIM, typename T> struct vec {
    vec() { for (size_t i=DIM; i--; data_[i] = T()); }
          T& operator[](const size_t i)       { assert(i<DIM); return data_[i]; }
    const T& operator[](const size_t i) const { assert(i<DIM); return data_[i]; }
private:
    T data_[DIM];
};

typedef vec<2, float> vec2f;
typedef vec<3, float> vec3f;
typedef vec<3, int  > vec3i;
typedef vec<4, float> vec4f;

template <typename T> struct vec<2,T> {
    vec() : x(T()), y(T()) {}
    vec(T X, T Y) : x(X), y(Y) {}
    template <class U> vec<2,T>(const vec<2,U> &v);
          T& operator[](const size_t i)       { assert(i<2); return i<=0 ? x : y; }
    const T& operator[](const size_t i) const { assert(i<2); return i<=0 ? x : y; }
    T x,y;
};

template <typename T> struct vec<3,T> {
    vec() : x(T()), y(T()), z(T()) {}
    vec(T X, T Y, T Z) : x(X), y(Y), z(Z) {}
          T& operator[](const size_t i)       { assert(i<3); return i<=0 ? x : (1==i ? y : z); }
    const T& operator[](const size_t i) const { assert(i<3); return i<=0 ? x : (1==i ? y : z); }
	float norm() const { return std::sqrt(x * x + y * y + z * z); }
	float norm2() const { return (x*x+y*y+z*z); }
    vec<3,T> & normalize(T l=1) { *this = (*this)*(l/norm()); return *this; }
    T x,y,z;
};

template <typename T> struct vec<4,T> {
    vec() : x(T()), y(T()), z(T()), w(T()) {}
    vec(T X, T Y, T Z, T W) : x(X), y(Y), z(Z), w(W) {}
	float norm() const { return std::sqrt(x * x + y * y + z * z + w * w); }
	float norm2() const { return (x * x + y * y + z * z + w * w); }
          T& operator[](const size_t i)       { assert(i<4); return i<=0 ? x : (1==i ? y : (2==i ? z : w)); }
    const T& operator[](const size_t i) const { assert(i<4); return i<=0 ? x : (1==i ? y : (2==i ? z : w)); }
    T x,y,z,w;
};

template<size_t DIM,typename T>vec<DIM,T> operator+(vec<DIM,T> lhs, const vec<DIM,T>& rhs) {
    for (size_t i=DIM; i--; lhs[i]+=rhs[i]);
    return lhs;
}

template<size_t DIM, typename T>vec<DIM, T> operator+(vec<DIM, T> lhs, const T& rhs) {
	for (size_t i = DIM; i--; lhs[i] += rhs);
	return lhs;
}

template<size_t DIM, typename T>vec<DIM, T> operator-(vec<DIM, T> lhs, const T& rhs) {
	for (size_t i = DIM; i--; lhs[i] -= rhs);
	return lhs;
}

template<size_t DIM,typename T>vec<DIM,T> operator-(vec<DIM,T> lhs, const vec<DIM,T>& rhs) {
    for (size_t i=DIM; i--; lhs[i]-=rhs[i]);
    return lhs;
}

template<size_t DIM,typename T,typename U> vec<DIM,T> operator*(const vec<DIM,T> &lhs, const U& rhs) {
    vec<DIM,T> ret;
    for (size_t i=DIM; i--; ret[i]=lhs[i]*rhs);
    return ret;
}

template<size_t DIM, typename T, typename U> vec<DIM, T> operator*(const U& rhs, const vec<DIM, T>& lhs) {
    return lhs * rhs;
}

template<size_t DIM,typename T> vec<DIM,T> operator-(const vec<DIM,T> &lhs) {
    return lhs*T(-1);
}

template <typename T> vec<3,T> cross(vec<3,T> v1, vec<3,T> v2) {
    return vec<3,T>(v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
}

template <size_t DIM, typename T> std::ostream& operator<<(std::ostream& out, const vec<DIM,T>& v) {
    for(unsigned int i=0; i<DIM; i++) {
        out << v[i] << " " ;
    }
    return out ;
}

inline float dot(vec3f const& lhs, vec3f const& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline float angle(vec3f const& lhs, vec3f const& rhs)
{
	return dot(lhs, rhs) / (lhs.norm() * rhs.norm());
}

inline vec3f reflect(vec3f const& v, vec3f const& n)
{
    return v - 2 * dot(v, n) * n;
}

inline vec3f refract(vec3f const& v, vec3f const& n, float refractive_index)
{
    float cosi = -std::max(-1.0f, std::min(1.0f, dot(v, n)));
    float etai = 1, etat = refractive_index;

	vec3f N = n;
	if (cosi < 0)
	{
        cosi = -cosi;
        std::swap(etai, etat);
        N = -n;
	}

	float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);

    return k < 0 ? vec3f(0, 0, 0) : v * eta + N * (eta * cosi - sqrtf(k));
}

template<typename T>
auto lerp(T from, T to, float t)
{
    return (1 - t) * from + t * to;
}

#endif //__GEOMETRY_H__