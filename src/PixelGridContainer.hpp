#include "debugAssert.hpp"
#include <cassert>
#include <iostream>
#include <span>
#include <vector>

template<typename T>
class PixelCol
{
    std::span<T> m_row;
    size_t m_height;

  public:
    PixelCol(std::span<T> row, size_t height)
      : m_row(row)
      , m_height(height) {}

    // Overload to accept int without warnings on implicit conversion
    T& operator[](int row)
    {
        if (debugAssert::isDebugBuild && ((row < 0) || (row > static_cast<int>(m_height)))) {
            debugAssert::assertFailiure("row index out of bounds in PixelCol::operator[]");
        }

        return m_row[static_cast<size_t>(row)];
    }

    const T& operator[](int row) const
    {
        if (debugAssert::isDebugBuild && ((row < 0) || (row > static_cast<int>(m_height)))) {
            debugAssert::assertFailiure("row index out of bounds in const PixelCol::operator[]");
        }
        return m_row[static_cast<size_t>(row)];
    }
};

template<typename T>
class PixelGridContainer
{
    size_t m_width{};
    size_t m_height{};
    std::vector<T> m_pixelGrid{};
    std::vector<PixelCol<T>> m_collums;

  public:
    PixelGridContainer() = default;

    PixelGridContainer(int m_width, int m_height, T initial)
      : m_width(static_cast<size_t>(m_width))
      , m_height(static_cast<size_t>(m_height))
    {

        std::cout << "width and height " << m_width << ", " << m_height << "\n";
        m_pixelGrid.assign(m_width * m_height, initial);
        for (size_t i = 0; i < m_width; ++i) {
            m_collums.push_back(PixelCol(
                std::span<T>(m_pixelGrid.data() + i * m_height, m_height),
                m_height));
        }
    }

    const PixelCol<T>& operator[](int x) const
    {
        if (debugAssert::isDebugBuild && ((x < 0) || x > static_cast<int>(m_width))) {
        debugAssert::assertFailiure("col index out of bounds in const PixelGridContainer::operator[]");
        }
        return m_collums[x];
    }

    PixelCol<T>& operator[](int x)
    {
        if (debugAssert::isDebugBuild && ((x < 0) || (x > static_cast<int>(m_width)))) {
            debugAssert::assertFailiure("col index out of bounds in PixelGridContainer::operator[]");
        }
        return m_collums[x];
    }
};
