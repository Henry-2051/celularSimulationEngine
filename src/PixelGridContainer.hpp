#include <cassert>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>


template<typename T>
class PixelCol {
    std::span<T> m_row;
    size_t m_height;
public:
    PixelCol(std::span<T> row, size_t height) : m_row(row), m_height(height) {
        std::cout << "height " << m_height << "\n";
    }

    // Overload to accept int without warnings on implicit conversion
    T& operator[](int row) {
        // assert(col >= 0 && static_cast<size_t>(col) < m_row.size());
        if ((row < 0) || row > (static_cast<int>(m_height))) { 
            std::cout << "trying to index " << row << " row, should be between 0 and " << m_height << "\n";
            throw std::runtime_error("trying to index out of row"); 
        };
        return m_row[static_cast<size_t>(row)];
    }

    const T& operator[](int row) const {
        // assert(col >= 0 && static_cast<size_t>(col) < m_row.size());
        if ((row < 0) || (row > static_cast<int>(m_height))) { 
            std::cout << "trying to index " << row << " row, should be between 0 and " << m_height << "\n";
            throw std::runtime_error("trying to index out of row"); 
        };
        return m_row[static_cast<size_t>(row)];
    }
};


template<typename T>
class PixelGridContainer
{
    size_t m_width {};
    size_t m_height {};
    std::vector<T> m_pixelGrid {};
    std::vector<PixelCol<T>> m_collums;
public:
    PixelGridContainer() = default;

    PixelGridContainer(int m_width, int m_height, T initial) : m_width(static_cast<size_t>(m_width)), m_height(static_cast<size_t>(m_height))
    {

        std::cout << "width and height " << m_width << ", " << m_height << "\n";
        m_pixelGrid.assign(m_width * m_height, initial);
        for (size_t i = 0; i < m_width; ++i) {
            m_collums.push_back(PixelCol(std::span<T>(m_pixelGrid.data() + i * m_height, m_height), m_height));
        } 
    }

    const PixelCol<T> operator[](int x) const {
        if ((x < 0) || (x > static_cast<int>(m_width))) {
            std::cout << "const trying to index " << x << " col, should be between 0 and " << m_width << "\n";
            throw std::runtime_error("trying to index out of col"); 
        }
        return m_collums[x];
    }


    PixelCol<T> operator[](int x) {
        if ((x < 0) || (x > static_cast<int>(m_width))) {
            std::cout << "trying to index " << x << " col, should be between 0 and " << m_width << "\n";
            throw std::runtime_error("trying to index out of col"); 
        }
        return m_collums[x];
    }

};
