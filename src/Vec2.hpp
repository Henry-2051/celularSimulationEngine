#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <iostream>
#include <math.h>
#include <ostream>

template <typename T>
struct Vec2
{
    T x = 0;
    T y = 0;

    Vec2() = default;

    Vec2(T xin, T yin)
        : x(xin), y(yin)
    { }

    // 
    Vec2(const sf::Vector2<T>& vec)
        : x(vec.x), y(vec.y)
    { }

    // auto conversion
    operator sf::Vector2<T>()
    {
        return sf::Vector2<T>(x, y);
    }    
    
    // operator conversion between vec2 types 
    template <typename U>
    operator Vec2<U>() const {
        return Vec2<U>(static_cast<U>(x), static_cast<U>(y));
    }

    Vec2 operator + (const Vec2& rhs) const
    {
        return Vec2(x + rhs.x, y + rhs.y);
    }
    
    Vec2 operator - (const Vec2& rhs) const
    {
        return Vec2(x - rhs.x, y - rhs.y);
    }

    Vec2 operator / (const T val) const {
        return Vec2(x / val, y / val);
    }

    Vec2 operator * (const T val) const {
        return Vec2(x * val, y * val);
    }

    Vec2 operator != (const Vec2& rhs) const {
        return (x != rhs.x) || (y != rhs.y);
    }

    Vec2 operator += (const Vec2 rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2 operator -= (const Vec2 rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vec2 operator *= (const T val) {
        x *= val;
        y *= val;
        return *this;
    }

    Vec2 operator *= (const Vec2 rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    Vec2 operator /= (const T val) {
        x /= val;
        y /= val;
        return *this;
    }

    T dist(const Vec2& rhs) const {
        return std::sqrt(pow(x - rhs.x, 2) + pow(y - rhs.y, 2));
    }

    T distsq(const Vec2& rhs) const {
       return  pow(x - rhs.x, 2) + pow(y - rhs.y, 2);
    }

    T abs() const {
        return std::sqrt(pow(x, 2) + pow(y, 2));
    }
    void print() {
        std::cout << "(x, y) : " << x << ", " << y << "\n";
    }
};

template <typename T>
Vec2<T> operator * (const T lhs, const Vec2<T> rhs) {
    return Vec2<T>(lhs * rhs.x, lhs * rhs.y);
}

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
