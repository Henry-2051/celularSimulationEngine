#include <iostream>

template<typename T>
class maybeResult{
    bool m_isNothing;
    T m_value;

public:
    maybeResult() : m_isNothing(true) {};

    maybeResult(T val) : m_isNothing(false), m_value(val) {};

    maybeResult<T>
    setValue(T val) {
        m_value = val;
        m_isNothing = false;
        return *this;
    }

    maybeResult<T> merge(const maybeResult<T>& tryUntilOther) const {
        return (!m_isNothing ? *this : tryUntilOther);
    }

    // Higher-order method: takes a function generating tryUntilResult<T>, merges it with current
    template<typename Func>
    maybeResult<T> tryWith(Func f) const {
        maybeResult<T> other = f();
        return this->merge(other);
    }

    maybeResult<T> operator + (const maybeResult<T> m_val2) {
        if (this->exists() && m_val2.exists()) {
            return maybeResult(m_value + m_val2.m_value);
        } else { return maybeResult<T>(); }
    }

    template<typename Func>
    maybeResult<T> passThrough(Func f) {
        if (this->exists()) {
            maybeResult<T> result_val= f(m_value);
            return (result_val.exists() ? result_val : *this);
        } else {
            return *this;
        }
    }

    template<typename Func>
    maybeResult<T> nothingThen(Func f) {
        if (m_isNothing) {
            return f();
        } else {
            return *this;
        }
    }

    void print() {
        if (m_isNothing) {
            std::cout << "Nothing" << std::endl;
        } else {
            std::cout << "Value: "  << m_value << std::endl;
        }
    }

    T getValue() {
        if (not m_isNothing) {
            return m_value;
        } else {
            throw std::runtime_error("Trying to get the value of Nothing\n");
        }
    }
    bool exists() {
        return not m_isNothing;
    }
};
