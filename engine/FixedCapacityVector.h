#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <cstddef>

namespace tetris {

template <typename T, size_t N>
class FixedCapacityVector {
public:
    FixedCapacityVector() : m_size(0) {}

    void push_back(const T& value) {
        if (m_size >= N) {
            // Ideally we don't throw in production, but for safety in C++ 
            // we should protect bounds. In high perf code, we might just ignore or assert.
            // A simple bound check is enough.
            return;
        }
        m_data[m_size++] = value;
    }

    void push_back(T&& value) {
        if (m_size >= N) return;
        m_data[m_size++] = std::move(value);
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (m_size >= N) return;
        m_data[m_size++] = T(std::forward<Args>(args)...);
    }

    void clear() { m_size = 0; }
    
    size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    
    T* data() { return m_data.data(); }
    const T* data() const { return m_data.data(); }

    T& operator[](size_t index) { return m_data[index]; }
    const T& operator[](size_t index) const { return m_data[index]; }

    T* begin() { return m_data.data(); }
    const T* begin() const { return m_data.data(); }
    T* end() { return m_data.data() + m_size; }
    const T* end() const { return m_data.data() + m_size; }

private:
    std::array<T, N> m_data;
    size_t m_size;
};

} // namespace tetris
