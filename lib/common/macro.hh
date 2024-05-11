#pragma once

#define COREY_UNREACHABLE __builtin_unreachable

#define COREY_ASSERT(EXPR) do { \
    if (!(EXPR)) { \
        corey::panic("{}:{}: assertion failed ({})", __FILE__, __LINE__, #EXPR); \
    } \
} while (0)
