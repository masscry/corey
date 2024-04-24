#pragma once

#define COREY_UNREACHABLE __builtin_unreachable

#define COREY_ASSERT(EXPR) ((EXPR) ?: (corey::panic("{}:{}: assertion failed ({})", __FILE__, __LINE__, #EXPR), false))
