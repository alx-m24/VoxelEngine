#pragma once

#ifndef NDEBUG
#include <cstdlib>
#include <iostream>

#define V_ASSERT_1(x)      V_ASSERT_2(x, "assertion failed: " #x)
#define V_ASSERT_2(x, msg) \
    do { \
        if (!(x)) { \
            std::cerr << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            std::abort(); \
        } \
    } while (false)

#define GET_V_ASSERT(_1, _2, NAME, ...) NAME
#define V_ASSERT(...) GET_V_ASSERT(__VA_ARGS__, V_ASSERT_2, V_ASSERT_1)(__VA_ARGS__)

#else
#define V_ASSERT(...) do {} while (false)
#endif
